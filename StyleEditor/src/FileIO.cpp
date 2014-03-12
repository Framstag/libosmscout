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
#include <QFile>
#include <QTextStream>

FileIO::FileIO(QObject *parent) :
    QObject(parent),m_source(QString("")),m_target(0)
{

}

void FileIO::read()
{
    if (m_source.isEmpty()){
        emit error("source is empty");
        return;
    } else if (m_target == 0){
        emit error("target is not set");
        return;
    }

    QFile file(m_source);
    if ( file.open(QIODevice::ReadOnly) ) {
        QString line;
        QTextStream t( &file );
        QVariant length = m_target->property("length");
        QVariant returnedValue;
        bool ret;
        if(length>0){
            ret = QMetaObject::invokeMethod(m_target, "remove",
                                            Q_RETURN_ARG(QVariant, returnedValue),
                                            Q_ARG(QVariant, QVariant(0)),
                                            Q_ARG(QVariant, length));
        }
        do {
            line = t.readLine();
            line.replace("<","&lt;").replace(">","&gt;").replace(" ","&nbsp;").replace("\t","        ");
            ret = QMetaObject::invokeMethod(m_target, "append",
                                            Q_RETURN_ARG(QVariant, returnedValue),
                                            Q_ARG(QVariant, QVariant(line)));
        } while (!t.atEnd());

        file.close();
        QVariant doc = m_target->property("textDocument");
        if (doc.canConvert<QQuickTextDocument*>()) {
            QQuickTextDocument *qqdoc = doc.value<QQuickTextDocument*>();
            if (qqdoc)
                m_doc = qqdoc->textDocument();
            m_highlighter = new Highlighter(m_doc);
            m_highlighter->setStyle(12);
        }
    } else {
        emit error("Unable to open the file");
        return;
    }
}

bool FileIO::write(const QString &fileName)
{
    if (m_target == 0){
        return false;
    }

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        return false;

    QVariant length = m_target->property("length");
    QVariant returnedValue;
    bool ret = QMetaObject::invokeMethod(m_target, "getText",
                                         Q_RETURN_ARG(QVariant, returnedValue),
                                         Q_ARG(QVariant, QVariant(0)),
                                         Q_ARG(QVariant, length));
    QString text = returnedValue.toString();
    text.replace(QChar(0x2029),"\n").replace(QChar(0x00a0)," ");
    QTextStream out(&file);
    out << text;

    file.close();

    return true;
}

bool FileIO::write()
{
    if (m_source.isEmpty()){
        return false;
    }
    return write(m_source);
}

bool FileIO::writeTmp()
{
    if (m_source.isEmpty()){
        return false;
    }
    return write(m_source+TMP_SUFFIX);
}

void FileIO::setTarget(QQuickItem *target)
{
    m_doc = 0;
    m_highlighter = 0;
    if(m_target == target){
        return;
    }
    m_target = target;
    if (!m_target || m_source.isEmpty())
        return;

    emit targetChanged();
}
