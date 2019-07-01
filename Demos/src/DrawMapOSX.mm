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

#include <DrawMap.h>

#include <osmscout/MapPainterIOS.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory):

  src/DrawMapOSX ../maps/nordrhein-westfalen ../images ../stylesheets/standard.oss 1024 800 7.46525 51.51241 70000 test.png
 */

static const double DPI=96.0;

int main(int argc, char* argv[])
{
  DrawMapDemo drawDemo("DrawMapOSX",
                       argc,argv,
                       DPI);

  if (!drawDemo.OpenDatabase()){
    return 2;
  }

  Arguments args = drawDemo.GetArguments();

  CGSize size = CGSizeMake(args.width, args.height);
  CGContextRef bitmapContext = CGBitmapContextCreate(nullptr, args.width, args.height, 8, 0, [[NSColorSpace genericRGBColorSpace] CGColorSpace], kCGBitmapByteOrder32Host|kCGImageAlphaPremultipliedFirst);
  NSGraphicsContext *nsgc = [NSGraphicsContext graphicsContextWithGraphicsPort:bitmapContext flipped:YES];
  [NSGraphicsContext setCurrentContext:nsgc];
  CGContextRef cg = (CGContextRef)[nsgc graphicsPort];
  CGAffineTransform flipVertical = CGAffineTransformMake(1, 0, 0, -1, 0, args.height);
  CGContextConcatCTM(cg, flipVertical);

  if (cg) {
    osmscout::MapPainterIOS painter(drawDemo.styleConfig);

    drawDemo.drawParameter.SetFontName("GillSans");
    drawDemo.drawParameter.SetFontSize(3.0);
    drawDemo.drawParameter.SetLabelLineMinCharCount(15);
    drawDemo.drawParameter.SetLabelLineMaxCharCount(30);
    drawDemo.drawParameter.SetLabelLineFitToArea(true);
    drawDemo.drawParameter.SetLabelLineFitToWidth(std::min(drawDemo.projection.GetWidth(), drawDemo.projection.GetHeight()));

    drawDemo.LoadData();

    if (painter.DrawMap(*drawDemo.styleConfig,
                        drawDemo.projection,
                        drawDemo.drawParameter,
                        drawDemo.data,
                        cg)) {
      CGImageRef cgImage = CGBitmapContextCreateImage(bitmapContext);
      CGContextRelease(bitmapContext);
      NSBitmapImageRep *bitmapRep = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
      NSDictionary *props = [NSDictionary dictionary];
      NSData *imgData = [bitmapRep representationUsingType:NSPNGFileType properties:props];
      NSString *path = [[NSString alloc] initWithUTF8String:args.output.c_str()];
      if(![imgData writeToFile:path atomically:NO]){
          std::cerr << "Cannot write PNG" << std::endl;
      }
    }
    else {
      std::cerr << "Error during drawing" << std::endl;
    }
  }
  else {
      std::cerr << "Cannot create graphic context" << std::endl;
  }

  return 0;
}
