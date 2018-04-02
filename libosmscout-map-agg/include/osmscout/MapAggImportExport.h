#ifndef OSMSCOUT_MAP_AGG_PRIVATE_IMPORT_EXPORT_H
#define OSMSCOUT_MAP_AGG_PRIVATE_IMPORT_EXPORT_H

/*
  This source is part of the libosmscout-map-svg library
  Copyright (C) 2011  Tim Teulings

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

#include <osmscout/MapAggFeatures.h>

// Shared library support
#if defined(_WIN32)
  #if defined(OSMSCOUT_MAP_AGG_EXPORT_SYMBOLS)
    #if defined(DLL_EXPORT) || defined(_WINDLL)
      #define OSMSCOUT_MAP_AGG_EXPTEMPL
      #define OSMSCOUT_MAP_AGG_API __declspec(dllexport)
    #else
      #define OSMSCOUT_MAP_AGG_API
    #endif
  #else
    #define OSMSCOUT_MAP_AGG_API __declspec(dllimport)
    #define OSMSCOUT_MAP_AGG_EXPTEMPL extern
  #endif

  #define OSMSCOUT_MAP_AGG_DLLLOCAL
#else
  #define OSMSCOUT_MAP_AGG_IMPORT
  #define OSMSCOUT_MAP_AGG_EXPTEMPL

  #if defined(OSMSCOUT_MAP_AGG_EXPORT_SYMBOLS)
    #define OSMSCOUT_MAP_AGG_EXPORT __attribute__ ((visibility("default")))
    #define OSMSCOUT_MAP_AGG_DLLLOCAL __attribute__ ((visibility("hidden")))
  #else
    #define OSMSCOUT_MAP_AGG_EXPORT
    #define OSMSCOUT_MAP_AGG_DLLLOCAL
  #endif

  #if defined(OSMSCOUT_MAP_AGG_EXPORT_SYMBOLS)
    #define OSMSCOUT_MAP_AGG_API OSMSCOUT_MAP_AGG_EXPORT
  #else
    #define OSMSCOUT_MAP_AGG_API OSMSCOUT_MAP_AGG_IMPORT
  #endif

#endif

// Throwable classes must always be visible on GCC in all binaries
#if defined(_WIN32)
  #define OSMSCOUT_MAP_AGG_EXCEPTIONAPI(api) api
#elif defined(OSMSCOUT_MAP_AGG_EXPORT_SYMBOLS)
  #define OSMSCOUT_MAP_AGG_EXCEPTIONAPI(api) OSMSCOUT_MAP_AGG_EXPORT
#else
  #define OSMSCOUT_MAP_AGG_EXCEPTIONAPI(api)
#endif

#if defined(_MSC_VER)
  #define OSMSCOUT_MAP_AGG_INSTANTIATE_TEMPLATES
#endif
#endif

