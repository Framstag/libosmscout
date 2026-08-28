/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2020 Lukas Karas

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

#include <osmscoutclientqt/InstalledVoicesModel.h>
#include <osmscoutclientqt/OSMScoutQt.h>
#include <osmscoutclientqt/VoicePlayer.h>
#include <osmscoutclientqt/ClientQtFeatures.h>
#include <osmscoutclientqt/TTSEngine.h>
#include <osmscoutclientqt/TTSMessageGeneratorQt.h>
#ifdef OSMSCOUT_HAVE_LIB_PIPER
#include <osmscoutclientqt/PiperTTSEngine.h>
#endif

#include <osmscout/navigation/VoiceInstructionAgent.h>
#include <osmscout/util/Locale.h>

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0) /* For compatibility with QT 5.6 */
#include <QRandomGenerator>
#endif

#include <algorithm>
#include <array>
#include <utility>

namespace osmscout {

InstalledVoicesModel::InstalledVoicesModel()
{
  voiceManager=OSMScoutQt::GetInstance().GetVoiceManager();
  settings=OSMScoutQt::GetInstance().GetSettings();
  assert(voiceManager);
  assert(settings);

  settings->voiceDirChanged.Connect(voiceDirSlot);

  connect(voiceManager.get(), &VoiceManager::reloaded,
      this, &InstalledVoicesModel::update);

  voiceDir = QString::fromStdString(settings->GetVoiceDir());
  update();
}

InstalledVoicesModel::~InstalledVoicesModel()
{
  if (ttsEngine != nullptr) {
    // engine lives in its own thread, delete it there
    ttsEngine->deleteLater();
    ttsEngine = nullptr;
  }
}

namespace {
bool voiceItemLessThan(const Voice &i1, const Voice &i2)
{
  if (i1.getLang() != i2.getLang()) {
    return i1.getLang().localeAwareCompare(i2.getLang()) < 0;
  }
  return i1.getName().localeAwareCompare(i2.getName()) < 0;
}
}

void InstalledVoicesModel::update()
{
  beginResetModel();
  voices.clear();
  voices<<Voice(); // no-voice placeholder
  QList<Voice> installedVoices=voiceManager->getInstalledVoices();
  std::sort(installedVoices.begin(), installedVoices.end(), voiceItemLessThan);
  voices+=installedVoices;
  endResetModel();
}

void InstalledVoicesModel::onVoiceChanged(const QString& dir)
{
  voiceDir = dir;
  if (!QDir(voiceDir).exists()){
    voiceDir.clear(); // disable voice
  }

  QVector<int> roles;
  roles<<SelectedRole;
  emit dataChanged(createIndex(0,0), createIndex(voices.size()-1,0),roles);
}

int InstalledVoicesModel::rowCount(const QModelIndex &/*parent*/) const
{
  return voices.size();
}

QVariant InstalledVoicesModel::data(const QModelIndex &index, int role) const
{
  if (index.row() < 0 || index.row() >= voices.size()){
    return QVariant();
  }
  auto voice=voices.at(index.row());
  switch (role) {
    case Qt::DisplayRole:
    case NameRole:
      return voice.getName();
    case LangRole:
      return voice.getLang();
    case GenderRole:
      return voice.getGender();
    case ValidRole:
      return voice.isValid();
    case LicenseRole:
      return voice.getLicense();
    case AuthorRole:
      return voice.getAuthor();
    case DescriptionRole:
      return voice.getDescription();
    case SelectedRole:
      return (voiceDir.isEmpty() && !voice.isValid()) ||
             (voiceDir == voice.getDir().absolutePath());
    case TypeRole:
      return voice.getType();
    default:
      break;
  }
  return QVariant();
}

void InstalledVoicesModel::select(const QModelIndex &index)
{
  if (index.row() < 0 || index.row() >= voices.size()){
    return;
  }
  auto voice=voices.at(index.row());
  if (!voice.isValid()) {
    // when voice is invalid, directory may be still valid (default-constructed QDir pointing to $PWD)
    settings->SetVoiceDir("");
  } else {
    settings->SetVoiceDir(voice.getDir().absolutePath().toStdString());
  }
}

void InstalledVoicesModel::playSample(const QModelIndex &index, const QStringList &sample)
{
  if (index.row() < 0 || index.row() >= voices.size()){
    return;
  }
  auto voice=voices.at(index.row());
  if (!voice.isValid() || !voice.getDir().exists()){
    return;
  }

  if (mediaPlayer==nullptr){
    mediaPlayer = new VoiceCorePlayer(this);
  }

  mediaPlayer->clearQueue();

  for (const auto& file : sample){
    auto sampleUrl = QUrl::fromLocalFile(voice.getDir().path() + QDir::separator() + file);
    qDebug() << "Adding to playlist:" << sampleUrl;
    mediaPlayer->addToQueue(sampleUrl);
  }

  mediaPlayer->setCurrentIndex(0);
  mediaPlayer->play();
}

void InstalledVoicesModel::EnsureTTSEngine()
{
#ifdef OSMSCOUT_HAVE_LIB_PIPER
  if (ttsEngine==nullptr){
    ttsEngine = new PiperTTSEngine(OSMScoutQt::GetInstance().GetEspeakDataDir());

    // ttsEngine lives in its own background thread, invoke it asynchronously
    connect(this, &InstalledVoicesModel::initTTSVoiceRequested,
            ttsEngine, &TTSEngine::initVoice, Qt::QueuedConnection);
    connect(this, &InstalledVoicesModel::playTTSMessageRequested,
            ttsEngine, &TTSEngine::playMessage, Qt::QueuedConnection);
    connect(ttsEngine, &TTSEngine::playAudioFilesRequest,
            this, &InstalledVoicesModel::playTTSAudio, Qt::QueuedConnection);
    connect(ttsEngine, &TTSEngine::stateChange,
            this, &InstalledVoicesModel::onTTSStateChange, Qt::QueuedConnection);
    connect(ttsEngine, &TTSEngine::error,
            this, &InstalledVoicesModel::onTTSError, Qt::QueuedConnection);
  }
#endif
}

void InstalledVoicesModel::onTTSStateChange(TTSEngine::TTSEngineState state)
{
  ttsState = state;
  if (state != TTSEngine::TTSEngineState::Error) {
    // keep the last error message around while in the Error state, clear it
    // once the engine recovers
    ttsErrorMessage.clear();
  }
  emit ttsStateChanged();
}

void InstalledVoicesModel::onTTSError(const QString &message)
{
  ttsErrorMessage = message;
  // note: the accompanying stateChange(Error) signal (emitted right after
  // this one by the engine) triggers the ttsStateChanged() notification
}

QString InstalledVoicesModel::getTTSStateText() const
{
  switch (ttsState) {
  case TTSEngine::TTSEngineState::Initializing:
    return tr("Initializing");
  case TTSEngine::TTSEngineState::Synthesizing:
    return tr("Synthesizing voice sample");
  case TTSEngine::TTSEngineState::Error:
      return ttsErrorMessage.isEmpty()
        ? tr("Voice synthesis failed")
        : tr("Voice synthesis failed: %1").arg(ttsErrorMessage);
    case TTSEngine::TTSEngineState::Idle:
    default:
      return tr("Ready");
  }
}

void InstalledVoicesModel::playTTSAudio(const QList<QUrl> &audioFiles)
{
  if (mediaPlayer==nullptr){
    mediaPlayer = new VoiceCorePlayer(this);
  }

  mediaPlayer->clearQueue();
  for (const auto &audioFile : audioFiles){
    qDebug() << "Adding to playlist:" << audioFile;
    mediaPlayer->addToQueue(audioFile);
  }
  mediaPlayer->setCurrentIndex(0);
  mediaPlayer->play();
}

void InstalledVoicesModel::playTTSSample([[maybe_unused]] const QModelIndex &index)
{
#ifdef OSMSCOUT_HAVE_LIB_PIPER
  if (index.row() < 0 || index.row() >= voices.size()){
    return;
  }
  auto voice=voices.at(index.row());
  if (!voice.isValid() || !voice.isPiper() || !voice.getDir().exists()){
    return;
  }

  EnsureTTSEngine();
  assert(ttsEngine!=nullptr);

  if (ttsMessageGenerator==nullptr){
    ttsMessageGenerator = std::make_shared<TTSMessageGeneratorQt>(OSMScoutQt::GetInstance().GetNavigationTranslationDir());
  }
  if (ttsMessageLanguage != voice.getLangCode()){
    ttsMessageGenerator->SetLanguage(voice.getLangCode().toStdString());
    ttsMessageLanguage = voice.getLangCode();
  }
  ttsMessageGenerator->SetUnits(Locale::ByEnvironmentSafe().GetDistanceUnits());

  if (ttsEngine->getVoice().getDir() != voice.getDir()){
    emit initTTSVoiceRequested(voice);
  }

  // A handful of representative maneuver combinations, in the same spirit as
  // the "Voice of Marble" sample sets used above for pre-recorded voices.
  using Type = VoiceMessageStruct::Type;
  static const std::array<std::pair<VoiceMessageStruct, VoiceMessageStruct>, 5> samples{{
    {VoiceMessageStruct(Type::TurnRight, Meters(500)), VoiceMessageStruct()},
    {VoiceMessageStruct(Type::LeaveRbExit3, Meters(50)), VoiceMessageStruct(Type::StraightOn, Meters(100))},
    {VoiceMessageStruct(Type::LeaveMotorwayRight, Meters(800)), VoiceMessageStruct()},
    {VoiceMessageStruct(Type::TurnLeft, Meters(300)), VoiceMessageStruct()},
    {VoiceMessageStruct(Type::LeaveMotorwayLeft, Meters(1500)), VoiceMessageStruct()}
  }};

  size_t sampleIndex;
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0) /* For compatibility with QT 5.6 */
  sampleIndex = qrand() % samples.size();
#else
  sampleIndex = QRandomGenerator::global()->bounded(int(samples.size()));
#endif

  const auto &sample = samples[sampleIndex];
  auto text = ttsMessageGenerator->GenerateMessage(Meters(0), sample.first, sample.second);
  if (!text.has_value()){
    return;
  }

  emit playTTSMessageRequested(QString::fromStdString(*text));
#else
  qWarning() << "Piper TTS support not built, cannot play synthesized sample";
#endif
}
QHash<int, QByteArray> InstalledVoicesModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

  roles[NameRole]="name";
  roles[LangRole]="lang";
  roles[GenderRole]="gender";
  roles[ValidRole]="valid";
  roles[SelectedRole]="selected";
  roles[TypeRole]="type";

  return roles;
}

Qt::ItemFlags InstalledVoicesModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

}
