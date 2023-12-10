#ifndef OSMSCOUT_CLIENT_POILOOKUPSERVICE_H
#define OSMSCOUT_CLIENT_POILOOKUPSERVICE_H
/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2018 Lukas Karas

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

#include <osmscoutclient/ClientImportExport.h>

#include <osmscoutclient/DBThread.h>
#include <osmscoutclient/LocationInfo.h>

namespace osmscout {

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_API POILookupModule: public AsyncWorker
{
private:
  DBThreadRef      dbThread;

public:

  using LookupResult = std::vector<LocationInfo>;
  using LookupFuture = CancelableFuture<LookupResult>;

  Signal<int> lookupAborted; //<! requestId was aborted
  Signal<int> lookupFinished; //<! requestId was finished
  Signal<int, LookupResult> lookupResult; //<! partial result for requestId

public:
  explicit POILookupModule(DBThreadRef dbThread);

  POILookupModule(const POILookupModule&) = delete;
  POILookupModule(POILookupModule&&) = delete;
  POILookupModule& operator=(const POILookupModule&) = delete;
  POILookupModule& operator=(POILookupModule&&) = delete;

  ~POILookupModule() override;

  /** Lookup POI around some point, to maximum distance by given types.
   * It returns future with complete result and emits lookupResult signal
   * with partial results.
   */
  LookupFuture lookupPOIRequest(int requestId,
                                const GeoCoord &searchCenter,
                                const std::vector<std::string> &types,
                                const Distance &maxDistance);

private:
  LookupResult doPOIlookup(DBInstanceRef db,
                           const GeoBox &searchBoundingBox,
                           const std::vector<std::string> &types);

};

}

#endif //OSMSCOUT_CLIENT_POILOOKUPSERVICE_H
