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

#include "Highlighter.h"
#include <QtGui>

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{

}

void Highlighter::updateRules()
{
    HighlightingRule rule;

    highlightingRules.clear();

    kwSectionFormat.setFontWeight(QFont::Bold);
    kwSectionFormat.setForeground(QColor("#3000ff"));
    rule.pattern = QRegExp("OSS|ORDER\\sWAYS|CONST|END");
    rule.pattern.setMinimal(true);
    rule.format = kwSectionFormat;
    highlightingRules.append(rule);

    kwFormat.setFontWeight(QFont::Bold);
    kwFormat.setForeground(QColor("#7070ff"));
    rule.pattern = QRegExp("GROUP|COLOR|SYMBOL");
    rule.pattern.setMinimal(true);
    rule.format = kwFormat;
    highlightingRules.append(rule);

    kwGeomFormat.setFontWeight(QFont::Bold);
    kwGeomFormat.setForeground(QColor("#aa70ff"));
    rule.pattern = QRegExp("POLYGON|CIRCLE|RECTANGLE");
    rule.pattern.setMinimal(true);
    rule.format = kwGeomFormat;
    highlightingRules.append(rule);

    kwTYPEFormat.setFontWeight(QFont::Bold);
    kwTYPEFormat.setForeground(QColor("#994000"));
    rule.pattern = QRegExp("TYPE");
    rule.pattern.setMinimal(true);
    rule.format = kwTYPEFormat;
    highlightingRules.append(rule);

    kwMAGFormat.setFontWeight(QFont::Bold);
    kwMAGFormat.setForeground(QColor("#ff0050"));
    rule.pattern = QRegExp("MAG");
    rule.pattern.setMinimal(true);
    rule.format = kwMAGFormat;
    highlightingRules.append(rule);

    kwGEOFormat.setFontWeight(QFont::Bold);
    kwGEOFormat.setForeground(QColor("#ff5050"));
    rule.pattern = QRegExp("(NODE|WAY|AREA)[^A-Za-z]");
    rule.pattern.setMinimal(true);
    rule.format = kwGEOFormat;
    highlightingRules.append(rule);

    kwSIZEFormat.setFontWeight(QFont::Bold);
    kwSIZEFormat.setForeground(QColor("#ff50ff"));
    rule.pattern = QRegExp("SIZE");
    rule.pattern.setMinimal(true);
    rule.format = kwSIZEFormat;
    highlightingRules.append(rule);

    kwTEXTICONFormat.setFontWeight(QFont::Bold);
    kwTEXTICONFormat.setForeground(QColor("#ffb33d"));
    rule.pattern = QRegExp("TEXT|ICON");
    rule.pattern.setMinimal(true);
    rule.format = kwTEXTICONFormat;
    highlightingRules.append(rule);

    kwTUNNELBRIDGEFormat.setFontWeight(QFont::Bold);
    kwTUNNELBRIDGEFormat.setForeground(QColor("#25a13b"));
    rule.pattern = QRegExp("FEATURE");
    rule.pattern.setMinimal(true);
    rule.format = kwTUNNELBRIDGEFormat;
    highlightingRules.append(rule);

    kwONEWAYFormat.setFontWeight(QFont::Bold);
    kwONEWAYFormat.setForeground(QColor("#50ff50"));
    rule.pattern = QRegExp("ONEWAY");
    rule.pattern.setMinimal(true);
    rule.format = kwONEWAYFormat;
    highlightingRules.append(rule);

    commentsFormat.setFontItalic(true);
    commentsFormat.setForeground(QColor("#309030"));
    rule.pattern = QRegExp("//.*");
    rule.format = commentsFormat;
    highlightingRules.append(rule);

    multiLineCommentFormat.setFontItalic(true);
    multiLineCommentFormat.setForeground(QColor("#309030"));
}

void Highlighter::highlightBlock(const QString &text)
{
    if (m_baseFontPointSize == 0.0)
            return;

    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }

    QRegExp commentStartExpression = QRegExp("/\\*");
    QRegExp commentEndExpression = QRegExp("\\*/");
    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = commentStartExpression.indexIn(text);
    while (startIndex >= 0) {
        int endIndex = commentEndExpression.indexIn(text, startIndex);
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex
                    + commentEndExpression.matchedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
    }
}

void Highlighter::setStyle(qreal baseFontPointSize)
{
    m_baseFontPointSize = baseFontPointSize;
    this->updateRules();
    this->rehighlight();
}
