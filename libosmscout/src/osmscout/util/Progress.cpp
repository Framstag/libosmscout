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

#include <osmscout/util/Progress.h>

#include <iostream>
#include <iomanip>

namespace osmscout {

  Progress::Progress()
  : outputDebug(false)
  {
    // no code
  }

  Progress::~Progress()
  {
    // no code
  }

  void Progress::SetOutputDebug(bool outputDebug)
  {
    this->outputDebug=outputDebug;
  }

  bool Progress::OutputDebug() const
  {
    return outputDebug;
  }

  void Progress::SetStep(const std::string& /*step*/)
  {
    // no code
  }

  void Progress::SetAction(const std::string& /*action*/)
  {
    // no code
  }

  void Progress::SetProgress(double /*current*/,
                             double /*total*/)
  {
    // no code
  }

  void Progress::SetProgress(unsigned int /*current*/,
                             unsigned int /*total*/)
  {
    // no code
  }

  void Progress::SetProgress(unsigned long /*current*/,
                             unsigned long /*total*/)
  {
    // no code
  }

  void Progress::SetProgress(unsigned long long /*current*/,
                             unsigned long long /*total*/)
  {
    // no code
  }

  void Progress::Debug(const std::string& /*text*/)
  {
    // no code
  }

  void Progress::Info(const std::string& /*text*/)
  {
    // no code
  }

  void Progress::Warning(const std::string& /*text*/)
  {
    // no code
  }

  void Progress::Error(const std::string& /*text*/)
  {
    // no code
  }

  SilentProgress::~SilentProgress()
  {
    // no code
  }

  void ConsoleProgress::SetStep(const std::string& step)
  {
    std::cout << "+ " << step << "..." << std::endl;

    lastProgressDump=0;
  }

  void ConsoleProgress::SetAction(const std::string& action)
  {
    std::cout << " - " << action << "..." << std::endl;

    lastProgressDump=0;
  }

  void ConsoleProgress::SetProgress(double current, double total)
  {
    if (lastProgressDump==0) {
      lastProgressDump=time(NULL);
      return;
    }

    time_t now=time(NULL);

    if (now-lastProgressDump>=5) {
      lastProgressDump=now;
      std::cout << "   % " << std::setiosflags(std::ios::fixed) << std::setprecision(2) << current/total*100 << " (" << std::setprecision(0) << current << "/" << std::setprecision(0) << total << ")" << std::endl;
    }
  }

  void ConsoleProgress::SetProgress(unsigned int current, unsigned int total)
  {
    SetProgress((double)current, (double)total);
  }

  void ConsoleProgress::SetProgress(unsigned long current, unsigned long total)
  {
    SetProgress((double)current,(double)total);
  }

  void ConsoleProgress::SetProgress(unsigned long long current,
                                    unsigned long long total)
  {
    SetProgress((double)current,(double)total);
  }

  void ConsoleProgress::Debug(const std::string& text)
  {
    if (OutputDebug()) {
      std::cout << "   " << text << std::endl;
    }
  }

  void ConsoleProgress::Info(const std::string& text)
  {
    std::cout << "   " << text << std::endl;
  }

  void ConsoleProgress::Warning(const std::string& text)
  {
    std::cout << "   WW " << text << std::endl;
  }

  void ConsoleProgress::Error(const std::string& text)
  {
    std::cout << "   !! " << text << std::endl;
  }
}

