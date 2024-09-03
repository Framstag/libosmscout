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
#include <QRegExp>
#include <QtQuick/QQuickItem>

class QTextDocument;
class StyleAnalyser;

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = nullptr);

    ~Highlighter() override;

    void setStyle();
    void startStyleAnalyser();
    void stopStyleAnalyser();

signals:
    void documentUpdated(QTextDocument *doc);

public slots:
    void onProblematicLines(QSet<int> errorLines, QSet<int> warningLines);

private slots:
    void onContentsChange(int position, int removed, int added);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };

    void updateRules();

    QVector<HighlightingRule> highlightingRules;
    QTextCharFormat commentFormat;

    StyleAnalyser *styleAnalyser{nullptr};
    QTextCharFormat errorFormat;
    QTextCharFormat warningFormat;
    QSet<int> errorLines;
    QSet<int> warningLines;
};

#endif
