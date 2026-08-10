#ifndef OSMSCOUT_CLIENT_QT_VOICE_H
#define OSMSCOUT_CLIENT_QT_VOICE_H
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

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <osmscoutclient/VoiceProvider.h>

#include <QObject>
#include <QDir>

namespace osmscout {

/**
 * Type of the "Voice of Marble" voice packs, made of pre-recorded ogg samples.
 */
constexpr char const *VoiceTypeVoiceOfMarble = "VoiceOfMarble";

/**
 * Type of Piper TTS voices, made of an onnx model and its json config.
 */
constexpr char const *VoiceTypePiper = "Piper";

class OSMSCOUT_CLIENT_QT_API AvailableVoice : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool valid READ isValid() CONSTANT)

  Q_PROPERTY(QString type READ getType() CONSTANT)
  Q_PROPERTY(QString lang READ getLang() CONSTANT)
  Q_PROPERTY(QString langCode READ getLangCode() CONSTANT)
  Q_PROPERTY(QString gender READ getGender() CONSTANT)
  Q_PROPERTY(QString name READ getName() CONSTANT)
  Q_PROPERTY(QString license READ getLicense() CONSTANT)
  Q_PROPERTY(QString directory READ getDirectory() CONSTANT)
  Q_PROPERTY(QString author READ getAuthor() CONSTANT)
  Q_PROPERTY(QString description   READ getDescription() CONSTANT)

private:
  bool valid{false};

  VoiceProvider provider;

  QString type;
  QString lang;
  QString langCode;
  QString gender;
  QString name;
  QString license;
  QString directory;
  QString author;
  QString description;

  // Piper specific: server-relative paths to the model and its json config
  QString model;
  QString metadataPath;

public:
  AvailableVoice() = default;

  AvailableVoice(const VoiceProvider &provider,
                 const QString &type,
                 const QString &lang,
                 const QString &langCode,
                 const QString &gender,
                 const QString &name,
                 const QString &license,
                 const QString &directory,
                 const QString &author,
                 const QString &description,
                 const QString &model,
                 const QString &metadataPath);

  AvailableVoice(const AvailableVoice& o);

  ~AvailableVoice() override = default;

  VoiceProvider getProvider() const
  {
    return provider;
  }

  QString getType() const
  {
    return type;
  }
  bool isPiper() const
  {
    return type == VoiceTypePiper;
  }
  QString getLang() const
  {
    return lang;
  }
  QString getLangCode() const
  {
    return langCode;
  }
  QString getGender() const
  {
    return gender;
  }
  QString getName() const
  {
    return name;
  }
  QString getLicense() const
  {
    return license;
  }
  QString getDirectory() const
  {
    return directory;
  }
  QString getAuthor() const
  {
    return author;
  }
  QString getDescription() const
  {
    return description;
  }
  QString getModel() const
  {
    return model;
  }
  QString getMetadataPath() const
  {
    return metadataPath;
  }

  bool isValid() const
  {
    return valid;
  }
};

/**
 * Holder for voice metadata
 *
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API Voice
{
public:
  Voice() = default;
  explicit Voice(QDir dir);
  ~Voice() = default;

  Voice(const Voice &other) = default;
  Voice &operator=(const Voice &other) = default;

  Voice(Voice &&other) = default;
  Voice &operator=(Voice &&other) = default;

  QDir getDir() const
  {
    return dir;
  }

  QString getType() const
  {
    return type;
  }

  bool isPiper() const
  {
    return type == VoiceTypePiper;
  }

  QString getLang() const
  {
    return lang;
  }

  QString getLangCode() const
  {
    return langCode;
  }

  QString getGender() const
  {
    return gender;
  }

  QString getName() const
  {
    return name;
  }

  QString getLicense() const
  {
    return license;
  }

  QString getAuthor() const
  {
    return author;
  }

  QString getDescription() const
  {
    return description;
  }

  /**
   * Local file name of the Piper onnx model (empty for other types).
   */
  QString getModelFile() const
  {
    return modelFile;
  }

  bool isValid() const
  {
    return valid;
  }

  bool deleteVoice();

  /**
   * List of files owned by this voice (depends on its type), relative to its
   * directory. Used for validation and removal.
   */
  QStringList files() const;

  /**
   * Mandatory files of a "Voice of Marble" ogg sample pack.
   */
  static QStringList marbleFiles();

private:
  QDir dir;
  bool valid{false};
  bool metadata{false};

  QString type;
  QString lang;
  QString langCode;
  QString gender;
  QString name;
  QString license;
  QString author;
  QString description;
  QString modelFile;
};

}

#endif //OSMSCOUT_CLIENT_QT_VOICE_H
