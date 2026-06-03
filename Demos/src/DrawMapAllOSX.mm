/*
  DrawMapAll - OSX backend rendering
  Copyright (C) 2025  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <DrawMap.h>

#include <osmscoutmapiosx/MapPainterIOS.h>

#include <fstream>
#include <iostream>

bool RenderWithOSX(DrawMapDemo& drawDemo,
                   const std::string& outputDir,
                   size_t width,
                   size_t height)
{
  Arguments args = drawDemo.GetArguments();

  if (!ValidateFontArguments(args)) {
    return false;
  }
  drawDemo.drawParameter.SetFontName("GillSans");
  drawDemo.drawParameter.SetFontSize(3.0);
  drawDemo.drawParameter.SetLabelLineMinCharCount(15);
  drawDemo.drawParameter.SetLabelLineMaxCharCount(30);
  drawDemo.drawParameter.SetLabelLineFitToArea(true);
  drawDemo.drawParameter.SetLabelLineFitToWidth(
      std::min(drawDemo.projection.GetWidth(), drawDemo.projection.GetHeight()));

  CGContextRef bitmapContext = CGBitmapContextCreate(
      nullptr, width, height, 8, 0,
      [[NSColorSpace genericRGBColorSpace] CGColorSpace],
      kCGBitmapByteOrder32Host | kCGImageAlphaPremultipliedFirst);

  if (!bitmapContext) {
    std::cerr << "  FAIL: OSX bitmap context creation" << std::endl;
    return false;
  }

  NSGraphicsContext* nsgc = [NSGraphicsContext graphicsContextWithCGContext:bitmapContext
                                                                     flipped:YES];
  [NSGraphicsContext setCurrentContext:nsgc];
  CGContextRef cg = [nsgc CGContext];
  CGAffineTransform flipVertical = CGAffineTransformMake(1, 0, 0, -1, 0, height);
  CGContextConcatCTM(cg, flipVertical);

  osmscout::MapPainterIOS painter;
  bool ok = painter.DrawMap(drawDemo.projection,
                             drawDemo.drawParameter,
                             drawDemo.data,
                             cg);

  if (ok) {
    CGImageRef cgImage = CGBitmapContextCreateImage(bitmapContext);
    if (cgImage) {
      NSBitmapImageRep* bitmapRep = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
      NSDictionary* props = [NSDictionary dictionary];
      NSData* imgData = [bitmapRep representationUsingType:NSBitmapImageFileTypePNG
                                                properties:props];

      std::string path = outputDir + "/DrawMapOSX.png";
      NSString* nsPath = [[NSString alloc] initWithUTF8String:path.c_str()];
      ok = [imgData writeToFile:nsPath atomically:NO];
      CGImageRelease(cgImage);
    } else {
      std::cerr << "  FAIL: OSX CGImage creation" << std::endl;
      ok = false;
    }
  }

  CGContextRelease(bitmapContext);
  return ok;
}
