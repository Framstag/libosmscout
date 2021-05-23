#ifndef OSMSCOUT_UTIL_BREAKER_H
#define OSMSCOUT_UTIL_BREAKER_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/CoreFeatures.h>

#include <atomic>
#include <memory>

#include <osmscout/CoreImportExport.h>

namespace osmscout {

  /**
   * \ingroup Util
   *
   * A breaker object instance allows to trigger cancelation of long running processes.
   */
  class OSMSCOUT_API Breaker
  {
  public:
    virtual ~Breaker() = default;

    /**
     * Stop the processing. This is just a setting a flag that must actively get polled
     * by the long running process. So while the process was signaled to stop, it can still
     * continue for a while.
     *
     * @return
     */
    virtual void Break() = 0;

    /**
     * Return true, if the process was signaled to stop
     *
     * @return
     *    True, if signaled, else false
     */
    virtual bool IsAborted() const = 0;

    /**
     * Reset the state of the breaker.
     */
    virtual void Reset() = 0;
  };

  using BreakerRef = std::shared_ptr<Breaker>;

  class OSMSCOUT_API ThreadedBreaker : public Breaker
  {
  private:
    std::atomic<bool> aborted{false};

  public:
    ThreadedBreaker() = default;

    void Break() override;
    bool IsAborted() const override;
    void Reset() override;
  };
}

#endif
