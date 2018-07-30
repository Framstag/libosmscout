/*
  DrawMap - a demo program for libosmscout
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

#include <iostream>
#include <iomanip>

#include <osmscout/Database.h>
#include <osmscout/MapService.h>

#include <osmscout/MapPainterIOS.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory):

  src/DrawMapOSX ../maps/nordrhein-westfalen ../stylesheets/standard.oss 1024 800 7.46525 51.51241 70000 test.png
 */

static const double DPI=96.0;

int main(int argc, char* argv[])
{
  std::string   map;
  std::string   style;
  std::string   output;
  size_t        width,height;
  double        lon,lat,zoom;

  if (argc!=9) {
    std::cerr << "DrawMap <map directory> <style-file> <width> <height> <lon> <lat> <zoom> <output>" << std::endl;
    return 1;
  }

  map=argv[1];
  style=argv[2];

  if (!osmscout::StringToNumber(argv[3],width)) {
    std::cerr << "width is not numeric!" << std::endl;
    return 1;
  }

  if (!osmscout::StringToNumber(argv[4],height)) {
    std::cerr << "height is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[5],"%lf",&lon)!=1) {
    std::cerr << "lon is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[6],"%lf",&lat)!=1) {
    std::cerr << "lat is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[7],"%lf",&zoom)!=1) {
    std::cerr << "zoom is not numeric!" << std::endl;
    return 1;
  }

  output=argv[8];

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database(new osmscout::Database(databaseParameter));
  osmscout::MapServiceRef     mapService(new osmscout::MapService(database));

  if (!database->Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::StyleConfigRef styleConfig(new osmscout::StyleConfig(database->GetTypeConfig()));

  if (!styleConfig->Load(style)) {
    std::cerr << "Cannot open style" << std::endl;
    return 1;
  }

    
    CGSize size = CGSizeMake(width, height);
    CGContextRef bitmapContext = CGBitmapContextCreate(NULL, width, height, 8, 0, [[NSColorSpace genericRGBColorSpace] CGColorSpace], kCGBitmapByteOrder32Host|kCGImageAlphaPremultipliedFirst);
    NSGraphicsContext *nsgc = [NSGraphicsContext graphicsContextWithGraphicsPort:bitmapContext flipped:YES];
    [NSGraphicsContext setCurrentContext:nsgc];
    CGContextRef cg = (CGContextRef)[nsgc graphicsPort];
    CGAffineTransform flipVertical = CGAffineTransformMake(1, 0, 0, -1, 0, height);
    CGContextConcatCTM(cg, flipVertical);
    
    if (cg) {
        osmscout::MercatorProjection  projection;
        osmscout::MapParameter        drawParameter;
        osmscout::AreaSearchParameter searchParameter;
        osmscout::MapData             data;
        osmscout::MapPainterIOS       painter(styleConfig);
        
        drawParameter.SetFontSize(3.0);
        drawParameter.SetLabelLineMinCharCount(15);
        drawParameter.SetLabelLineMaxCharCount(30);
        drawParameter.SetLabelLineFitToArea(true);
        drawParameter.SetLabelLineFitToWidth(std::min(projection.GetWidth(), projection.GetHeight()));
        
        projection.Set(osmscout::GeoCoord(lat,lon),
                       osmscout::Magnification(zoom),
                       DPI,
                       width,
                       height);
        
        std::list<osmscout::TileRef> tiles;
        
        mapService->LookupTiles(projection,tiles);
        mapService->LoadMissingTileData(searchParameter,*styleConfig,tiles);
        mapService->AddTileDataToMapData(tiles,data);
        
        if (painter.DrawMap(*styleConfig,
                            projection,
                            drawParameter,
                            data,
                            cg)) {
            CGImageRef cgImage = CGBitmapContextCreateImage(bitmapContext);
            CGContextRelease(bitmapContext);
            NSBitmapImageRep *bitmapRep = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
            NSDictionary *props = [NSDictionary dictionary];
            NSData *imgData = [bitmapRep representationUsingType:NSPNGFileType properties:props];
            NSString *path = [[NSString alloc] initWithUTF8String:output.c_str()];
            if(![imgData writeToFile:path atomically:NO]){
                std::cerr << "Cannot write PNG" << std::endl;
            }
        }
        
    }
    else {
        std::cerr << "Cannot create graphic context" << std::endl;
    }

  return 0;
}
