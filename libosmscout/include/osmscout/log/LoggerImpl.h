#ifndef OSMSCOUT_UTIL_LOGGER_IMPL_H
#define OSMSCOUT_UTIL_LOGGER_IMPL_H

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

#include <osmscout/lib/CoreImportExport.h>

#include <osmscout/log/Logger.h>

#include <sstream>

namespace osmscout {

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
   * logging output and std::cerr for error output.
   */
  class OSMSCOUT_API ConsoleLogger : public StreamLogger
  {
  public:
    ConsoleLogger();
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
