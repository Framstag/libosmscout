#ifndef OSMSCOUT_MAP_PATTERNLOOKUP_H
#define OSMSCOUT_MAP_PATTERNLOOKUP_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2026  Tim Teulings

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

#include <cstdint>
#include <list>
#include <string>

#include <osmscoutmap/MapImportExport.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * \ingroup Renderer
   *
   * Looks a pattern image up in the pattern image directories of a render parameter and
   * classifies the outcome, so that every map backend can report a pattern fill it cannot
   * serve in the same way.
   *
   * The lookup only answers where an image is and what should be reported if there is none;
   * loading the image stays with the backend, because every backend keeps its images in its
   * own type.
   */
  class OSMSCOUT_MAP_API PatternLookup CLASS_FINAL
  {
  public:
    /**
     * Outcome of searching the configured pattern image directories for a pattern.
     */
    enum class Status : uint8_t {
      Found,             //!< An image for the pattern was found in one of the configured directories
      NotFound,          //!< The configured directories hold no image for the pattern
      NoSourceConfigured //!< No pattern image directory is configured at all
    };

  public:
    PatternLookup() = delete;

    /**
     * Search the given directories for an image of the pattern.
     *
     * The directories are searched in their given order, so the first directory that holds the
     * image wins.
     *
     * @param patternPaths directories to search, in the order to search them
     * @param patternName pattern name as the style sheet uses it, without an extension
     * @param extension image file extension including the dot (for example ".png")
     * @param filename set to the file that holds the image, empty otherwise
     * @return the classification of the lookup
     */
    static Status Resolve(const std::list<std::string>& patternPaths,
                          const std::string& patternName,
                          const std::string& extension,
                          std::string& filename);

    /**
     * Report for a lookup that did not resolve the pattern: names the pattern and either says
     * that no pattern image source is configured or lists the directories that were searched.
     *
     * @param patternPaths directories the lookup searched
     * @param patternName pattern name as the style sheet uses it, without an extension
     * @param extension image file extension including the dot (for example ".png")
     * @param status outcome of the lookup
     * @return the report, empty for Status::Found
     */
    static std::string Describe(const std::list<std::string>& patternPaths,
                                const std::string& patternName,
                                const std::string& extension,
                                Status status);
  };
} // namespace osmscout

#endif
