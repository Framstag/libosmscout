/*
  TravelJinni - Openstreetmap offline viewer
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

#include "Configuration.h"

#include <iostream>

#include <Lum/Base/Path.h>
#include <Lum/Base/String.h>

#include <Lum/Config/Config.h>

std::list<Map>   maps;
std::list<Style> styles;

std::wstring     currentMap;
std::wstring     currentStyle;

bool LoadConfig()
{
  Lum::Config::Node      *top;
  Lum::Config::ErrorList errors;
  Lum::Base::Path        path(Lum::Base::Path::GetApplicationConfigPath());

  top=Lum::Config::LoadConfigFromXMLFile(path.GetPath(),errors);

  if (top==NULL || top->GetName()!=L"TravelJinni") {
    std::cerr << "'" << Lum::Base::WStringToString(path.GetPath()) << "' is not a valid config file!" << std::endl;
    delete top;
    return false;
  }

  for (Lum::Config::Node::NodeList::const_iterator iter=top->GetChildren().begin();
       iter!=top->GetChildren().end();
       ++iter) {
    Lum::Config::Node *node=*iter;

    if (node->GetName()==L"map") {
      Map map;

      if (node->GetAttribute(L"dir",map.dir)) {
        bool active=false;

        node->GetAttribute(L"active",active);

        if (active) {
          currentMap=map.dir;
        }

        maps.push_back(map);
      }
    }
    else if (node->GetName()==L"style") {
      Style style;

      if (node->GetAttribute(L"file",style.file)) {
        bool active=false;

        node->GetAttribute(L"active",active);

        if (active) {
          currentStyle=style.file;
        }

        styles.push_back(style);
      }
    }
    else {
      std::cout << "Unnown element'" << Lum::Base::WStringToString(node->GetName()) << "'" << std::endl;
    }
  }

  if (currentMap.empty() && maps.size()>0) {
    currentMap=maps.front().dir;
  }

  if (currentStyle.empty() && styles.size()>0) {
    currentStyle=styles.front().file;
  }

  delete top;

  return true;
}

bool SaveConfig()
{
  Lum::Config::Node *top;
  Lum::Base::Path   path(Lum::Base::Path::GetApplicationConfigPath());
  std::wstring      config;
  bool              res;

  top=new Lum::Config::Node();
  top->SetName(L"TravelJinni");

  for (std::list<Map>::const_iterator map=maps.begin();
       map!=maps.end();
       ++map) {
    Lum::Config::Node *node=new Lum::Config::Node(L"map");

    node->SetAttribute(L"dir",map->dir);

    if (map->dir==currentMap) {
      node->SetAttribute(L"active",true);
    }

    top->Add(node);
  }

  for (std::list<Style>::const_iterator style=styles.begin();
       style!=styles.end();
       ++style) {
    Lum::Config::Node *node=new Lum::Config::Node(L"style");

    node->SetAttribute(L"file",style->file);

    if (style->file==currentStyle) {
      node->SetAttribute(L"active",true);
    }

    top->Add(node);
  }

  Lum::Base::Status status;

  status=path.CreateDirRecursive();

  if (!status) {
    std::cerr << "Cannot create config directory '" << Lum::Base::WStringToString(path.GetDir()) << "': " << Lum::Base::WStringToString(status.GetDescription()) << std::endl;
    return false;
  }

  res=Lum::Config::SaveConfigToXMLFile(path.GetPath(),top);

  delete top;

  return res;
}


