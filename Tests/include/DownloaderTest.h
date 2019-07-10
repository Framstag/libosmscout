/*
  ClientQtThreading - a test program for libosmscout
  Copyright (C) 2019  Lukas Karas

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef LIBOSMSCOUT_DOWNLOADERTEST_H
#define LIBOSMSCOUT_DOWNLOADERTEST_H


#define OSMSCOUT_CLIENT_QT_FILEDOWNLOADER_TEST 1

#include <osmscout/FileDownloader.h>

#include <QObject>


struct Arguments
{
  bool                help{false};
  QString             url;
  QString             destination;
  uint64_t            from{0};
};

class DownloaderTest : public QObject
{
  Q_OBJECT

private:
  QNetworkAccessManager webCtrl;
  osmscout::FileDownloader downloader;
  int resultVal{0};

  signals:
      void finished();

public slots:
      void onError(QString error_text, bool recoverable)
  {
    qWarning() << error_text;
    if (!recoverable){
      resultVal+=1;
      emit finished();
    }
  };

public:
  DownloaderTest(const Arguments &args):
      downloader(&webCtrl, args.url, args.destination)
  {
    downloader.downloaded=args.from;
    connect(&downloader, &osmscout::FileDownloader::finished, this, &DownloaderTest::finished);
    connect(&downloader, &osmscout::FileDownloader::error, this, &DownloaderTest::onError);

    QTimer *startTimer = new QTimer();
    startTimer->setInterval(0);
    startTimer->setSingleShot(true);
    connect(startTimer, &QTimer::timeout, &downloader, &osmscout::FileDownloader::startDownload);
    connect(startTimer, &QTimer::timeout, startTimer, &QTimer::deleteLater);
    startTimer->start();
  }

  int result() const
  {
    return resultVal;
  }
};

#endif //LIBOSMSCOUT_DOWNLOADERTEST_H
