/*
 This source is part of the libosmscout library
 Copyright (C) 2024  Jean-Luc Barriere

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <DocumentHandler.h>
#include <Highlighter.h>

#include <QFile>
#include <QFileInfo>
#include <QQuickTextDocument>
#include <QTextCharFormat>
#include <QTextCodec>
#include <QTextDocument>
#include <QDebug>

DocumentHandler::DocumentHandler(QObject *parent)
    : QObject(parent)
    , m_document(nullptr)
    , m_cursorPosition(-1)
    , m_selectionStart(0)
    , m_selectionEnd(0)
    , m_indentString('\t')
{
}

QQuickTextDocument *DocumentHandler::document() const
{
  return m_document;
}

void DocumentHandler::setDocument(QQuickTextDocument *document)
{
  if (document == m_document)
    return;

  if (m_document)
    m_document->textDocument()->disconnect(this);
  m_document = document;
  if (m_document)
    connect(m_document->textDocument(), &QTextDocument::modificationChanged, this, &DocumentHandler::modifiedChanged);
  emit documentChanged();
}

int DocumentHandler::cursorPosition() const
{
  return m_cursorPosition;
}

void DocumentHandler::setCursorPosition(int position)
{
  if (position == m_cursorPosition)
    return;

  m_cursorPosition = position;
  emit cursorPositionChanged();
}

int DocumentHandler::selectionStart() const
{
  return m_selectionStart;
}

void DocumentHandler::setSelectionStart(int position)
{
  if (position == m_selectionStart)
    return;

  m_selectionStart = position;
  emit selectionStartChanged();
}

int DocumentHandler::selectionEnd() const
{
  return m_selectionEnd;
}

void DocumentHandler::setSelectionEnd(int position)
{
  if (position == m_selectionEnd)
    return;

  m_selectionEnd = position;
  emit selectionEndChanged();
}

QString DocumentHandler::indentString() const
{
  return m_indentString;
}

void DocumentHandler::setIndentString(const QString &str)
{
  if (m_indentString == str)
    return;

  m_indentString = str;
  emit indentStringChanged();
}

bool DocumentHandler::styleAnalyserEnabled() const
{
  return m_styleAnalyserEnabled;
}

void DocumentHandler::setStyleAnalyserEnabled(bool yesno)
{
  if (m_styleAnalyserEnabled == yesno)
    return;
  m_styleAnalyserEnabled = yesno;
  emit styleAnalyserEnabledChanged();
  if (yesno)
    m_highlighter->startStyleAnalyser();
  else
    m_highlighter->stopStyleAnalyser();
  m_highlighter->setStyle();
}

QString DocumentHandler::fileName() const
{
  const QString fileName = QFileInfo(m_filePath).fileName();
  if (fileName.isEmpty())
    return QStringLiteral("untitled.oss");
  return fileName;
}

QString DocumentHandler::fileType() const
{
  return QFileInfo(fileName()).suffix();
}

void DocumentHandler::load()
{
  if (QFile::exists(m_filePath)) {
    QFile file(m_filePath);
    if (file.open(QFile::ReadOnly)) {
      QByteArray data = file.readAll();
      if (QTextDocument *doc = textDocument()) {
        doc->clear();

        if (!m_highlighter) {
          m_highlighter = new Highlighter(doc); // owned by doc (parent)
          if (m_styleAnalyserEnabled)
            m_highlighter->startStyleAnalyser();
        }

        m_highlighter->setStyle();

        QTextCodec *codec = QTextCodec::codecForUtfText(data);
        emit loaded(codec->toUnicode(data), Qt::PlainText);
        doc->setModified(false);
      }
    }
  }
}

bool DocumentHandler::save(const QString &fileName)
{
  QTextDocument *doc = textDocument();
  if (!doc)
    return false;

  QFile file(fileName);
  if (!file.open(QFile::WriteOnly | QFile::Truncate | QFile::Text)) {
    emit error("Cannot save: " + file.errorString());
    return false;
  }
  file.write(doc->toPlainText().toUtf8());
  file.close();
  doc->setModified(false);
  emit saved();
  return true;
}

bool DocumentHandler::save()
{
  if (m_filePath.isEmpty()){
    return false;
  }
  return save(m_filePath);
}

bool DocumentHandler::saveTmp()
{
  QTextDocument *doc = textDocument();
  if (!doc)
    return false;

  QFile file(m_filePath + tmpSuffix());
  if (!file.open(QFile::WriteOnly | QFile::Truncate | QFile::Text)) {
    emit error("Cannot save: " + file.errorString());
    return false;
  }
  file.write(doc->toPlainText().toUtf8());
  file.close();
  return true;
}

int DocumentHandler::tabSelectedText()
{
  QTextCursor cursor = textCursor();
  if (!cursor.hasSelection()) {
    return 0;
  }
  QString fragment = cursor.selectedText();
  int begin = 0;
  int end = fragment.length() - 1;
  const QChar nl(8233);
  while (begin < end) {
    fragment.insert(begin, m_indentString);
    end += m_indentString.length();
    begin = fragment.indexOf(nl, begin);
    begin = (begin < 0 ? end : begin + 1);
  }
  cursor.insertText(fragment);
  return end + 1;
}

int DocumentHandler::backtabSelectedText()
{
  QTextCursor cursor = textCursor();
  if (!cursor.hasSelection()) {
    return 0;
  }
  QString fragment = cursor.selectedText();
  int begin = 0;
  int end = fragment.length() - 1;
  const QChar nl(8233);
  while (begin < end) {
    if (fragment.mid(begin, m_indentString.length()) == m_indentString) {
      fragment.remove(begin, m_indentString.length());
      end -= m_indentString.length();
    } else {
      for (int i = 0; i < m_indentString.length(); ++i) {
        if (fragment.length() == 0 || fragment.mid(begin, 1) != m_indentString.mid(i, 1))
          break;
        fragment.remove(begin, 1);
        end -= 1;
      }
    }
    begin = fragment.indexOf(nl, begin);
    begin = (begin < 0 ? end : begin + 1);
  }
  cursor.insertText(fragment);
  return end + 1;
}

int DocumentHandler::positionNextPage(int pageSize)
{
  QTextCursor cursor = textCursor();
  for (int i = 0; i < pageSize; ++i) {
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor);
  }
  cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
  return cursor.position();
}

int DocumentHandler::positionPreviousPage(int pageSize)
{
  QTextCursor cursor = textCursor();
  for (int i = 0; i < pageSize; ++i) {
    cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor);
  }
  cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
  return cursor.position();
}

QTextCursor DocumentHandler::textCursor() const
{
  QTextDocument *doc = textDocument();
  if (!doc)
    return QTextCursor();

  QTextCursor cursor = QTextCursor(doc);
  if (m_selectionStart != m_selectionEnd) {
    cursor.setPosition(m_selectionStart);
    cursor.setPosition(m_selectionEnd, QTextCursor::KeepAnchor);
  } else {
    cursor.setPosition(m_cursorPosition);
  }
  return cursor;
}

QTextDocument *DocumentHandler::textDocument() const
{
  if (!m_document)
    return nullptr;

  return m_document->textDocument();
}

bool DocumentHandler::modified() const
{
  return m_document && m_document->textDocument()->isModified();
}

void DocumentHandler::setModified(bool m)
{
  if (m_document)
    m_document->textDocument()->setModified(m);
}
