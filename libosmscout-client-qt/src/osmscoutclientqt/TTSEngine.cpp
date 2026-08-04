/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2026 Lukas Karas

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

#include <osmscoutclientqt/TTSEngine.h>
#include <osmscoutclientqt/OSMScoutQt.h>

#include <QThread>

namespace osmscout {

TTSEngine::TTSEngine():
  QObject(nullptr), // no parent, we move the engine to its own thread
  thread(OSMScoutQt::GetInstance().makeThread("TTSEngine"))
{
  // the engine (with all its operations) lives in its own background thread
  moveToThread(thread);
  // clean the cache when the engine thread starts (runs on the engine thread)
  connect(thread, &QThread::started, this, &TTSEngine::cleanCache);
  thread->start();
}

void TTSEngine::cleanCache()
{
  // default implementation does nothing, may be overridden by a derived class
}

TTSEngine::~TTSEngine()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  // quit the event loop of our background thread. The QThread deletes itself
  // via the finished -> deleteLater connection setup in the constructor.
  if (thread != nullptr) {
    thread->quit();
  }
}

}

