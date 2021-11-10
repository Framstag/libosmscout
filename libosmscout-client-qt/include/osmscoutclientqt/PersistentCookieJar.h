/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/**
 * Original source:
 *  http://code.qt.io/cgit/qt/qt.git/tree/tools/qml/qmlruntime.cpp?h=4.7#n438
 */

#ifndef PERSISTENTCOOKIEJAR_H
#define	PERSISTENTCOOKIEJAR_H

#include <QMutex>
#include <QNetworkCookieJar>
#include <QNetworkCookie>

#include <osmscoutclientqt/ClientQtImportExport.h>
#include <osmscoutclientqt/OSMScoutQt.h>

namespace osmscout {

class OSMSCOUT_CLIENT_QT_API PersistentCookieJar : public QNetworkCookieJar {
public:
    PersistentCookieJar(SettingsRef settings, QObject *parent = Q_NULLPTR) :
        QNetworkCookieJar(parent), settings(settings) { load(); }
    virtual ~PersistentCookieJar() { save(); }

    virtual QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const
    {
        QMutexLocker lock(&mutex);
        return QNetworkCookieJar::cookiesForUrl(url);
    }

    virtual bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)
    {
        QMutexLocker lock(&mutex);
        return QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
    }

private:
    void save()
    {
        QMutexLocker lock(&mutex);
        QList<QNetworkCookie> list = allCookies();
        QByteArray data;
        foreach (QNetworkCookie cookie, list) {
            if (!cookie.isSessionCookie()) {
                data.append(cookie.toRawForm());
                data.append("\n");
            }
        }

        settings->SetCookieData(data);
    }

    void load()
    {
        QMutexLocker lock(&mutex);
        const QByteArray data = settings->GetCookieData();
        setAllCookies(QNetworkCookie::parseCookies(data));
    }

    mutable QMutex mutex;
    SettingsRef settings;
};

}

#endif	/* PERSISTENTCOOKIEJAR_H */
