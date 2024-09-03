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

#include <Highlighter.h>
#include <StyleAnalyser.h>
#include <osmscoutclientqt/OSMScoutQt.h>

#include <QtGui>

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
  connect(parent, SIGNAL(contentsChange(int,int,int)),
          this, SLOT(onContentsChange(int,int,int)));
}

Highlighter::~Highlighter()
{
  stopStyleAnalyser();
}

void Highlighter::updateRules()
{
  HighlightingRule rule;

  highlightingRules.clear();

  rule.pattern = QRegExp("OSS|FLAG|ORDER\\sWAYS|CONST|STYLE|END");
  rule.pattern.setMinimal(true);
  rule.format = QTextCharFormat();
  rule.format.setFontWeight(QFont::Bold);
  rule.format.setForeground(QColor("#3000ff"));
  highlightingRules.append(rule);

  rule.pattern = QRegExp("GROUP|COLOR|SYMBOL|UINT|WIDTH");
  rule.pattern.setMinimal(true);
  rule.format = QTextCharFormat();
  rule.format.setFontWeight(QFont::Bold);
  rule.format.setForeground(QColor("#7070ff"));
  highlightingRules.append(rule);

  rule.pattern = QRegExp("POLYGON|CIRCLE|RECTANGLE");
  rule.pattern.setMinimal(true);
  rule.format = QTextCharFormat();
  rule.format.setFontWeight(QFont::Bold);
  rule.format.setForeground(QColor("#aa70ff"));
  highlightingRules.append(rule);

  rule.pattern = QRegExp("TYPE");
  rule.pattern.setMinimal(true);
  rule.format = QTextCharFormat();
  rule.format.setFontWeight(QFont::Bold);
  rule.format.setForeground(QColor("#994000"));
  highlightingRules.append(rule);

  rule.pattern = QRegExp("MAG");
  rule.pattern.setMinimal(true);
  rule.format = QTextCharFormat();
  rule.format.setFontWeight(QFont::Bold);
  rule.format.setForeground(QColor("#ff0050"));
  highlightingRules.append(rule);

  rule.pattern = QRegExp("(NODE|WAY|AREA)[^A-Za-z]");
  rule.pattern.setMinimal(true);
  rule.format = QTextCharFormat();
  rule.format.setFontWeight(QFont::Bold);
  rule.format.setForeground(QColor("#ff5050"));
  highlightingRules.append(rule);

  rule.pattern = QRegExp("SIZE");
  rule.pattern.setMinimal(true);
  rule.format = QTextCharFormat();
  rule.format.setFontWeight(QFont::Bold);
  rule.format.setForeground(QColor("#ff50ff"));
  highlightingRules.append(rule);

  rule.pattern = QRegExp("TEXT|ICON");
  rule.pattern.setMinimal(true);
  rule.format = QTextCharFormat();
  rule.format.setFontWeight(QFont::Bold);
  rule.format.setForeground(QColor("#ffb33d"));
  highlightingRules.append(rule);

  rule.pattern = QRegExp("FEATURE");
  rule.pattern.setMinimal(true);
  rule.format = QTextCharFormat();
  rule.format.setFontWeight(QFont::Bold);
  rule.format.setForeground(QColor("#25a13b"));
  highlightingRules.append(rule);

  rule.pattern = QRegExp("ONEWAY");
  rule.pattern.setMinimal(true);
  rule.format = QTextCharFormat();
  rule.format.setFontWeight(QFont::Bold);
  rule.format.setForeground(QColor("#2ce838"));
  highlightingRules.append(rule);

  rule.pattern = QRegExp("MODULE");
  rule.pattern.setMinimal(true);
  rule.format = QTextCharFormat();
  rule.format.setFontWeight(QFont::Bold);
  rule.format.setForeground(QColor("#820761"));
  highlightingRules.append(rule);

  commentFormat.setFontItalic(true);
  commentFormat.setForeground(QColor("#309030"));
  rule.pattern = QRegExp("//.*");
  rule.format = commentFormat;
  highlightingRules.append(rule);

  errorFormat.setBackground(QBrush(QColor(255, 0, 0, 30), Qt::SolidPattern));
  warningFormat.setBackground(QBrush(QColor(255, 255, 0, 50), Qt::SolidPattern));
}

void Highlighter::highlightBlock(const QString &text)
{
  int line = currentBlock().firstLineNumber()+1;
  if (currentBlock().lineCount() == 1){
    if (errorLines.contains(line)) {
      setFormat(0, text.size(), errorFormat);
    }
    if (warningLines.contains(line)) {
      setFormat(0, text.size(), warningFormat);
    }
  }

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

  if (previousBlockState() != 1) {
    startIndex = commentStartExpression.indexIn(text);
  }

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
    setFormat(startIndex, commentLength, commentFormat);
    startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
  }
}

void Highlighter::setStyle()
{
  warningLines.clear();
  errorLines.clear();
  this->updateRules();
  this->rehighlight();
}

void Highlighter::startStyleAnalyser()
{
  if (styleAnalyser)
    return;

  styleAnalyser = new StyleAnalyser("StyleAnalyser");

  connect(this, SIGNAL(documentUpdated(QTextDocument*)),
          styleAnalyser, SLOT(onDocumentUpdated(QTextDocument*)));
  connect(styleAnalyser, SIGNAL(problematicLines(QSet<int>,QSet<int>)),
          this, SLOT(onProblematicLines(QSet<int>,QSet<int>)),
          Qt::QueuedConnection);

  styleAnalyser->onDocumentUpdated(document());
}

void Highlighter::stopStyleAnalyser()
{
  if (!styleAnalyser)
    return;

  disconnect(this, SIGNAL(documentUpdated(QTextDocument*)),
             styleAnalyser, SLOT(onDocumentUpdated(QTextDocument*)));
  disconnect(styleAnalyser, SIGNAL(problematicLines(QSet<int>,QSet<int>)),
             this, SLOT(onProblematicLines(QSet<int>,QSet<int>)));

  styleAnalyser->deleteLater();
  styleAnalyser = nullptr;
}

void Highlighter::onContentsChange([[maybe_unused]] int position, int removed, int added)
{
  if (added != removed) {
    emit documentUpdated(document());
  }
}

void Highlighter::onProblematicLines(QSet<int> errorLines, QSet<int> warningLines)
{
  if (this->errorLines==errorLines &&
      this->warningLines==warningLines){
    return;
  }
  QSet<int> changedLines = this->errorLines + errorLines +
                           this->warningLines + warningLines;

  this->errorLines=errorLines;
  this->warningLines=warningLines;

  if (document() != nullptr) {
    for (int line: changedLines) {
      QTextBlock block = document()->findBlockByLineNumber(line - 1);
      rehighlightBlock(block);
    }
  }
}
