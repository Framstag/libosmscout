#ifndef OSMSCOUT_UTIL_MEMORYMONITOR_H
#define OSMSCOUT_UTIL_MEMORYMONITOR_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2016 Tim Teulings

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

#include <atomic>
#include <mutex>
#include <thread>

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * Starts a background threads that checks every 200ms the current memory usage of the current
   * process and accumulates it to return the maximum memory usage since start or reset.
   *
   * Thread is started on construction and join again on destruction (with up to 200ms wait time).
   *
   * Implementation is OS specific, if GetValue() retutns 0.0 for each value there is likely no
   * implementation for your OS.
   */
  class OSMSCOUT_API MemoryMonitor CLASS_FINAL
  {
  private:
    std::atomic<bool> quit;
    std::mutex        mutex;
    double            maxVMUsage;
    double            maxResidentSet;
    std::thread       thread;

  private:
    void SignalStop();
    void BackgroundJob();
    void Measure();

  public:
    MemoryMonitor();
    ~MemoryMonitor();

    void GetMaxValue(double& vmUsage,
                     double& residentSet);

    void Reset();
  };

}

#endif
