#ifndef OSMSCOUT_UTIL_PROGRESS_H
#define OSMSCOUT_UTIL_PROGRESS_H

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

#include <ctime>
#include <string>

#include <osmscout/CoreFeatures.h>

#include <osmscout/private/CoreImportExport.h>

namespace osmscout {

  class OSMSCOUT_API Progress
  {
  private:
    bool outputDebug;

  protected:
    Progress();

  public:
    virtual ~Progress();

    void SetOutputDebug(bool outputDebug);
    bool OutputDebug() const;

    virtual void SetStep(const std::string& step);
    virtual void SetAction(const std::string& action);
    virtual void SetProgress(double current, double total);
    virtual void SetProgress(unsigned int current, unsigned int total);
    virtual void SetProgress(unsigned long current, unsigned long total);
    virtual void SetProgress(unsigned long long current, unsigned long long total);
    virtual void Debug(const std::string& text);
    virtual void Info(const std::string& text);
    virtual void Warning(const std::string& text);
    virtual void Error(const std::string& text);
  };

  class OSMSCOUT_API SilentProgress : public Progress
  {
  public:
    virtual ~SilentProgress();
  };

  class OSMSCOUT_API ConsoleProgress : public Progress
  {
  private:
    std::time_t lastProgressDump;

  public:
    void SetStep(const std::string& step);
    void SetAction(const std::string& action);
    void SetProgress(double current, double total);
    void SetProgress(unsigned int current, unsigned int total);
    void SetProgress(unsigned long current, unsigned long total);
    void SetProgress(unsigned long long current, unsigned long long total);

    void Debug(const std::string& text);
    void Info(const std::string& text);
    void Warning(const std::string& text);
    void Error(const std::string& text);
  };
}

#endif
