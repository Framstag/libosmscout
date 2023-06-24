/*
  This source is part of the libosmscout library
  Copyright (C) 2015  Tim Teulings

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

#include <osmscout/log/Logger.h>

#include <osmscout/log/LoggerImpl.h>

#include <osmscout/system/Assert.h>

namespace osmscout {

  // Global logger
  Log log;

  Logger::Line::Line(Destination& destination)
  : destination(destination)
  {
    // no code
  }

  Logger::Line::~Line()
  {
    destination.PrintLn();
  }

  Logger::Line& Logger::Line::operator<<(float value)
  {
    std::stringstream strstream;

    strstream.precision(5);
    strstream << std::fixed;
    strstream << value;

    destination.Print(strstream.str());

    return *this;
  }

  Logger::Line& Logger::Line::operator<<(double value)
  {
    std::stringstream strstream;

    strstream.precision(5);
    strstream << std::fixed;
    strstream << value;

    destination.Print(strstream.str());

    return *this;
  }

  Logger::Line& Logger::Line::operator<<(void* value)
  {
    std::stringstream strstream;

    strstream << value;

    destination.Print(strstream.str());

    return *this;
  }

  Logger::Line Logger::Debug()
  {
    return Log(DEBUG);
  }

  Logger::Line Logger::Info()
  {
    return Log(INFO);
  }

  Logger::Line Logger::Warn()
  {
    return Log(WARN);
  }

  Logger::Line Logger::Error()
  {
    return Log(ERROR);
  }

  Log::Log()
  : logger(new ConsoleLogger())
  {
    Debug() << "Initializing Logging";
  }

  Log::~Log()
  {
    Debug() << "Deinitializing Logging";

    delete logger;
  }

  void Log::SetLogger(Logger* logger)
  {
    assert(logger!=nullptr);
    Logger* oldLogger=this->logger;

    this->logger=logger;

    delete oldLogger;
  }

  Logger::Line Log::Debug()
  {
    if (logDebug) {
      return logger->Debug();
    }

    return noOpLogger.Debug();
  }

  Logger::Line Log::Info()
  {
    if (logInfo) {
      return logger->Info();
    }

    return noOpLogger.Debug();
  }

  Logger::Line Log::Warn()
  {
    if (logWarn) {
      return logger->Warn();
    }

    return noOpLogger.Debug();
  }

  Logger::Line Log::Error()
  {
    if (logError) {
      return logger->Error();
    }

    return noOpLogger.Debug();
  }
}

