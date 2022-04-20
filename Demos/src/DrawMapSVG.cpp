/*
  DrawMap - a demo program for libosmscout
  Copyright (C) 2011  Tim Teulings

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

#include <DrawMap.h>

#include <fstream>
#include <iostream>

#include <osmscoutmapsvg/MapPainterSVG.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory):

  src/DrawMapSVG <path-to-map> ../stylesheets/standard.oss 1000 1000 51.2   6.5    1000 test.svg
  src/DrawMapSVG <path-to-map> ../stylesheets/standard.oss 1000 1000 51.565 7.45 160000 test.svg
  src/DrawMapSVG <path-to-map> ../stylesheets/standard.oss 1000 1000 51.48  7.45 160000 test.svg
*/

int main(int argc, char* argv[])
{
  DrawMapDemo drawDemo("DrawMapSVG", argc, argv);

  if (!drawDemo.OpenDatabase()){
    return 2;
  }

  drawDemo.LoadData();
  Arguments args = drawDemo.GetArguments();
#ifdef WIN32
  // Windows font file fix for SVG
  char drive1[_MAX_DRIVE], dir1[_MAX_DIR], fname1[_MAX_FNAME], ext1[_MAX_EXT];
  _splitpath(args.fontName.c_str(), drive1, dir1, fname1, ext1);
  if (stricmp(ext1, ".ttf") == 0 || stricmp(ext1, ".otf") == 0)
  {
    char drive2[_MAX_DRIVE], dir2[_MAX_DIR], fname2[_MAX_FNAME], ext2[_MAX_EXT];
    HKEY hKey = nullptr;
    if (RegOpenKey(HKEY_LOCAL_MACHINE, R"(SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts)", &hKey) == ERROR_SUCCESS)
    {
      DWORD cValues = 0, cchMaxValue, cbMaxValueData, cbSecurityDescriptor, cchName = 16383, cchValue = 16383, cType = 0;
      FILETIME ftLastWriteTime;
      char achName[16383], achValue[16383];
      DWORD retCode = RegQueryInfoKey(hKey,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      &cValues,
                                      &cchMaxValue,
                                      &cbMaxValueData,
                                      &cbSecurityDescriptor,
                                      &ftLastWriteTime);
      if (retCode == ERROR_SUCCESS && cValues > 0)
      {
        for (DWORD i = 0; i < cValues; i++)
        {
          cchName = 16383;
          achName[0] = '\0';
          cchValue = 16383;
          achValue[0] = '\0';

          if (RegEnumValueA(hKey,
                            i,
                            achName,
                            &cchName,
                            nullptr,
                            &cType,
                            (LPBYTE) achValue,
                            &cchValue) == ERROR_SUCCESS)
          {
            _splitpath(achValue, drive2, dir2, fname2, ext2);
            if (stricmp(fname1, fname2) == 0 && stricmp(ext1, ext2) == 0)
            {
              size_t length = strlen(achName);

              if (length > 11)
              {
                if (strcmp(achName + length - 11, " (TrueType)") == 0)
                {
                  achName[length - 11] = 0;
                }
              }

              drawDemo.drawParameter.SetFontName(std::string(achName));
              break;
            }
          }
        }
      }
      RegCloseKey(hKey);
    }
  }
#endif

  std::ofstream stream(args.output.c_str(), std::ios_base::binary|std::ios_base::trunc|std::ios_base::out);

  if (!stream) {
    std::cerr << "Cannot open '" << args.output << "' for writing!" << std::endl;
    return 1;
  }

  osmscout::MapPainterSVG painter(drawDemo.styleConfig);

  painter.DrawMap(drawDemo.projection,
                  drawDemo.drawParameter,
                  drawDemo.data,
                  stream);

  stream.close();

  return 0;
}
