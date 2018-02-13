#ifndef LIBOSMSCOUT_STYLEANALYSER_H
#define LIBOSMSCOUT_STYLEANALYSER_H

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

#include "Highlighter.h"

#include <osmscout/TypeConfig.h>

#include <QObject>
#include <QSet>
#include <QTextDocument>

class StyleAnalyser : public QObject
{
  Q_OBJECT

signals:
  void problematicLines(QSet<int> errorLines, QSet<int> warningLines);
  void updateRequest(QString content);

public slots:
  void onContentsChanged();

private slots:
  void update(QString content);

public:
  StyleAnalyser(QThread *thread,
                   QTextDocument *doc,
                   Highlighter *highlighter);
  virtual ~StyleAnalyser();

private:
  QThread *thread;
  osmscout::TypeConfigRef typeConfig;
  QTextDocument *doc;
  Highlighter *highlighter;
  QString lastContent;
};



#endif //LIBOSMSCOUT_STYLEANALYSER_H
