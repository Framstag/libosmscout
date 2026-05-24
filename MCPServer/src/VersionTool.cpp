/*
  MCPServer - a demo program for libosmscout
  Copyright (C) 2025  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <string>

#include <osmscout/system/Compiler.h>

#include "VersionTool.h"

#if !defined(OSMSCOUT_LIBRARY_VERSION)
#define OSMSCOUT_LIBRARY_VERSION "unknown"
#endif

namespace osmscout::mcp {

  ToolResult HandleVersion(unsigned int id)
  {
    ToolResult result;

    result.body["jsonrpc"] = "2.0";
    result.body["id"] = id;
    result.body["result"]["structuredContent"]["version"] = OSMSCOUT_LIBRARY_VERSION;

    result.status = 200;

    return result;
  }

} // namespace osmscout::mcp