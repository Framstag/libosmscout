/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/Progress.h>

#include <iostream>

namespace osmscout {

  Progress::Progress()
  {
    // no code
  }

  Progress::~Progress()
  {
    // no code
  }

  void Progress::SetStep(const std::string& step)
  {
    // no code
  }

  void Progress::SetAction(const std::string& action)
  {
    // no code
  }

  void Progress::Debug(const std::string& text)
  {
    // no code
  }

  void Progress::Info(const std::string& text)
  {
    // no code
  }

  void Progress::Warning(const std::string& text)
  {
    // no code
  }

  void Progress::Error(const std::string& text)
  {
    // no code
  }


  void ConsoleProgress::SetStep(const std::string& step)
  {
    std::cout << "+ " << step << "..." << std::endl;
  }

  void ConsoleProgress::SetAction(const std::string& action)
  {
    std::cout << " - " << action << "..." << std::endl;
  }

  void ConsoleProgress::Debug(const std::string& text)
  {
    std::cout << "   " << text << std::endl;
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

