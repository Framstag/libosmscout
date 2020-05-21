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

#include <osmscout/util/MemoryMonitor.h>
#include <osmscout/util/Logger.h>

// For MemoryMonitor
#ifdef __linux__
#include <unistd.h>

#include <fstream>
#endif

#include <algorithm>

namespace osmscout {

  MemoryMonitor::MemoryMonitor()
  : quit(false),
    maxVMUsage(0.0),
    maxResidentSet(0.0),
    thread(&MemoryMonitor::BackgroundJob,this)
  {
    // no code
  }

  MemoryMonitor::~MemoryMonitor()
  {
    SignalStop();
    thread.join();
  }

  /**
   * The actual background thread, sleeping for 200ms.
   */
  void MemoryMonitor::BackgroundJob()
  {
    while (!quit) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));

      {
        std::lock_guard<std::mutex> lock(mutex);

        Measure();
      }
    }
  }

  void MemoryMonitor::Measure()
  {
    double currentVMUsage=0.0;
    double currentResidentSet=0.0;

#ifdef __linux__
    double vsize=0;
    double rss=0;
    {
      std::ifstream ifs("/proc/self/statm", std::ios_base::in);
      ifs.imbue(std::locale("C"));

      ifs >> vsize >> rss;

      if (ifs.fail()){
        log.Warn() << "Failed to measure memory usage";
      }
    }

    long pageSizeInByte=sysconf(_SC_PAGE_SIZE);

    currentVMUsage=vsize*pageSizeInByte;
    currentResidentSet=rss*pageSizeInByte;
#endif

    maxVMUsage=std::max(maxVMUsage,currentVMUsage);
    maxResidentSet=std::max(maxResidentSet,currentResidentSet);
  }

  /**
   * Sinal the backgound thread to stop.
   */
  void MemoryMonitor::SignalStop()
  {
    quit=true;
  }

  /**
   * Return the maximum measured memory usage. If there is no implementation
   * for your OS, both values return are 0.0.
   */
  void MemoryMonitor::GetMaxValue(double& vmUsage,
                                  double& residentSet)
  {
    std::lock_guard<std::mutex> lock(mutex);

    Measure();

    vmUsage=maxVMUsage;
    residentSet=maxResidentSet;
  }

  /**
   * Resets the internal values to 0.0.
   */
  void MemoryMonitor::Reset()
  {
    std::lock_guard<std::mutex> lock(mutex);

    maxVMUsage=0.0;
    maxResidentSet=0.0;
  }
}

