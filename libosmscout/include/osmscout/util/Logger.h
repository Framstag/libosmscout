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

#include <osmscout/CoreImportExport.h>

#include <ostream>
#include <sstream>

#include <osmscout/util/StopClock.h>
#include <osmscout/util/Distance.h>

// Since we have a DEBUG enumeration member
#ifdef DEBUG
#undef DEBUG
#endif

// Since we have a ERROR enumeration member
#ifdef ERROR
#undef ERROR
#endif

namespace osmscout {

  /**
   * \ingroup Logging
   * A logger is a special output stream. It can direct internal output to either the
   * console, a file or some other (possibly OS specific) output sink. The actual
   * destination is defined by passing a Destination instance to a Line instance.
   */
  class OSMSCOUT_API Logger
  {
  public:
    enum Level {
      DEBUG,
      INFO,
      WARN,
      ERROR
    };

    /**
     * \ingroup Logging
     * Abstract base class for printing log information to a specific
     * output sink.
     */
    class OSMSCOUT_API Destination
    {
    public:
      Destination() = default;
      virtual ~Destination() = default;

      /**
       * Print a std::string
       */
      virtual void Print(const std::string& value) = 0;

      /**
       * Print a std::string_view
       */
      virtual void Print(const std::string_view& value) = 0;

      /**
       * Print a const char*
       */
      virtual void Print(const char* value) = 0;

      /**
       * Print a boolean value (values are printed as "true" or "false")
       */
      virtual void Print(bool value) = 0;
      virtual void Print(short value) = 0;
      virtual void Print(unsigned short value) = 0;
      virtual void Print(int value) = 0;
      virtual void Print(unsigned int value) = 0;
      virtual void Print(long value) = 0;
      virtual void Print(unsigned long value) = 0;
      virtual void Print(long long value) = 0;
      virtual void Print(unsigned long long value) = 0;

      /**
       * Finish printing the line. Internally called by the Line instance on destruction
       * of the Line.
       */
      virtual void PrintLn() = 0;
    };

    /**
     * \ingroup Logging
     * A log consists of a number of lines. A line is implicitly
     * created by the logger if instructing it to start logging
     * in a certain log level. The Logger at this point
     * passes the Line a destination.
     */
    class OSMSCOUT_API Line
    {
    private:
      Destination& destination;

    public:
      explicit Line(Destination& destination);
      virtual ~Line();

      Line& operator<<(const std::string& value)
      {
        destination.Print(value);

        return *this;
      }

      Line& operator<<(const std::string_view& value)
      {
        destination.Print(value);

        return *this;
      }

      Line& operator<<(const char* value)
      {
        destination.Print(value);

        return *this;
      }

      Line& operator<<(bool value)
      {
        destination.Print(value);

        return *this;
      }

      Line& operator<<(short value)
      {
        destination.Print(value);

        return *this;
      }

      Line& operator<<(unsigned short value)
      {
        destination.Print(value);

        return *this;
      }

      Line& operator<<(int value)
      {
        destination.Print(value);

        return *this;
      }

      Line& operator<<(unsigned int value)
      {
        destination.Print(value);

        return *this;
      }

      Line& operator<<(long value)
      {
        destination.Print(value);

        return *this;
      }

      Line& operator<<(unsigned long value)
      {
        destination.Print(value);

        return *this;
      }

      Line& operator<<(long long value)
      {
        destination.Print(value);

        return *this;
      }

      Line& operator<<(unsigned long long value)
      {
        destination.Print(value);

        return *this;
      }

      Line& operator<<(float value);
      Line& operator<<(double value);
      Line& operator<<(void* value);

      Line& operator<<(const StopClock& value)
      {
        destination.Print(value.ResultString());

        return *this;
      }

      Line& operator<<(const Distance& value)
      {
        destination.Print(value.AsString());

        return *this;
      }
    };

  protected:
    /**
     * The actual logging method, Debug(), Info(), Warn() and Error() are dispatching to.
     */
    virtual Line Log(Level level) = 0;

  public:
    Logger() = default;
    virtual ~Logger() = default;

    /**
     * Start logging a line of debug output
     */
    Line Debug();

    /**
     * Start logging a line of informational output
     */
    Line Info();

    /**
     * Start logging a line of warning output (there is a potential problem, but
     * the application could handle it)
     */
    Line Warn();

    /**
     * Start logging a line of error output
     */
    Line Error();
  };

  /**
   * \ingroup Logging
   * Special Logger that just does *not* output the logged information.
   */
  class OSMSCOUT_API NoOpLogger : public Logger
  {
  private:
    /**
     * \ingroup Logging
     * Special "no operation" destination.
     */
    class OSMSCOUT_API NoOpDestination : public Destination
    {
    public:
       void Print(const std::string& /*value*/) override
      {
        // no code
      }

       void Print(const std::string_view& /*value*/) override
      {
        // no code
      }

       void Print(const char* /*value*/) override
      {
        // no code
      }

       void Print(bool /*value*/) override
      {
        // no code
      }

       void Print(short /*value*/) override
      {
        // no code
      }

       void Print(unsigned short /*value*/) override
      {
        // no code
      }

       void Print(int /*value*/) override
      {
        // no code
      }

       void Print(unsigned int /*value*/) override
      {
        // no code
      }

       void Print(long /*value*/) override
      {
        // no code
      }

       void Print(unsigned long /*value*/) override
      {
        // no code
      }

       void Print(long long /*value*/) override
      {
        // no code
      }

       void Print(unsigned long long /*value*/) override
      {
        // no code
      }

       void PrintLn() override
      {
        // no code
      }
    };

  private:
    NoOpDestination destination;

  public:
     Line Log(Level /*level*/) override
    {
      return Line(destination);
    }
  };

  /**
   * \ingroup Logging
   *  The StreamLogger allows to direct logging output to a standard library std::ostream.
   *  IT allows to assign one stream for DEBUG and INFO logging and a different stream
   *  for WARN and ERROR log output.
   */
  class OSMSCOUT_API StreamLogger : public Logger
  {
  private:
    /**
     * \ingroup Logging
     * Special Destination implementation that delegates printing to the assigned
     * std::ostream.
     */
    class OSMSCOUT_API StreamDestination : public Destination
    {
    private:
      std::ostream& stream;

    public:
      explicit StreamDestination(std::ostream& stream);

      void Print(const std::string& value) override;
      void Print(const std::string_view& value) override;
      void Print(const char* value) override;
      void Print(bool value) override;
      void Print(short value) override;
      void Print(unsigned short value) override;
      void Print(int value) override;
      void Print(unsigned int value) override;
      void Print(long value) override;
      void Print(unsigned long value) override;
      void Print(long long value) override;
      void Print(unsigned long long value) override;
      void PrintLn() override;
    };

  private:
    StreamDestination infoDestination;
    StreamDestination errorDestination;

  public:
    StreamLogger(std::ostream& infoStream,
                 std::ostream& errorStream);

    Line Log(Level level) override;
  };

  /**
   * \ingroup Logging
   * The console logger extends the StreamLogger by assigning std::cout for normal
   * loging output and std::cerr for error output.
   */
  class OSMSCOUT_API ConsoleLogger : public StreamLogger
  {
  public:
    ConsoleLogger();
  };

  /**
   * \ingroup Logging
   * Simple logging proxy object that encapsulates one exchangeable global
   * logger instance. Log should behave as Logger in all other cases.
   */
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

     Log& Debug(bool state)
    {
      logDebug=state;

      return *this;
    }

     bool IsDebug() const
    {
      return logDebug;
    }

     bool IsInfo() const
    {
      return logInfo;
    }

     bool IsWarn() const
    {
      return logWarn;
    }

     bool IsError() const
    {
      return logError;
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

  /**
   * \ingroup Logging
   * The one an donly global instance of the logger that should get used
   * for all logging output.
   */
  extern OSMSCOUT_API Log log;
}

/**
 * \defgroup Logging
 *
   * A logger is a special output stream, that is used by the library.
   *
   * The logger has a uniform interface independent of the actual
   * data sink the information is stored to.
   *
   * This allows the application developer (and library user) to
   * redirect logging output either to the console, to some special
   * OS information sink, to "nowhere", to a file or any other location.
   *
   * The actual logger used can get exchanged by using Log::SetLogger.
 */

#endif
