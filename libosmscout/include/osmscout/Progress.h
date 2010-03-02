#ifndef OSMSCOUT_PROGRESS_H
#define OSMSCOUT_PROGRESS_H

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

#include <string>

namespace osmscout {

  class Progress
  {
  protected:
    Progress();

  public:
    virtual ~Progress();

    virtual void SetStep(const std::string& step);
    virtual void SetAction(const std::string& action);

    virtual void Debug(const std::string& text);
    virtual void Info(const std::string& text);
    virtual void Warning(const std::string& text);
    virtual void Error(const std::string& text);
  };

  class ConsoleProgress : public Progress
  {
  public:
    void SetStep(const std::string& step);
    void SetAction(const std::string& action);

    void Debug(const std::string& text);
    void Info(const std::string& text);
    void Warning(const std::string& text);
    void Error(const std::string& text);
  };
}

#endif
