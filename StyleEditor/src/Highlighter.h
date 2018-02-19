#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

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

#include <QSyntaxHighlighter>

#include <QHash>
#include <QTextCharFormat>

#include <QtQuick/QQuickItem>

class QTextDocument;

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = 0);

    virtual ~Highlighter(){};

    void setStyle(qreal m_baseFontPointSize);

public slots:
    void onProblematicLines(QSet<int> errorLines, QSet<int> warningLines);

protected:
    void highlightBlock(const QString &text);

private:
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };

    void updateRules();

    QVector<HighlightingRule> highlightingRules;

    qreal m_baseFontPointSize;

    QRegExp commentStartExpression;
    QRegExp commentEndExpression;

    QTextCharFormat kwSectionFormat;
    QTextCharFormat kwFormat;
    QTextCharFormat kwGeomFormat;
    QTextCharFormat kwTYPEFormat;
    QTextCharFormat kwMAGFormat;
    QTextCharFormat kwGEOFormat;
    QTextCharFormat kwSIZEFormat;
    QTextCharFormat kwTEXTICONFormat;
    QTextCharFormat kwTUNNELBRIDGEFormat;
    QTextCharFormat kwONEWAYFormat;
    QTextCharFormat commentsFormat;
    QTextCharFormat multiLineCommentFormat;

    QTextCharFormat errorFormat;
    QTextCharFormat warningFormat;

    QSet<int> errorLines;
    QSet<int> warningLines;
};

#endif
