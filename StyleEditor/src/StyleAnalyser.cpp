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

#include "StyleAnalyser.h"

#include <osmscout/DBThread.h>
#include <osmscout/OSMScoutQt.h>

osmscout::TypeConfigRef getTypeConfig()
{
  osmscout::TypeConfigRef typeConfig;
  OSMScoutQt::GetInstance().GetDBThread()->RunSynchronousJob(
      [&typeConfig](const std::list<DBInstanceRef>& databases) {
        for (auto &db:databases) {
          if (db->database && db->database->IsOpen() && db->database->GetTypeConfig()){
            typeConfig = db->database->GetTypeConfig();
            return;
          }
        }
      }
  );
  return typeConfig;
}


StyleAnalyser::StyleAnalyser(QThread *thread,
                             QTextDocument *doc,
                             Highlighter *highlighter):
    thread(thread), typeConfig(getTypeConfig()), doc(doc), highlighter(highlighter)
{
  if (typeConfig) {
    connect(doc, SIGNAL(contentsChanged()),
            this, SLOT(onContentsChanged()));

    connect(this, SIGNAL(updateRequest(QString)),
            this, SLOT(update(QString)),
            Qt::QueuedConnection);

    connect(this, SIGNAL(problematicLines(QSet<int>, QSet<int>)),
            highlighter, SLOT(onProblematicLines(QSet<int>, QSet<int>)),
            Qt::QueuedConnection);

    onContentsChanged();
  }
}

StyleAnalyser::~StyleAnalyser()
{
  thread->quit();
}

void StyleAnalyser::onContentsChanged()
{
  QString content = doc->toPlainText();
  if (content == lastContent) {
    return;
  }
  lastContent = content;
  emit updateRequest(content);
}

void StyleAnalyser::update(QString content)
{
  osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);
  styleConfig->LoadContent(content.toStdString());

  QSet<int> errorLines;
  QSet<int> warningLines;
  int line;
  for (auto &w: styleConfig->GetWarnings()) {
    // TODO: expose warning as some structure with line number
    osmscout::StringToNumberSigned(w, line);
    warningLines << line;
  }
  for (auto &w: styleConfig->GetErrors()) {
    // TODO: expose warning as some structure with line number
    osmscout::StringToNumberSigned(w, line);
    errorLines << line;
  }

  emit problematicLines(errorLines, warningLines);
}
