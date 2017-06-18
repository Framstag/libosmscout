/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Tim Teulings

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

#include <osmscout/util/CmdLineParsing.h>

#include <cstdlib>
#include <iostream>

#include <osmscout/util/String.h>

namespace osmscout {

  bool ParseBoolArgument(int argc,
                         char* argv[],
                         int& currentIndex,
                         bool& value)
  {
    int parameterIndex=currentIndex;
    int argumentIndex=currentIndex+1;

    currentIndex+=2;

    if (argumentIndex>=argc) {
      std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
      return false;
    }

    if (!StringToBool(argv[argumentIndex],
                      value)) {
      std::cerr << "Cannot parse argument for parameter '" << argv[parameterIndex] << "'" << std::endl;
      return false;
    }

    return true;
  }

  bool ParseStringArgument(int argc,
                           char* argv[],
                           int& currentIndex,
                           std::string& value)
  {
    int parameterIndex=currentIndex;
    int argumentIndex=currentIndex+1;

    currentIndex+=2;

    if (argumentIndex>=argc) {
      std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
      return false;
    }

    value=argv[argumentIndex];

    return true;
  }

  bool ParseSizeTArgument(int argc,
                          char* argv[],
                          int& currentIndex,
                          size_t& value)
  {
    int parameterIndex=currentIndex;
    int argumentIndex=currentIndex+1;

    currentIndex+=2;

    if (argumentIndex>=argc) {
      std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
      return false;
    }

    if (!StringToNumber(argv[argumentIndex],
                        value)) {
      std::cerr << "Cannot parse argument for parameter '" << argv[parameterIndex] << "'" << std::endl;
      return false;
    }

    return true;
  }
}
