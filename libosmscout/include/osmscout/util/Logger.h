#ifndef OSMSCOUT_UTIL_LOGGER_H
#define OSMSCOUT_UTIL_LOGGER_H

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

#include <osmscout/CoreFeatures.h>

#include <osmscout/private/CoreImportExport.h>

#include <ostream>
#include <sstream>

#include <osmscout/util/StopClock.h>

namespace osmscout {

  class OSMSCOUT_API Logger
  {
  public:
    enum Level {
      DEBUG,
      INFO,
      WARN,
      ERROR
    };

    class OSMSCOUT_API Destination
    {
    public:
      virtual ~Destination();

      virtual void Print(const std::string& value) = 0;
      virtual void Print(const char* value) = 0;
      virtual void Print(bool value) = 0;
      virtual void Print(short value) = 0;
      virtual void Print(unsigned short value) = 0;
      virtual void Print(int value) = 0;
      virtual void Print(unsigned int value) = 0;
      virtual void Print(long value) = 0;
      virtual void Print(unsigned long value) = 0;
      virtual void PrintLn() = 0;
    };

    class OSMSCOUT_API Line
    {
    private:
      Destination& destination;

    public:
      Line(Destination& destination);
      virtual ~Line();

      inline Line& operator<<(const std::string& value)
      {
        destination.Print(value);

        return *this;
      }

      inline Line& operator<<(const char* value)
      {
        destination.Print(value);

        return *this;
      }

      inline Line& operator<<(bool value)
      {
        destination.Print(value);

        return *this;
      }

      inline Line& operator<<(short value)
      {
        destination.Print(value);

        return *this;
      }

      inline Line& operator<<(unsigned short value)
      {
        destination.Print(value);

        return *this;
      }

      inline Line& operator<<(int value)
      {
        destination.Print(value);

        return *this;
      }

      inline Line& operator<<(unsigned int value)
      {
        destination.Print(value);

        return *this;
      }

      inline Line& operator<<(long value)
      {
        destination.Print(value);

        return *this;
      }

      inline Line& operator<<(unsigned long value)
      {
        destination.Print(value);

        return *this;
      }

      Line& operator<<(float value);
      Line& operator<<(double value);

      inline Line& operator<<(const StopClock& value)
      {
        destination.Print(value.ResultString());

        return *this;
      }
    };

  public:
    Logger();
    virtual ~Logger();

    virtual Line Log(Level level) = 0;

    Line Debug();
    Line Info();
    Line Warn();
    Line Error();
  };

  class OSMSCOUT_API NoOpLogger : public Logger
  {
  private:
    class OSMSCOUT_API NoOpDestination : public Destination
    {
    public:
      inline void Print(const std::string& /*value*/)
      {
        // no code
      }

      inline void Print(const char* /*value*/)
      {
        // no code
      }

      inline void Print(bool /*value*/)
      {
        // no code
      }

      inline void Print(short /*value*/)
      {
        // no code
      }

      inline void Print(unsigned short /*value*/)
      {
        // no code
      }

      inline void Print(int /*value*/)
      {
        // no code
      }

      inline void Print(unsigned int /*value*/)
      {
        // no code
      }

      inline void Print(long /*value*/)
      {
        // no code
      }

      inline void Print(unsigned long /*value*/)
      {
        // no code
      }

      inline void PrintLn()
      {
        // no code
      }
    };

  private:
    NoOpDestination destination;

  public:
    inline Line Log(Level /*level*/)
    {
      return Line(destination);
    }
  };

  class OSMSCOUT_API StreamLogger : public Logger
  {
  private:
    class OSMSCOUT_API StreamDestination : public Destination
    {
    private:
      std::ostream& stream;

    public:
      StreamDestination(std::ostream& stream);

      void Print(const std::string& value);
      void Print(const char* value);
      void Print(bool value);
      void Print(short value);
      void Print(unsigned short value);
      void Print(int value);
      void Print(unsigned int value);
      void Print(long value);
      void Print(unsigned long value);
      void PrintLn();
    };

  private:
    StreamDestination infoDestination;
    StreamDestination errorDestination;

  public:
    StreamLogger(std::ostream& infoStream,
                 std::ostream& errorStream);

    Line Log(Level level);
  };

  class OSMSCOUT_API ConsoleLogger : public StreamLogger
  {
  public:
    ConsoleLogger();
  };

  class OSMSCOUT_API Log
  {
  private:
    Logger*    logger;
    NoOpLogger noOpLogger;
    bool       logDebug;
    bool       logInfo;
    bool       logWarn;
    bool       logError;

  public:
    Log();
    ~Log();

    void SetLogger(Logger* logger);

    inline Log& Debug(bool state)
    {
      logDebug=state;

      return *this;
    }

    Log& Info(bool state)
    {
      logInfo=state;

      return *this;
    }

    Log& Warn(bool state)
    {
      logWarn=state;

      return *this;
    }

    Log& Error(bool state)
    {
      logError=state;

      return *this;
    }

    Logger::Line Debug();
    Logger::Line Info();
    Logger::Line Warn();
    Logger::Line Error();
  };

  extern OSMSCOUT_API Log log;
}

#define OSMSCOUT_L (osmscout::Logger::GetGlobalInstance())

#endif
