#ifndef OSMSCOUT_LOCATIONINDEX_H
#define OSMSCOUT_LOCATIONINDEX_H

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

#include <list>
#include <map>
#include <memory>
#include <set>
#include <unordered_set>

#include <osmscout/Location.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/ObjectPool.h>
#include <osmscout/util/FileScanner.h>

namespace osmscout {

  /**
   * \ingroup Database
   * Location index returns objects by names (the name should be changed). You
   * can currently either search for regions like 'cities' or for named locations in
   * areas like 'street in city'.
   *
   * Currently every type that has option 'INDEX' set in the map.ost file is indexed as
   * location. Areas are currently build by scanning administrative boundaries and the
   * various sized city typed locations and areas.
   */
  class OSMSCOUT_API LocationIndex
  {
  public:
    static const char* const FILENAME_LOCATION_IDX;

    /**
     * Util class that cleanup location index cache when instance is destructed.
     */
    class OSMSCOUT_API ScopeCacheCleaner CLASS_FINAL {
      std::shared_ptr<LocationIndex> index;
    public:
      explicit ScopeCacheCleaner(std::shared_ptr<LocationIndex> index):
        index(std::move(index))
      {}

      ScopeCacheCleaner(const ScopeCacheCleaner&) = delete;
      ScopeCacheCleaner(ScopeCacheCleaner&&) = delete;
      ScopeCacheCleaner& operator=(const ScopeCacheCleaner &) = delete;
      ScopeCacheCleaner& operator=(ScopeCacheCleaner &&) = delete;

      ~ScopeCacheCleaner() {
        if (index) {
          index->FlushCache();
        }
      }
    };

  private:

    /**
     * FileScanner opening may be expensive operation,
     * but LocationIndex may be recursive, because some Visit* method
     * may be called even from visitor. So it is not possible
     * use "global" scanner, because its position would be different
     * when returning from visitor. So, scanners are managed in
     * the ObjectPool that allows reusing the objects, and guarantee
     * exclusive access.
     *
     * To keep small LocationIndex memory footprint, user may call
     * FlushCache method when index is not needed anymore.
     */
    class FileScannerPool: public ObjectPool<FileScanner>
    {
    public:
      std::string path;
      bool memoryMappedData=false;
    public:
      FileScannerPool():
          ObjectPool<FileScanner>(4) // 4 should be enough for recursive visitors
      {}

      ~FileScannerPool() override {
        Clear(); // we have Close method override...
      }

      Ptr Borrow() override;

      FileScanner* MakeNew() noexcept override;

      void Destroy(FileScanner*) noexcept override;

      bool IsValid(FileScanner* o) noexcept override;
    };

    using FileScannerPtr = FileScannerPool::Ptr;

  private:
    mutable FileScannerPool         fileScannerPool;
    uint8_t                         bytesForNodeFileOffset;
    uint8_t                         bytesForAreaFileOffset;
    uint8_t                         bytesForWayFileOffset;
    std::vector<std::string>        regionIgnoreTokens;
    std::unordered_set<std::string> regionIgnoreTokenSet;
    std::vector<std::string>        poiIgnoreTokens;
    std::unordered_set<std::string> poiIgnoreTokenSet;
    std::vector<std::string>        locationIgnoreTokens;
    std::unordered_set<std::string> locationIgnoreTokenSet;
    uint32_t                        minRegionChars;
    uint32_t                        maxRegionChars;
    uint32_t                        minRegionWords;
    uint32_t                        maxRegionWords;
    uint32_t                        maxPOIWords;
    uint32_t                        minLocationChars;
    uint32_t                        maxLocationChars;
    uint32_t                        minLocationWords;
    uint32_t                        maxLocationWords;
    uint32_t                        maxAddressWords;
    FileOffset                      indexOffset;

  private:
    void Read(FileScanner& scanner,
              ObjectFileRef& object) const;

    bool LoadAdminRegion(FileScanner& scanner,
                         AdminRegion& region) const;

    AdminRegionVisitor::Action VisitRegionEntries(const AdminRegion& region,
                                                  FileScanner& scanner,
                                                  AdminRegionVisitor& visitor) const;

    bool VisitRegionPOIs(const AdminRegion& region,
                         FileScanner& scanner,
                         POIVisitor& visitor,
                         bool recursive,
                         bool& stopped) const;

    bool VisitPostalArea(const AdminRegion& adminRegion,
                         const PostalArea& postalArea,
                         FileScanner& scanner,
                         LocationVisitor& visitor,
                         bool recursive,
                         bool& stopped) const;

    bool VisitLocations(const AdminRegion& adminRegion,
                        FileScanner& scanner,
                        LocationVisitor& visitor,
                        bool recursive,
                        bool& stopped) const;

    bool VisitPostalAreaLocations(const AdminRegion& adminRegion,
                                  const PostalArea& postalArea,
                                  FileScanner& scanner,
                                  LocationVisitor& visitor,
                                  bool& stopped) const;

    bool VisitLocation(FileScanner& scanner,
                       const AdminRegion& region,
                       const PostalArea& postalArea,
                       const Location& location,
                       AddressVisitor& visitor,
                       bool& stopped) const;

  public:
    LocationIndex() = default;
    virtual ~LocationIndex() = default;

    bool Load(const std::string& path, bool memoryMappedData);

    const std::vector<std::string>& GetRegionIgnoreTokens() const
    {
      return regionIgnoreTokens;
    }

    const std::vector<std::string>& GetPOIIgnoreTokens() const
    {
      return poiIgnoreTokens;
    }

    const std::vector<std::string>& GetLocationIgnoreTokens() const
    {
      return locationIgnoreTokens;
    }

    bool IsRegionIgnoreToken(const std::string& token) const;
    bool IsLocationIgnoreToken(const std::string& token) const;

    inline uint32_t GetRegionMaxWords() const
    {
      return maxRegionWords;
    }

    inline uint32_t GetPOIMaxWords() const
    {
      return maxPOIWords;
    }

    inline uint32_t GetLocationMaxWords() const
    {
      return maxLocationWords;
    }

    inline uint32_t GetAddressMaxWords() const
    {
      return maxAddressWords;
    }

    /**
     * Visit all admin regions
     */
    bool VisitAdminRegions(AdminRegionVisitor& visitor) const;

    /**
     * Visit given admin region and all sub regions
     */
    bool VisitAdminRegions(const AdminRegion& adminRegion,
                           AdminRegionVisitor& visitor) const;

    /**
     * Visit all POIs within the given admin region
     */
    bool VisitPOIs(const AdminRegion& region,
                   POIVisitor& visitor,
                   bool recursive=true) const;

    /**
     * Visit all locations within the given admin region and its children
     */
    bool VisitLocations(const AdminRegion& adminRegion,
                        LocationVisitor& visitor,
                        bool recursive=true) const;

    /**
     * Visit all locations within the given admin region and postal region
     */
    bool VisitLocations(const AdminRegion& adminRegion,
                        const PostalArea& postalArea,
                        LocationVisitor& visitor,
                        bool recursive=true) const;

    /**
     * Visit all addresses for a given location (in a given AdminRegion)
     */
    bool VisitAddresses(const AdminRegion& region,
                        const PostalArea& postalArea,
                        const Location& location,
                        AddressVisitor& visitor) const;

    bool ResolveAdminRegionHierachie(const AdminRegionRef& region,
                                     std::map<FileOffset,AdminRegionRef>& refs) const;

    void DumpStatistics() const;

    void FlushCache() const;
  };

  using LocationIndexRef = std::shared_ptr<LocationIndex>;
}

#endif
