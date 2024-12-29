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
#include <format>
#include <string>

#include <osmscout/lib/CoreImportExport.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class OSMSCOUT_API Progress
  {
  private:
    bool outputDebug;

  protected:
    Progress();

  public:
    Progress(const Progress& other) = delete;
    Progress(const Progress&& other) = delete;
    virtual ~Progress() = default;

    Progress operator=(const Progress& other) = delete;
    Progress operator=(const Progress&& other) = delete;

    void SetOutputDebug(bool outputDebug);
    bool OutputDebug() const;

    virtual void SetStep(const std::string& step);
    virtual void SetAction(const std::string& action);
    virtual void SetProgress(double current, double total, const std::string& label="");
    virtual void SetProgress(unsigned int current, unsigned int total, const std::string& label="");
    virtual void SetProgress(unsigned long current, unsigned long total, const std::string& label="");
    virtual void SetProgress(unsigned long long current, unsigned long long total, const std::string& label="");
    virtual void Debug(const std::string& text);
    virtual void Info(const std::string& text);
    virtual void Warning(const std::string& text);
    virtual void Error(const std::string& text);

    template<class... Args>
    void SetStep(std::format_string<Args...> format, Args&& ...args)
    {
      SetStep(std::vformat(format.get(), std::make_format_args(args...)));
    }

    template<class... Args>
    void SetAction(std::format_string<Args...> format, Args&& ...args)
    {
      this->SetAction(std::vformat(format.get(), std::make_format_args(args...)));
    }

    template<class... Args>
    void Debug(std::format_string<Args...> format, Args&& ...args)
    {
      this->Debug(std::vformat(format.get(), std::make_format_args(args...)));
    }

    template<class... Args>
    void Info(std::format_string<Args...> format, Args&& ...args)
    {
      this->Info(std::vformat(format.get(), std::make_format_args(args...)));
    }

    template<class... Args>
    void Warning(std::format_string<Args...> format, Args&& ...args)
    {
      this->Warning(std::vformat(format.get(), std::make_format_args(args...)));
    }

    template<class... Args>
    void Error(std::format_string<Args...> format, Args&& ...args)
    {
      this->Error(std::vformat(format.get(), std::make_format_args(args...)));
    }
  };

  class OSMSCOUT_API SilentProgress CLASS_FINAL : public Progress
  {
    public:
    SilentProgress() = default;
    ~SilentProgress() override = default;
  };

  class OSMSCOUT_API ConsoleProgress : public Progress
  {
  private:
    std::time_t lastProgressDump;

  public:
    ConsoleProgress() = default;
    ~ConsoleProgress() override = default;

    void SetStep(const std::string& step) override;
    void SetAction(const std::string& action) override;
    void SetProgress(double current, double total, const std::string& label) override;
    void SetProgress(unsigned int current, unsigned int total,const std::string& label) override;
    void SetProgress(unsigned long current, unsigned long total, const std::string& label="") override;
    void SetProgress(unsigned long long current, unsigned long long total, const std::string& label="") override;

    void Debug(const std::string& text) override;
    void Info(const std::string& text) override;
    void Warning(const std::string& text) override;
    void Error(const std::string& text) override;
  };
}

#endif
