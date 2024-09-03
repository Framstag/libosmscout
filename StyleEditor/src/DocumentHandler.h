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

#ifndef DOCUMENTHANDLER_H
#define DOCUMENTHANDLER_H

#include <QFont>
#include <QObject>
#include <QTextCursor>
#include <QUrl>

#include <QQuickTextDocument>
#include <QTextDocument>

QT_BEGIN_NAMESPACE
class QTextDocument;
class QQuickTextDocument;
QT_END_NAMESPACE

Q_DECLARE_OPAQUE_POINTER(QTextDocument)
Q_DECLARE_OPAQUE_POINTER(QQuickTextDocument)

class Highlighter;

class DocumentHandler : public QObject
{
  Q_OBJECT

  Q_PROPERTY(QQuickTextDocument *document READ document WRITE setDocument NOTIFY documentChanged)
  Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged)
  Q_PROPERTY(int selectionStart READ selectionStart WRITE setSelectionStart NOTIFY selectionStartChanged)
  Q_PROPERTY(int selectionEnd READ selectionEnd WRITE setSelectionEnd NOTIFY selectionEndChanged)

  Q_PROPERTY(QString indentString READ indentString WRITE setIndentString NOTIFY indentStringChanged)
  Q_PROPERTY(bool styleAnalyserEnabled READ styleAnalyserEnabled WRITE setStyleAnalyserEnabled NOTIFY styleAnalyserEnabledChanged)

  Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
  Q_PROPERTY(QString fileName READ fileName NOTIFY sourceChanged)
  Q_PROPERTY(QString fileType READ fileType NOTIFY sourceChanged)

  Q_PROPERTY(bool modified READ modified WRITE setModified NOTIFY modifiedChanged)

public:
  explicit DocumentHandler(QObject *parent = nullptr);
  virtual ~DocumentHandler() = default;

  QQuickTextDocument *document() const;
  void setDocument(QQuickTextDocument *document);

  int cursorPosition() const;
  void setCursorPosition(int position);

  int selectionStart() const;
  void setSelectionStart(int position);

  int selectionEnd() const;
  void setSelectionEnd(int position);

  QString indentString() const;
  void setIndentString(const QString& str);

  bool styleAnalyserEnabled() const;
  void setStyleAnalyserEnabled(bool yesno);

  QString source() { return m_filePath; }
  void setSource(const QString& source) {
    if(m_filePath != source){
      m_filePath = source;
      emit sourceChanged(m_filePath);
    }
  }

  QString fileName() const;
  QString fileType() const;

  bool modified() const;
  void setModified(bool m);

  static QString tmpSuffix() { return QString(".tmp"); }

public Q_SLOTS:
  void load();
  bool save();
  bool saveTmp();

  /* Returns the new size of the selected block */
  int tabSelectedText();
  /* Returns the new size of the selected block */
  int backtabSelectedText();

  int positionNextPage(int pageSize);
  int positionPreviousPage(int pageSize);

Q_SIGNALS:
  void documentChanged();
  void cursorPositionChanged();
  void selectionStartChanged();
  void selectionEndChanged();
  void indentStringChanged();
  void styleAnalyserEnabledChanged();

  void textChanged();
  void sourceChanged(const QString& source);

  void loaded(const QString &text, int format);
  void saved();
  void error(const QString &message);

  void modifiedChanged();

private:
  bool save(const QString &fileName);
  QTextCursor textCursor() const;
  QTextDocument *textDocument() const;
  void mergeFormatOnWordOrSelection(const QTextCharFormat &format);

  QQuickTextDocument *m_document;

  int m_cursorPosition;
  int m_selectionStart;
  int m_selectionEnd;

  bool m_styleAnalyserEnabled{true};
  QString m_indentString;
  QString m_filePath;
  Highlighter *m_highlighter{nullptr};
};

#endif // DOCUMENTHANDLER_H
