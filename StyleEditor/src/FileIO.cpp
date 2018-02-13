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

#include <osmscout/OSMScoutQt.h>

#include <QFile>
#include <QTextStream>

FileIO::FileIO(QObject *parent) :
    QObject(parent),styleSheetFile(QString("")),targetComponent(0),
    highlighter(nullptr), styleAnalyser(nullptr)
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
    } else if (targetComponent == 0){
        emit error("target is not set");
        return;
    }

    QFile file(styleSheetFile);
    if ( file.open(QIODevice::ReadOnly) ) {
        QString line;
        QTextStream t( &file );
        QVariant length = targetComponent->property("length");
        QVariant returnedValue;

        if(length>0){
            QMetaObject::invokeMethod(targetComponent, "remove",
                                      Q_RETURN_ARG(QVariant, returnedValue),
                                      Q_ARG(QVariant, QVariant(0)),
                                      Q_ARG(QVariant, length));
            lineOffsets.clear();
            lineOffsets.push_back(1);
        }
        do {
            line = t.readLine();
            line.replace("\t","        ");
            lineOffsets.push_back(line.length()+1);
            line.replace("<","&lt;").replace(">","&gt;").replace(" ","&nbsp;");
            QMetaObject::invokeMethod(targetComponent, "append",
                                      Q_RETURN_ARG(QVariant, returnedValue),
                                      Q_ARG(QVariant, QVariant(line)));
        } while (!t.atEnd());

        file.close();
        QVariant docComponent = targetComponent->property("textDocument");
        if (docComponent.canConvert<QQuickTextDocument*>()) {
            QQuickTextDocument *qqdoc = docComponent.value<QQuickTextDocument*>();
            if (qqdoc) {
              doc = qqdoc->textDocument();
            }

            stopAnalyser();
            highlighter = new Highlighter(doc);
            highlighter->setStyle(12);

            QThread *analyserThread = OSMScoutQt::GetInstance().makeThread("StyleAnalyser");
            styleAnalyser = new StyleAnalyser(analyserThread, doc, highlighter);
            styleAnalyser->moveToThread(analyserThread);
            analyserThread->start();
        }
        if (doc) {
            doc->setModified(false);
        }
    } else {
        emit error("Unable to open the file");
        return;
    }
}

QString FileIO::getTargetContent()
{
    QVariant length = targetComponent->property("length");
    QVariant returnedValue;
    QMetaObject::invokeMethod(targetComponent, "getText",
                              Q_RETURN_ARG(QVariant, returnedValue),
                              Q_ARG(QVariant, QVariant(0)),
                              Q_ARG(QVariant, length));
    QString text = returnedValue.toString();
    text.replace(QChar(0x2029),"\n").replace(QChar(0x00a0)," ");
    return text;
}

bool FileIO::isModified()
{
    return doc && doc->isModified();
}

bool FileIO::write(const QString &fileName)
{
    if (targetComponent == 0){
        return false;
    }

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        return false;

    QString text=getTargetContent();
    QTextStream out(&file);
    out << text;

    file.close();

    if (doc) {
        doc->setModified(false);
    }
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
    doc = 0;
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
    for(int i=0; i<std::min(l, lineOffsets.count()); i++){
        s += lineOffsets[i];
    }
    return s;
}
