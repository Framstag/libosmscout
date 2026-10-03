#ifndef OSMSCOUT_MAP_FONTNAMERESOLUTION_H
#define OSMSCOUT_MAP_FONTNAMERESOLUTION_H

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

#include <string>

#include <osmscoutmap/MapImportExport.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * \ingroup Renderer
   *
   * Answers what a configured font name names: a font family to look up, or the face that a font
   * file holds.
   *
   * A render parameter carries one font name, and a caller may configure a family name
   * ("Liberation Sans") or the path of a font file. A backend whose text stack resolves a font by
   * family has to serve the face the file holds and make that file resolvable to its stack, while a
   * backend that loads a font file directly wants the path. This is the one place that decision is
   * taken, so that every backend and both build systems agree on it.
   */
  class OSMSCOUT_MAP_API FontNameResolution CLASS_FINAL
  {
  public:
    /**
     * What a configured font name names.
     */
    struct Result {
      std::string fontName; //!< name to hand an interface that resolves a font by family
      std::string fontFile; //!< font file the name refers to, empty if the name names no file
    };

  public:
    FontNameResolution() = delete;

    /**
     * Whether this build can read the family name out of a font file.
     *
     * Without the ability to read a font file, a configured file is still recognised, but the
     * family it holds cannot be named.
     */
    static bool CanReadFamilyFromFile();

    /**
     * Read the family name stored in a font file.
     *
     * @param fontFile path to a font file
     * @param family set to the family name stored in the file on success
     * @param error set to a description of what went wrong on failure
     * @return true on success, false on failure
     */
    static bool ReadFamilyFromFile(const std::string& fontFile,
                                   std::string& family,
                                   std::string& error);

    /**
     * Resolve a configured font name to the face it names.
     *
     * A name that is an existing font file is read for the family it holds, so that an interface
     * resolving by family serves that face; the file is reported as well, so that a caller can make
     * it resolvable to its text stack. Any other name is passed on unchanged.
     *
     * @param configuredName font name as the render parameter carries it
     * @return the name to hand a family-resolving interface and the font file it names, if any
     */
    static Result Resolve(const std::string& configuredName);
  };
} // namespace osmscout

#endif
