/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2018 Lukas Karas

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

#include <StyleAnalyser.h>
#include <osmscoutclient/DBThread.h>
#include <osmscoutclientqt/OSMScoutQt.h>

using namespace osmscout;

osmscout::TypeConfigRef getTypeConfig()
{
  osmscout::TypeConfigRef typeConfig;
  OSMScoutQt::GetInstance().GetDBThread()->RunSynchronousJob(
      [&typeConfig](const std::list<DBInstanceRef>& databases) {
        for (auto &db:databases) {
          auto database=db->GetDatabase();
          if (database && database->IsOpen() && database->GetTypeConfig()){
            typeConfig = database->GetTypeConfig();
            return;
          }
        }
      }
  );
  return typeConfig;
}

StyleAnalyser::StyleAnalyser(const QString& threadName)
    : typeConfig(getTypeConfig())
{
  thread = osmscout::OSMScoutQt::GetInstance().makeThread(threadName);
  this->moveToThread(thread);
  thread->start();

  // setup the root path for modules
  styleSheetFilePath = QString::fromUtf8(osmscout::OSMScoutQt::GetInstance().GetSettings()->GetStyleSheetDirectory().c_str())
                       + QDir::separator() + "main.oss";

  if (typeConfig) {
    connect(this, SIGNAL(updateRequest(QTextDocument*)),
            this, SLOT(update(QTextDocument*)),
            Qt::QueuedConnection);
  } else {
    osmscout::log.Error() << "Type Config not set";
  }
}

StyleAnalyser::~StyleAnalyser()
{
  thread->quit();
  if (thread->isRunning())
    thread->requestInterruption();
  thread->deleteLater();
}

void StyleAnalyser::onDocumentUpdated(QTextDocument *doc)
{
  if (doc) {
    lock.lock();
    if (!pending) {
      pending = true;
      lock.unlock(); // unlock before long op
      emit updateRequest(doc);
    } else {
      delayed = true;
      lock.unlock();
    }
  }
}

void StyleAnalyser::update(QTextDocument *doc)
{
  lock.lock();
  for(;;) {
    delayed = false; // starting analysis, I reset delayed
    lock.unlock();

    if (thread->isInterruptionRequested())
      break;

    osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);
    styleConfig->LoadContent(styleSheetFilePath.toStdString(), doc->toPlainText().toStdString());

    QSet<int> errorLines;
    QSet<int> warningLines;
    for (const auto &w: styleConfig->GetWarnings()) {
      warningLines << w.GetLine();
    }
    for (const auto &w: styleConfig->GetErrors()) {
      errorLines << w.GetLine();
    }

    emit problematicLines(errorLines, warningLines);

    lock.lock();
    if (delayed)
      continue;
    else
      break;
  }
  pending = false;
  lock.unlock();
}
