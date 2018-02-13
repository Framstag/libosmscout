#ifndef FILEIO_H
#define FILEIO_H

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

#include <QObject>
#include <QtQuick>
#include <QQuickItem>
#include <QVector>

#include "Highlighter.h"
#include "StyleAnalyser.h"

#define TMP_SUFFIX ".tmp"

class FileIO : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    explicit FileIO(QObject *parent = 0);
    ~FileIO();

    Q_INVOKABLE bool write();
    Q_INVOKABLE void read();
    Q_INVOKABLE bool writeTmp();
    Q_INVOKABLE int lineOffset(int l);

    /**
     * Check if content of file match with content of target
     * @return true if content in target was changed
     */
    Q_INVOKABLE bool isModified();

    QQuickItem *target() { return targetComponent; }
    void setTarget(QQuickItem *target);

    QString source() { return styleSheetFile; }
    void setSource(const QString& source) {
        if(styleSheetFile != source){
            styleSheetFile = source;
            emit sourceChanged(styleSheetFile);
        }
    }

public slots:

signals:
    void targetChanged();
    void sourceChanged(const QString& source);
    void error(const QString& msg);

private:
    void stopAnalyser();
    bool write(const QString &filename);
    QString getTargetContent();

    QString styleSheetFile;
    QQuickItem *targetComponent;
    QTextDocument *doc;
    Highlighter *highlighter;
    StyleAnalyser *styleAnalyser;
    QVector<int> lineOffsets;
};

#endif // FILEIO_H
