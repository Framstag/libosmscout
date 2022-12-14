/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010  Tim Teulings

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

#include "FileIO.h"

#include <osmscoutclientqt/OSMScoutQt.h>

#include <QFile>
#include <QTextStream>

using namespace osmscout;

FileIO::FileIO(QObject *parent) :
    QObject(parent)
{
}

FileIO::~FileIO()
{
  stopAnalyser();
}

void FileIO::stopAnalyser()
{
  if (styleAnalyser){
    styleAnalyser->deleteLater();
    styleAnalyser=nullptr;
  }
}

void FileIO::read()
{
    if (styleSheetFile.isEmpty()){
        emit error("source is empty");
        return;
    } else if (targetComponent == nullptr){
        emit error("target is not set");
        return;
    }

    QFile file(styleSheetFile);
    if ( file.open(QIODevice::ReadOnly) ) {
        QString line;
        QTextStream t( &file );

        lineOffsets.clear();
        lineOffsets.push_back(1);

        QVariant docComponent = targetComponent->property("textDocument");
        if (docComponent.canConvert<QQuickTextDocument*>()) {
          if (QQuickTextDocument *qqdoc = docComponent.value<QQuickTextDocument *>();
              qqdoc) {
            doc = qqdoc->textDocument();
          }
        }
        if (!doc){
          emit error("document is not set");
          return;
        }

        QString content;
        content.reserve(file.size()*2);
        do {
            line = t.readLine();
            line.replace("\t","        "); // TODO: it is necessary?
            lineOffsets.push_back(line.length()+1);
            content += line + "\n";
        } while (!t.atEnd());

        doc->setPlainText(content);

        file.close();

        if (!styleAnalyser) {
          Highlighter *highlighter = new Highlighter(doc); // owned by doc (parent)
          highlighter->setStyle(12);

          QThread *analyserThread = OSMScoutQt::GetInstance().makeThread("StyleAnalyser");
          styleAnalyser = new StyleAnalyser(analyserThread, doc, *highlighter);
          styleAnalyser->moveToThread(analyserThread);
          analyserThread->start();
        }

        doc->setModified(false);
    } else {
        emit error("Unable to open the file");
        return;
    }
}

bool FileIO::isModified()
{
    return doc && doc->isModified();
}

bool FileIO::write(const QString &fileName)
{
    if (!doc){
        return false;
    }

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        return false;
    }

    QString text=doc->toPlainText();
    QTextStream out(&file);
    out << text;

    file.close();

    doc->setModified(false);

    return true;
}

bool FileIO::write()
{
    if (styleSheetFile.isEmpty()){
        return false;
    }
    return write(styleSheetFile);
}

bool FileIO::writeTmp()
{
    if (styleSheetFile.isEmpty()){
        return false;
    }
    return write(styleSheetFile+TMP_SUFFIX);
}

void FileIO::setTarget(QQuickItem *target)
{
    doc = nullptr;
    stopAnalyser();
    if(targetComponent == target){
        return;
    }
    targetComponent = target;
    if (!targetComponent || styleSheetFile.isEmpty())
        return;

    emit targetChanged();
}

int FileIO::lineOffset(int l)
{
    int s = 0;
    for(qsizetype i=0; i<std::min(qsizetype(l), qsizetype(lineOffsets.count())); i++){
        s += lineOffsets[i];
    }
    return s;
}
