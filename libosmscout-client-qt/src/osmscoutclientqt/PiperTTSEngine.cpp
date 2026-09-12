/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2026 Lukas Karas

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <osmscoutclientqt/PiperTTSEngine.h>

#include <osmscout/io/File.h>
#include <osmscout/log/Logger.h>

#include <QCryptographicHash>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>

#include <algorithm>
#include <cmath>
#include <vector>

#include <piper.h>

namespace osmscout {

namespace {

/**
 * Write 16 bit PCM mono samples into a WAV file.
 */
bool WriteWav(const QString &path, const std::vector<int16_t> &samples, int sampleRate)
{
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }

  const uint16_t channels = 1;
  const uint16_t bitsPerSample = 16;
  const uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
  const uint16_t blockAlign = channels * (bitsPerSample / 8);
  const uint32_t dataSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
  const uint32_t riffSize = 36 + dataSize;

  QDataStream out(&file);
  out.setByteOrder(QDataStream::LittleEndian);

  auto writeTag = [&](const char *tag) {
    out.writeRawData(tag, 4);
  };

  writeTag("RIFF");
  out << riffSize;
  writeTag("WAVE");

  writeTag("fmt ");
  out << static_cast<uint32_t>(16);          // fmt chunk size
  out << static_cast<uint16_t>(1);           // PCM
  out << channels;
  out << static_cast<uint32_t>(sampleRate);
  out << byteRate;
  out << blockAlign;
  out << bitsPerSample;

  writeTag("data");
  out << dataSize;
  if (!samples.empty()) {
    out.writeRawData(reinterpret_cast<const char *>(samples.data()),
                     static_cast<int>(dataSize));
  }

  file.close();
  return out.status() == QDataStream::Ok;
}

} // namespace

PiperTTSEngine::PiperTTSEngine(const QString &espeakDataDir):
  espeakDataPath(espeakDataDir)
{
  // directory for synthesized WAV files, in the per-application cache
  // location (e.g. ~/.cache/<organization>/<app>/tts on Unix)
  QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  cacheDir.setPath(QDir(cacheRoot).filePath("tts"));
  if (!cacheDir.exists()) {
    cacheDir.mkpath(".");
  }
}

PiperTTSEngine::~PiperTTSEngine()
{
  closeSynthesizer();
}

void PiperTTSEngine::closeSynthesizer()
{
  if (synthesizer != nullptr) {
    piper_free(synthesizer);
    synthesizer = nullptr;
  }
}

void PiperTTSEngine::cleanCache()
{
  // TODO: make the cache size limit configurable
  constexpr qint64 cacheSizeLimit = 50 * 1024 * 1024; // 50 MiB

  // WAV files, oldest first (QDir::Time sorts newest first, so reverse)
  QFileInfoList entries = cacheDir.entryInfoList(QStringList{"*.wav"},
                                                 QDir::Files,
                                                 QDir::Time | QDir::Reversed);

  qint64 totalSize = 0;
  for (const auto &entry : entries) {
    totalSize += entry.size();
  }

  // remove the oldest samples until the cache is below the limit
  for (const auto &entry : entries) {
    if (totalSize <= cacheSizeLimit) {
      break;
    }
    qint64 size = entry.size();
    if (QFile::remove(entry.absoluteFilePath())) {
      totalSize -= size;
      log.Debug() << "PiperTTSEngine: removed cached sample " << entry.fileName().toStdString();
    } else {
      log.Warn() << "PiperTTSEngine: failed to remove cached sample " << entry.absoluteFilePath().toStdString();
    }
  }
}

void PiperTTSEngine::initVoice(const Voice &voice)
{
  state = TTSEngineState::Initializing;
  emit stateChange(state);

  this->voice = voice;
  // synthesized messages of a previous voice are no longer valid
  cache.clear();
  voiceId.clear();
  closeSynthesizer();

  if (!voice.isValid() || !voice.isPiper()) {
    log.Warn() << "PiperTTSEngine: voice is not a valid Piper voice";
    emit error(tr("The selected voice is not a valid Piper voice."));
    state = TTSEngineState::Error;
    emit stateChange(state);
    return;
  }

  // identifier of the voice, mixed into the sample hash so that cached WAV
  // files of different voices do not collide
  voiceId = voice.getName() + "|" + voice.getDir().absolutePath();

  QString modelPath = voice.getDir().filePath(voice.getModelFile());
  QString configPath = modelPath + ".json";

  if (espeakDataPath.isEmpty()) {
    log.Error() << "PiperTTSEngine: espeak-ng data directory was not configured";
    emit error(tr("The espeak-ng data directory was not configured."));
    state = TTSEngineState::Error;
    emit stateChange(state);
    return;
  }

  // Exit if espeak data /phontab do not exist, as libpiper do not use espeakINITIALIZE_DONT_EXIT option,
  // Piper is aborting process when this is not found!
  if (auto phontabFile = AppendFileToDir(espeakDataPath.toStdString(), "phontab");
      ExistsInFilesystem(phontabFile) == false) {
    log.Error() << "espeak-ng data directory does not contain phontab file: " << phontabFile;
    emit error(tr("The espeak-ng data directory does not contain the required data files."));
    state = TTSEngineState::Error;
    emit stateChange(state);
    return;
  }

  synthesizer = piper_create(modelPath.toUtf8().constData(),
                             configPath.toUtf8().constData(),
                             espeakDataPath.toUtf8().constData());
  if (synthesizer == nullptr) {
    log.Error() << "PiperTTSEngine: failed to load Piper voice model " << modelPath.toStdString();
    emit error(tr("Failed to load the voice model \"%1\".").arg(voice.getName()));
    state = TTSEngineState::Error;
    emit stateChange(state);
  } else {
    log.Debug() << "PiperTTSEngine: loaded Piper voice model " << modelPath.toStdString();
    state = TTSEngineState::Idle;
    emit stateChange(state);
  }
}

Voice PiperTTSEngine::getVoice()
{
  return voice;
}

QString PiperTTSEngine::wavFilePath(const QString &message) const
{
  // mix the voice identifier into the hash, so that cached samples of
  // different voices do not collide
  QCryptographicHash hasher(QCryptographicHash::Sha1);
  hasher.addData(voiceId.toUtf8());
  hasher.addData(QByteArray(1, '\0')); // separator
  hasher.addData(message.toUtf8());
  return cacheDir.filePath(QString::fromLatin1(hasher.result().toHex()) + ".wav");
}

QString PiperTTSEngine::prepare(const QString &message)
{
  if (message.isEmpty()) {
    return {};
  }

  // already synthesized?
  if (auto it = cache.find(message); it != cache.end()) {
    return it.value();
  }

  QString path = wavFilePath(message);
  if (QFile::exists(path)) {
    cache.insert(message, path);
    return path;
  }

  if (synthesizer == nullptr) {
    log.Warn() << "PiperTTSEngine: no voice loaded, cannot synthesize message";
    emit error(tr("No voice is loaded, cannot synthesize the message."));
    state = TTSEngineState::Error;
    emit stateChange(state);
    return {};
  }

  // actual synthesis is about to start, this may take a while for the first
  // message of a voice (model warm-up), let the UI know
  state = TTSEngineState::Synthesizing;
  emit stateChange(state);

  piper_synthesize_options options = piper_default_synthesize_options(synthesizer);
  if (piper_synthesize_start(synthesizer, message.toUtf8().constData(), &options) != PIPER_OK) {
    log.Error() << "PiperTTSEngine: failed to start synthesis for \"" << message.toStdString() << "\"";
    emit error(tr("Failed to start voice synthesis."));
    state = TTSEngineState::Error;
    emit stateChange(state);
    return {};
  }

  std::vector<int16_t> samples;
  int sampleRate = 22050; // sensible default, overwritten by the first chunk
  piper_audio_chunk chunk;
  int rc = PIPER_OK;
  while ((rc = piper_synthesize_next(synthesizer, &chunk)) == PIPER_OK || rc == PIPER_DONE) {
    if (chunk.num_samples > 0 && chunk.samples != nullptr) {
      sampleRate = chunk.sample_rate;
      samples.reserve(samples.size() + chunk.num_samples);
      for (size_t i = 0; i < chunk.num_samples; ++i) {
        float v = std::clamp(chunk.samples[i], -1.0f, 1.0f);
        samples.push_back(static_cast<int16_t>(std::lround(v * 32767.0f)));
      }
    }
    if (rc == PIPER_DONE || chunk.is_last) {
      break;
    }
  }

  if (rc < 0) {
    log.Error() << "PiperTTSEngine: synthesis failed for \"" << message.toStdString() << "\"";
    emit error(tr("Voice synthesis failed."));
    state = TTSEngineState::Error;
    emit stateChange(state);
    return {};
  }

  if (!WriteWav(path, samples, sampleRate)) {
    log.Error() << "PiperTTSEngine: failed to write WAV file " << path.toStdString();
    emit error(tr("Failed to write the synthesized audio file."));
    state = TTSEngineState::Error;
    emit stateChange(state);
    return {};
  }

  cache.insert(message, path);
  state = TTSEngineState::Idle;
  emit stateChange(state);
  return path;
}

void PiperTTSEngine::prepareMessage(QString message)
{
  prepare(message);
}

void PiperTTSEngine::playMessage(QString message)
{
  QString path = prepare(message);
  if (path.isEmpty()) {
    return;
  }

  QList<QUrl> urls;
  urls << QUrl::fromLocalFile(path);
  emit playAudioFilesRequest(urls);
}

}















