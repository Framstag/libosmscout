/*
  This source is part of the libosmscout-map library
  Copyright (C) 2009  Tim Teulings, Vladimir Vyskocil

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

#import <osmscout/MapPainterIOS.h>

#include <cassert>
#include <iostream>
#include <limits>

#include <osmscout/util/Geometry.h>

#include <osmscout/private/Math.h>

namespace osmscout {
        
    MapPainterIOS::MapPainterIOS()
    : MapPainter(new CoordBufferImpl<Vertex2D>()),
    coordBuffer((CoordBufferImpl<Vertex2D>*)transBuffer.buffer)
    {
    }
        
    MapPainterIOS::~MapPainterIOS(){
        for(std::vector<Image>::const_iterator image=images.begin(); image<images.end();image++){
            CGImageRelease(*image);
        }
        for(std::vector<Image>::const_iterator image=patternImages.begin(); image<patternImages.end();image++){
            CGImageRelease(*image);
        }
        for(std::vector<CGPatternRef>::const_iterator p=patterns.begin(); p<patterns.end();p++){
            CGPatternRelease(*p);
        }
        for(std::map<size_t,Font *>::const_iterator f=fonts.begin(); f!=fonts.end();f++){
            #if !__has_feature(objc_arc)
            [f->second release];
            #endif
        }
    }
    
    Font *MapPainterIOS::GetFont(const MapParameter& parameter,
                                double fontSize)
    {
        std::map<size_t,Font *>::const_iterator f;
        
        fontSize=fontSize*ConvertWidthToPixel(parameter,parameter.GetFontSize());
        
        f=fonts.find(fontSize);
        
        if (f!=fonts.end()) {
            return f->second;
        }
        
        Font *font = [Font fontWithName:[NSString stringWithUTF8String: parameter.GetFontName().c_str()] size:fontSize];
        #if !__has_feature(objc_arc)
        [font retain];
        #endif
        return fonts.insert(std::pair<size_t,Font *>(fontSize,font)).first->second;
    }

    
    /*
     * DrawMap()
     */
    bool MapPainterIOS::DrawMap(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data,
                               CGContextRef paintCG){
        this->cg = paintCG;
        Draw(styleConfig,
             projection,
             parameter,
             data);
        return true;
    }

    /*
     * HasIcon()
     */
    bool MapPainterIOS::HasIcon(const StyleConfig& styleConfig,
                                const MapParameter& parameter,
                                IconStyle& style){
        if (style.GetIconId()==0) {
            return false;
        }
        
        size_t idx=style.GetIconId()-1;
        
        if (idx<images.size() &&
            images[idx]!=NULL) {

            return true;
        }
        
        for (std::list<std::string>::const_iterator path=parameter.GetIconPaths().begin();
             path!=parameter.GetIconPaths().end();
             ++path) {
            
            std::string filename=*path+style.GetIconName()+".png";
            std::cout << "Trying to Load image " << filename << std::endl;
            
#if TARGET_OS_IPHONE
            UIImage *image = [[UIImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String: filename.c_str()]];
#else
            NSImage *image = [[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String: filename.c_str()]];
#endif
            if (image) {
#if TARGET_OS_IPHONE
                CGImageRef imgRef= [image CGImage];
#else
                CGImageRef imgRef= [image CGImageForProposedRect:NULL context:[NSGraphicsContext currentContext] hints:NULL];
#endif
                CGImageRetain(imgRef);
                
                if (idx>=images.size()) {
                    images.resize(idx+1, NULL);
                }
                
                images[idx]=imgRef;                
                std::cout << "Loaded image '" << filename << "'" << std::endl;

                return true;
            }
        }
        
        std::cerr << "ERROR while loading image '" << style.GetIconName() << "'" << std::endl;
        style.SetIconId(0);
        
        return false;
}
    
    static void DrawPattern (void * info,CGContextRef cg){
        CGImageRef imgRef = (CGImageRef)info;
#if TARGET_OS_IPHONE
        CGAffineTransform transform = {1,0,0,-1,0,0};
        transform.ty = CGImageGetHeight(imgRef);
        CGContextConcatCTM(cg,transform);
#endif
        CGRect rect = CGRectMake(0, 0, CGImageGetWidth(imgRef), CGImageGetHeight(imgRef));
        CGContextDrawImage(cg, rect, imgRef);
    }
        
    static CGPatternCallbacks patternCallbacks = {
      0, &DrawPattern,NULL
    };
    
    /*
     * HasPattern()
     */
    bool MapPainterIOS::HasPattern(const MapParameter& parameter,
                                   const FillStyle& style){
        assert(style.HasPattern());
        
        // Was not able to load pattern
        if (style.GetPatternId()==0) {
            return false;
        }
        
        size_t idx=style.GetPatternId()-1;
        
        if (idx<patterns.size() &&
            patterns[idx]) {

            return true;
        }
        
        for (std::list<std::string>::const_iterator path=parameter.GetPatternPaths().begin();
             path!=parameter.GetPatternPaths().end();
             ++path) {
            std::string filename=*path+style.GetPatternName()+".png";
            
#if TARGET_OS_IPHONE
            UIImage *image = [[UIImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String: filename.c_str()]];
#else
            NSImage *image = [[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String: filename.c_str()]];
#endif
            if (image) {
                NSInteger imgWidth = [image size].width;
                NSInteger imgHeight = [image size].height;
#if TARGET_OS_IPHONE
                CGImageRef imgRef= [image CGImage];
#else
                NSRect rect = CGRectMake(0, 0, 16, 16);
                NSImageRep *imageRep = [image bestRepresentationForRect:rect context:[NSGraphicsContext currentContext] hints:0];
                imgWidth = [imageRep pixelsWide];
                imgHeight = [imageRep pixelsHigh];
                rect = CGRectMake(0, 0, imgWidth, imgHeight);
                CGImageRef imgRef= [image CGImageForProposedRect:&rect context:[NSGraphicsContext currentContext] hints:NULL];
#endif
                CGImageRetain(imgRef);
                patternImages.resize(patternImages.size()+1,imgRef);
                style.SetPatternId(patternImages.size());
                CGPatternRef pattern = CGPatternCreate(imgRef, CGRectMake(0,0, imgWidth, imgHeight), CGAffineTransformIdentity, imgWidth, imgHeight, kCGPatternTilingNoDistortion, true, &patternCallbacks);
                patterns.resize(patternImages.size(),pattern);
                
                std::cout << "Loaded image " << filename << " (" <<  imgWidth << "x" << imgHeight <<  ") => id " << style.GetPatternId() << std::endl;
                
                return true;
            }
        }
        
        std::cerr << "ERROR while loading icon file '" << style.GetPatternName() << "'" << std::endl;
        style.SetPatternId(std::numeric_limits<size_t>::max());
        
        return false;
    }
    
    /*
     * GetTextDimension()
     */
     void MapPainterIOS::GetTextDimension(const MapParameter& parameter,
                          double fontSize,
                          const std::string& text,
                          double& xOff,
                          double& yOff,
                          double& width,
                          double& height){
        Font *font = GetFont(parameter,fontSize);
        NSString *str = [NSString stringWithUTF8String:text.c_str()];
#if TARGET_OS_IPHONE
        CGSize size = [str sizeWithFont:font];
#else
        NSRect stringBounds = [str boundingRectWithSize:CGSizeMake(500, 50) options:NSStringDrawingUsesLineFragmentOrigin attributes:[NSDictionary dictionaryWithObject:font forKey:NSFontAttributeName]];
        CGSize size = stringBounds.size;
#endif
        xOff = 0;
        yOff = 0;
        width = size.width;
        height = size.height;
    }
 
    double MapPainterIOS::textLength(const MapParameter& parameter, double fontSize, std::string text){
        double xOff;
        double yOff;
        double width;
        double height;
        GetTextDimension(parameter,fontSize,text,xOff,yOff,width,height);
        return width;
    }
    
    double MapPainterIOS::textHeight(const MapParameter& parameter, double fontSize, std::string text){
        double xOff;
        double yOff;
        double width;
        double height;
        GetTextDimension(parameter,fontSize,text,xOff,yOff,width,height);
        return height;
    }
    
    /*
     * DrawLabel(const Projection& projection, const MapParameter& parameter, const LabelData& label)
     */
    void MapPainterIOS::DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const LabelData& label){
        
        if (dynamic_cast<const TextStyle*>(label.style.Get())!=NULL) {
            
            if(label.y <= MAP_PAINTER_Y_LABEL_MARGIN ||
               label.y >= projection.GetHeight() - MAP_PAINTER_Y_LABEL_MARGIN){
                return;
            }
            
            const TextStyle* style=dynamic_cast<const TextStyle*>(label.style.Get());
            double           r=style->GetTextColor().GetR();
            double           g=style->GetTextColor().GetG();
            double           b=style->GetTextColor().GetB();
            
            CGContextSaveGState(cg);
            CGContextSetTextDrawingMode(cg, kCGTextFill);
            Font *font = GetFont(parameter,label.fontSize);
            NSString *str = [NSString stringWithCString:label.text.c_str() encoding:NSUTF8StringEncoding];
            //std::cout << "label : "<< label.text << " font size : " << label.fontSize << std::endl;
            
            if (style->GetStyle()==TextStyle::normal) {
                CGContextSetRGBFillColor(cg, r, g, b, label.alpha);
#if TARGET_OS_IPHONE
                [str drawAtPoint:CGPointMake(label.x, label.y) withFont:font];
#else
                NSColor *color = [NSColor colorWithSRGBRed:style->GetTextColor().GetR() green:style->GetTextColor().GetG() blue:style->GetTextColor().GetB() alpha:style->GetTextColor().GetA()];
                NSDictionary *attrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:font,NSFontAttributeName,color,NSForegroundColorAttributeName, nil];
                [str drawAtPoint:CGPointMake(label.x, label.y) withAttributes:attrsDictionary];
                
#endif
            } else if (style->GetStyle()==TextStyle::emphasize) {
                CGContextSetRGBFillColor(cg, r, g, b, label.alpha);
                CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
                CGColorRef haloColor = CGColorCreate(colorSpace, (CGFloat[]){ 1, 1, 1, static_cast<CGFloat>(label.alpha) });
                CGContextSetShadowWithColor( cg, CGSizeMake( 0.0, 0.0 ), 2.0f, haloColor );
#if TARGET_OS_IPHONE
                [str drawAtPoint:CGPointMake(label.x, label.y) withFont:font];
#else
                NSColor *color = [NSColor colorWithSRGBRed:style->GetTextColor().GetR() green:style->GetTextColor().GetG() blue:style->GetTextColor().GetB() alpha:style->GetTextColor().GetA()];
                NSDictionary *attrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:font,NSFontAttributeName,color,NSForegroundColorAttributeName, nil];
                [str drawAtPoint:CGPointMake(label.x, label.y) withAttributes:attrsDictionary];
#endif
                CGColorRelease(haloColor);
                CGColorSpaceRelease(colorSpace);
            }
            CGContextRestoreGState(cg);
        }
        
        else if (dynamic_cast<const ShieldStyle*>(label.style.Get())!=NULL) {
            const ShieldStyle* style=dynamic_cast<const ShieldStyle*>(label.style.Get());
            
            
            if(label.bx1 <= MAP_PAINTER_PLATE_LABEL_MARGIN ||
               label.by1 <= MAP_PAINTER_PLATE_LABEL_MARGIN ||
               label.bx2 >= projection.GetWidth()-MAP_PAINTER_PLATE_LABEL_MARGIN ||
               label.by2 >= projection.GetHeight()-MAP_PAINTER_PLATE_LABEL_MARGIN
               ){
                return;
            }
            CGContextSaveGState(cg);
            CGContextSetRGBFillColor(cg,
                                     style->GetBgColor().GetR(),
                                     style->GetBgColor().GetG(),
                                     style->GetBgColor().GetB(),
                                     1);
            CGContextSetRGBStrokeColor(cg,style->GetBorderColor().GetR(),
                                       style->GetBorderColor().GetG(),
                                       style->GetBorderColor().GetB(),
                                       style->GetBorderColor().GetA());
            CGContextAddRect(cg, CGRectMake(label.bx1,
                                            label.by1,
                                            label.bx2-label.bx1+1,
                                            label.by2-label.by1+1));
            CGContextDrawPath(cg, kCGPathFillStroke);
            
            CGContextAddRect(cg, CGRectMake(label.bx1+2,
                                            label.by1+2,
                                            label.bx2-label.bx1+1-4,
                                            label.by2-label.by1+1-4));
            CGContextDrawPath(cg, kCGPathStroke);
            
            
            Font *font = GetFont(parameter,label.fontSize);
            NSString *str = [NSString stringWithUTF8String:label.text.c_str()];
#if TARGET_OS_IPHONE
            CGContextSetRGBFillColor(cg,style->GetTextColor().GetR(),
                                     style->GetTextColor().GetG(),
                                     style->GetTextColor().GetB(),
                                     style->GetTextColor().GetA());
            [str drawAtPoint:CGPointMake(label.x, label.y) withFont:font];
#else
            NSColor *color = [NSColor colorWithSRGBRed:style->GetTextColor().GetR() green:style->GetTextColor().GetG() blue:style->GetTextColor().GetB() alpha:style->GetTextColor().GetA()];
            NSDictionary *attrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:font,NSFontAttributeName,color,NSForegroundColorAttributeName, nil];
            [str drawAtPoint:CGPointMake(label.x, label.y) withAttributes:attrsDictionary];
#endif
            CGContextRestoreGState(cg);
            
        }
    }
    
    double MapPainterIOS::pathLength(size_t transStart, size_t transEnd){
        double len = 0.0;
        double deltaX, deltaY;
        for(size_t j=transStart; j<transEnd; j++) {
            deltaX = coordBuffer->buffer[j].GetX() - coordBuffer->buffer[j+1].GetX();
            deltaY = coordBuffer->buffer[j].GetY() - coordBuffer->buffer[j+1].GetX();
            len += sqrt(deltaX*deltaX + deltaY*deltaY);
        }
        return len;
    }

    XYSlope MapPainterIOS::originAndSlopeAlongPath(CGFloat l, CGFloat nextW, size_t transStart, size_t transEnd,
                                                   CGFloat &posX, CGFloat &posY, size_t &i, CGFloat &currentL) {
        CGFloat widthToGo = l - currentL;
        XYSlope pos = {-1, -1, -1};
        Pt pos1 = {NAN, NAN};
        
        size_t pathSize = labs(transEnd - transStart);
        int direction = (transStart < transEnd) ? 1 : -1;

        while(widthToGo+nextW >= 0) {
            //out of path, return invalid point
            if(i+direction >= pathSize) {
                return pos;
            }
            
            CGFloat nextX = coordBuffer->buffer[transStart+(i+1)*direction].GetX();
            CGFloat nextY = coordBuffer->buffer[transStart+(i+1)*direction].GetY();;
            
            CGFloat xDiff = (nextX - posX)*direction;
            CGFloat yDiff = (nextY - posY)*direction;
            CGFloat distToNext = sqrt(xDiff*xDiff + yDiff*yDiff);
            
            CGFloat fracToGo1 = widthToGo/distToNext;
            if(fracToGo1 < 1 && std::isnan(pos1.x)){
                pos1.x = posX+(xDiff*fracToGo1);
                pos1.y = posY+(yDiff*fracToGo1);
            }
            
            CGFloat fracToGo2 = (widthToGo+nextW)/distToNext;
            //point is before next point in path, interpolate the answer
            if(fracToGo2 < 1) {
                pos.x = posX+(xDiff*fracToGo2);
                pos.y = posY+(yDiff*fracToGo2);
                pos.slope = atan2(pos.y-pos1.y, pos.x-pos1.x);
                pos.x = (pos.x + pos1.x)/2;
                pos.y = (pos.y + pos1.y)/2;
                return pos;
            }
            
            //advance to next point on the path
            widthToGo -= distToNext;
            currentL += distToNext;
            posX = nextX;
            posY = nextY;
            i+=direction;
        }
        return pos;
    }
    
    void MapPainterIOS::DrawContourSymbol(const Projection& projection,
                                            const MapParameter& parameter,
                                            const Symbol& symbol,
                                            double space,
                                            size_t transStart, size_t transEnd){
        double lineLength=pathLength(transStart, transEnd);
        CGContextSaveGState(cg);
        
        double minX;
        double minY;
        double maxX;
        double maxY;
        
        symbol.GetBoundingBox(minX,minY,maxX,maxY);
        
        double width=ConvertWidthToPixel(parameter,maxX-minX);
        double height=ConvertWidthToPixel(parameter,maxY-minY);
        CGAffineTransform transform=CGAffineTransformMake(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
        for (std::list<DrawPrimitiveRef>::const_iterator p=symbol.GetPrimitives().begin();
             p!=symbol.GetPrimitives().end();
             ++p) {
            FillStyleRef fillStyle=(*p)->GetFillStyle();
            
            CGContextBeginPath(cg);
            DrawFillStyle(projection,
                          parameter,
                          *fillStyle);
            
            CGFloat lenUpToNow = space/2;
            CGFloat posX = coordBuffer->buffer[transStart].GetX();
            CGFloat posY = coordBuffer->buffer[transStart].GetY();
            size_t segment = 0;
            CGFloat currentL = 0;
            while (lenUpToNow+width<lineLength) {
                XYSlope origin = originAndSlopeAlongPath(lenUpToNow, width, transStart, transEnd, posX, posY, segment, currentL);
                CGContextSaveGState(cg);
                
                CGContextTranslateCTM(cg, origin.x, origin.y);
                CGAffineTransform ct = CGAffineTransformConcat(transform, CGAffineTransformMakeRotation(origin.slope));
                CGContextConcatCTM(cg, ct);
                DrawPrimitivePath(projection, parameter, *p,
                                  width,height/2,
                                  minX, minY, maxX, maxY);
                CGContextRestoreGState(cg);
                
                lenUpToNow+=width+space;
            }
            
            CGContextFillPath(cg);
        }
        CGContextRestoreGState(cg);
    }

    /*
     * DrawContourLabel(const Projection& projection,
     *                  const MapParameter& parameter,
     *                  const PathTextStyle& style,
     *                  const std::string& text,
     *                  size_t transStart, size_t transEnd)
     */
    void MapPainterIOS::DrawContourLabel(const Projection& projection,
                          const MapParameter& parameter,
                          const PathTextStyle& style,
                          const std::string& text,
                          size_t transStart, size_t transEnd){
        Font *font = GetFont(parameter,style.GetSize());
        size_t tStart, tEnd;
        if(coordBuffer->buffer[transStart].GetX()<coordBuffer->buffer[transEnd].GetX()){
            tStart = transStart;
            tEnd = transEnd;
        } else {
            tStart = transEnd;
            tEnd = transStart;
        }
        double pathLen = pathLength(transStart, transEnd);
        double textLen = textLength(parameter,style.GetSize()*ConvertWidthToPixel(parameter,parameter.GetFontSize()),text)+MAP_PAINTER_DRAW_CONTOUR_LABEL_MARGIN*2;
        if (textLen > pathLen) {
             return;
         }
        
        CGContextSaveGState(cg);
#if TARGET_OS_IPHONE
        CGContextSetTextDrawingMode(cg, kCGTextFill);
        CGContextSetLineWidth(cg, 1.0);
        CGContextSetRGBFillColor(cg, style.GetTextColor().GetR(), style.GetTextColor().GetG(), style.GetTextColor().GetB(), style.GetTextColor().GetA());
        CGContextSetRGBStrokeColor(cg, 1, 1, 1, 1);
#if __has_feature(objc_arc)
        CGContextSetFont(cg, (__bridge CGFontRef)font);
#else
        CGContextSetFont(cg, (CGFontRef)font);
#endif
#else
        NSColor *color = [NSColor colorWithSRGBRed:style.GetTextColor().GetR() green:style.GetTextColor().GetG() blue:style.GetTextColor().GetB() alpha:style.GetTextColor().GetA()];
        NSDictionary *attrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:font,NSFontAttributeName,color,NSForegroundColorAttributeName, nil];
#endif
        CGAffineTransform ct;
        CGFloat lenUpToNow = MAP_PAINTER_DRAW_CONTOUR_LABEL_MARGIN;
        
        NSString *nsText= [NSString stringWithCString:text.c_str() encoding:NSUTF8StringEncoding];
        CGFloat posX = coordBuffer->buffer[transStart].GetX();
        CGFloat posY = coordBuffer->buffer[transStart].GetY();
        int charIndex = 0;
        size_t segment = 0;
        CGFloat currentL = 0.0;
        double nww,nhh,xOff,yOff;
        for(int i=0;i<[nsText length];i++) {
            
            NSString *str = [nsText substringWithRange:NSMakeRange(i, 1)];
            
            GetTextDimension(parameter,style.GetSize(), [str cStringUsingEncoding:NSUTF8StringEncoding], xOff, yOff, nww, nhh);            
            XYSlope charOrigin = originAndSlopeAlongPath(lenUpToNow, nww, tStart, tEnd, posX, posY, segment, currentL);
            if(charOrigin.x == -1 && charOrigin.y == -1){
                CGContextRestoreGState(cg);
                return;
            }
            //std::cout << " CHARACTER " << [str UTF8String] << " placed at " << charOrigin.x << "," << charOrigin.y << " slope "<< charOrigin.slope << std::endl;
            
            CGContextSaveGState(cg);
            
            CGContextTranslateCTM(cg, charOrigin.x, charOrigin.y);
            ct = CGAffineTransformMakeRotation(charOrigin.slope);
            CGContextConcatCTM(cg, ct);
            
#if TARGET_OS_IPHONE
            [str drawAtPoint:CGPointMake(-nww/2,-nhh/2) withFont:font];
#else
            [str drawAtPoint:CGPointMake(-nww/2,-nhh/2) withAttributes:attrsDictionary];
#endif
            CGContextRestoreGState(cg);
            lenUpToNow += nww;
            
            charIndex++;
        }
        CGContextRestoreGState(cg);
    }

    /*
     *
     * DrawIcon(const IconStyle* style,
     *          double x, double y)
     */
    void MapPainterIOS::DrawIcon(const IconStyle* style,
                  double x, double y){
        size_t idx=style->GetIconId()-1;
        
        assert(idx<images.size());
        assert(images[idx]);
        
        CGRect rect = CGRectMake(x-CGImageGetWidth(images[idx])/2, CGImageGetHeight(images[idx])-y-1.5*CGImageGetHeight(images[idx]), CGImageGetWidth(images[idx]), CGImageGetHeight(images[idx]));
        CGContextSaveGState(cg);
        CGContextScaleCTM(cg, 1.0, -1.0);
        CGContextDrawImage(cg, rect, images[idx]);
        CGContextRestoreGState(cg);
    }
    
    /*
     * DrawPrimitivePath()
     */
    //#define DEBUG_DRAW_POLYGON_IOSX 1
    void MapPainterIOS::DrawPrimitivePath(const Projection& projection,
                                            const MapParameter& parameter,
                                            const DrawPrimitiveRef& p,
                                            double x, double y,
                                            double minX,
                                            double minY,
                                            double maxX,
                                            double maxY)
    {
        DrawPrimitive* primitive=p.Get();
        double         centerX=maxX-minX;
        double         centerY=maxY-minY;
        
        if (dynamic_cast<PolygonPrimitive*>(primitive)!=NULL) {
            PolygonPrimitive* polygon=dynamic_cast<PolygonPrimitive*>(primitive);
#ifdef DEBUG_DRAW_POLYGON_IOSX
            CGContextSaveGState(cg);
            CGContextSetLineWidth(cg, 1.0);
            CGContextSetRGBStrokeColor(cg, 1.0, 0, 0, 1);
            CGContextBeginPath(cg);
            CGContextMoveToPoint(cg, x, y-10);
            CGContextAddLineToPoint(cg, x, y+10);
            CGContextMoveToPoint(cg, x-10, y);
            CGContextAddLineToPoint(cg, x+10,y);
            CGContextDrawPath(cg, kCGPathStroke);
            CGContextRestoreGState(cg);
#endif
            CGContextBeginPath(cg);
            for (std::list<Coord>::const_iterator pixel=polygon->GetCoords().begin();
                 pixel!=polygon->GetCoords().end();
                 ++pixel) {
                if (pixel==polygon->GetCoords().begin()) {
                    CGContextMoveToPoint(cg,x+ConvertWidthToPixel(parameter,pixel->x-centerX),
                                         y+ConvertWidthToPixel(parameter,maxY-pixel->y-centerY));
                } else {
                    CGContextAddLineToPoint(cg,x+ConvertWidthToPixel(parameter,pixel->x-centerX),
                                            y+ConvertWidthToPixel(parameter,maxY-pixel->y-centerY));
                }
            }
            
            CGContextDrawPath(cg, kCGPathFill);
        }
        else if (dynamic_cast<RectanglePrimitive*>(primitive)!=NULL) {
            RectanglePrimitive* rectangle=dynamic_cast<RectanglePrimitive*>(primitive);
            CGRect rect = CGRectMake(x+ConvertWidthToPixel(parameter,rectangle->GetTopLeft().x-centerX),
                                     y+ConvertWidthToPixel(parameter,maxY-rectangle->GetTopLeft().y-centerY),
                                     ConvertWidthToPixel(parameter,rectangle->GetWidth()),
                                     ConvertWidthToPixel(parameter,rectangle->GetHeight()));
            CGContextAddRect(cg,rect);
            CGContextDrawPath(cg, kCGPathFill);
        }
        else if (dynamic_cast<CirclePrimitive*>(primitive)!=NULL) {
            CirclePrimitive* circle=dynamic_cast<CirclePrimitive*>(primitive);
            CGRect rect = CGRectMake(x+ConvertWidthToPixel(parameter,circle->GetCenter().x-centerX),
                                     y+ConvertWidthToPixel(parameter,maxY-circle->GetCenter().y-centerY),
                                     ConvertWidthToPixel(parameter,circle->GetRadius()),
                                     ConvertWidthToPixel(parameter,circle->GetRadius()));
            CGContextAddEllipseInRect(cg, rect);
            CGContextDrawPath(cg, kCGPathFill);
        }
    }
    
    /*
     * DrawSymbol(const Projection& projection,
     *            const MapParameter& parameter,
     *            const SymbolRef& symbol,
     *            double x, double y)
     */
    void MapPainterIOS::DrawSymbol(const Projection& projection,
                    const MapParameter& parameter,
                    const Symbol& symbol,
                    double x, double y){
        
        double minX;
        double minY;
        double maxX;
        double maxY;

        symbol.GetBoundingBox(minX,minY,maxX,maxY);
        
        CGContextSaveGState(cg);
        
        for (std::list<DrawPrimitiveRef>::const_iterator p=symbol.GetPrimitives().begin();
             p!=symbol.GetPrimitives().end();
             ++p) {
            FillStyleRef fillStyle=(*p)->GetFillStyle();
            
            DrawFillStyle(projection,
                          parameter,
                          *fillStyle);

            DrawPrimitivePath(projection,
                              parameter,
                              *p,
                              x,y,
                              minX,
                              minY,
                              maxX,
                              maxY);
            
        }
        CGContextRestoreGState(cg);
    }
    
    /*
     * DrawPath(const Projection& projection,
     *          const MapParameter& parameter,
     *          const Color& color,
     *          double width,
     *          const std::vector<double>& dash,
     *          CapStyle startCap,
     *          CapStyle endCap,
     *          size_t transStart, size_t transEnd)
     */
    void MapPainterIOS::DrawPath(const Projection& projection,
                  const MapParameter& parameter,
                  const Color& color,
                  double width,
                  const std::vector<double>& dash,
                  LineStyle::CapStyle startCap,
                  LineStyle::CapStyle endCap,
                  size_t transStart, size_t transEnd){
        CGContextSaveGState(cg);
        CGContextSetRGBStrokeColor(cg, color.GetR(), color.GetG(), color.GetB(), color.GetA());
        CGContextSetLineWidth(cg, width);
        CGContextSetLineJoin(cg, kCGLineJoinRound);

        if (dash.empty()) {
            CGContextSetLineDash(cg, 0.0, NULL, 0);
        } else {
            CGFloat *dashes = (CGFloat *)malloc(sizeof(CGFloat)*dash.size());
            for (size_t i=0; i<dash.size(); i++) {
                dashes[i] = dash[i];
            }
            CGContextSetLineDash(cg, 0.0, dashes, dash.size());
            free(dashes); dashes = NULL;
            CGContextSetLineCap(cg, kCGLineCapButt);
        }
        CGContextBeginPath(cg);
        CGContextMoveToPoint(cg,coordBuffer->buffer[transStart].GetX(),coordBuffer->buffer[transStart].GetY());
        for (size_t i=transStart+1; i<=transEnd; i++) {
            CGContextAddLineToPoint (cg,coordBuffer->buffer[i].GetX(),coordBuffer->buffer[i].GetY());
        }
        CGContextStrokePath(cg);
        if (startCap==LineStyle::capRound) {
            CGContextSetRGBFillColor(cg, color.GetR(), color.GetG(), color.GetB(), color.GetA());
            CGContextFillEllipseInRect(cg, CGRectMake(coordBuffer->buffer[transStart].GetX()-width/2,
                                                     coordBuffer->buffer[transStart].GetY()-width/2,
                                                     width,width));
        }
        if (endCap==LineStyle::capRound) {
            CGContextSetRGBFillColor(cg, color.GetR(), color.GetG(), color.GetB(), color.GetA());
            CGContextFillEllipseInRect(cg, CGRectMake(coordBuffer->buffer[transEnd].GetX()-width/2,
                                                     coordBuffer->buffer[transEnd].GetY()-width/2,
                                                     width,width));
        }
        CGContextRestoreGState(cg);
    }
        
    /*
     * SetBrush(const Projection& projection,
     *          const MapParameter& parameter,
     *          const FillStyle& fillStyle)
     */
    
    void MapPainterIOS::DrawFillStyle(const Projection& projection,
                                        const MapParameter& parameter,
                                        const FillStyle& fillStyle) {
        double borderWidth=ConvertWidthToPixel(parameter,fillStyle.GetBorderWidth());

        if (fillStyle.HasPattern() &&
            projection.GetMagnification()>=fillStyle.GetPatternMinMag() &&
            HasPattern(parameter,fillStyle)) {
            CGColorSpaceRef sp = CGColorSpaceCreatePattern(NULL);
            CGContextSetFillColorSpace (cg, sp);
            CGColorSpaceRelease (sp);
            CGFloat components = 1.0;
            size_t patternIndex = fillStyle.GetPatternId()-1;
            CGPatternRef pattern = patterns[patternIndex];
            CGContextSetFillPattern(cg, pattern, &components);
        } else if (fillStyle.GetFillColor().IsVisible()) {

            CGContextSetRGBFillColor(cg, fillStyle.GetFillColor().GetR(), fillStyle.GetFillColor().GetG(),
                                     fillStyle.GetFillColor().GetB(), fillStyle.GetFillColor().GetA());
        } else {
            CGContextSetRGBFillColor(cg,0,0,0,0);
        }
        
        if (borderWidth>=parameter.GetLineMinWidthPixel()) {
            CGContextSetRGBStrokeColor(cg,fillStyle.GetBorderColor().GetR(),
                                          fillStyle.GetBorderColor().GetG(),
                                          fillStyle.GetBorderColor().GetB(),
                                          fillStyle.GetBorderColor().GetA());
            CGContextSetLineWidth(cg, borderWidth);
            
            if (fillStyle.GetBorderDash().empty()) {
                CGContextSetLineDash(cg, 0.0, NULL, 0);
            }
            else {
                CGFloat *dashes = (CGFloat *)malloc(sizeof(CGFloat)*fillStyle.GetBorderDash().size());
                for (size_t i=0; i<fillStyle.GetBorderDash().size(); i++) {
                    dashes[i] = fillStyle.GetBorderDash()[i];
                }
                CGContextSetLineDash(cg, 0.0, dashes, fillStyle.GetBorderDash().size());
                free(dashes); dashes = NULL;
                CGContextSetLineCap(cg, kCGLineCapButt);
            }
        }
        else {
            CGContextSetRGBStrokeColor(cg,0,0,0,0);
        }
    }

    
    /*
     * DrawArea(const Projection& projection,
     *          const MapParameter& parameter,
     *          const AreaData& area)
     */
    void MapPainterIOS::DrawArea(const Projection& projection,
                                 const MapParameter& parameter,
                                 const AreaData& area){

        CGContextSaveGState(cg);
        CGContextSetLineWidth(cg, 2);
        CGContextSetRGBStrokeColor(cg, area.fillStyle->GetFillColor().GetR(), area.fillStyle->GetFillColor().GetG(),
                                   area.fillStyle->GetFillColor().GetB(), area.fillStyle->GetFillColor().GetA());
        CGPathDrawingMode drawingMode = kCGPathFillStroke;
        CGContextBeginPath(cg);
        CGContextMoveToPoint(cg,coordBuffer->buffer[area.transStart].GetX(),coordBuffer->buffer[area.transStart].GetY());
        for (size_t i=area.transStart+1; i<=area.transEnd; i++) {
            CGContextAddLineToPoint(cg,coordBuffer->buffer[i].GetX(),coordBuffer->buffer[i].GetY());
        }
        CGContextClosePath(cg);
        
        if (!area.clippings.empty()) {
            for (std::list<MapPainter::PolyData>::const_iterator c=area.clippings.begin();
                 c!=area.clippings.end();
                 c++) {
                const MapPainter::PolyData& data=*c;
                CGContextMoveToPoint(cg,coordBuffer->buffer[area.transStart].GetX(),coordBuffer->buffer[area.transStart].GetY());
                for (size_t i=data.transStart+1; i<=data.transEnd; i++) {
                    CGContextAddLineToPoint(cg,coordBuffer->buffer[i].GetX(),coordBuffer->buffer[i].GetY());
                }
                CGContextClosePath(cg);
            }
        }
        
        double borderWidth=MapPainter::ConvertWidthToPixel(parameter,area.fillStyle->GetBorderWidth());
        if (borderWidth>=parameter.GetLineMinWidthPixel()) {
            CGContextSetRGBStrokeColor(cg, area.fillStyle->GetBorderColor().GetR(), area.fillStyle->GetBorderColor().GetG(),
                                       area.fillStyle->GetBorderColor().GetB(), area.fillStyle->GetBorderColor().GetA());
            CGContextSetLineWidth(cg, borderWidth);
            
            if (area.fillStyle->GetBorderDash().empty()) {
                CGContextSetLineDash(cg, 0.0, NULL, 0);     // SolidLine
            } else {
                CGFloat *dashes = (CGFloat *)malloc(sizeof(CGFloat)*area.fillStyle->GetBorderDash().size());
                for (size_t i=0; i<area.fillStyle->GetBorderDash().size(); i++) {
                    dashes[i] = area.fillStyle->GetBorderDash()[i];
                }
                CGContextSetLineDash(cg, 0.0, dashes, area.fillStyle->GetBorderDash().size());
                free(dashes); dashes = NULL;
                CGContextSetLineCap(cg, kCGLineCapButt);   // CapButt
            }
            drawingMode = kCGPathFillStroke;
        } 
        //
        DrawFillStyle(projection, parameter,*area.fillStyle);
        CGContextDrawPath(cg, drawingMode);
        CGContextRestoreGState(cg);
    }
    
    /*
     * DrawGround(const Projection& projection,
     *            const MapParameter& parameter,
     *            const FillStyle& style)
     */
    void MapPainterIOS::DrawGround(const Projection& projection,
                                   const MapParameter& parameter,
                                   const FillStyle& style){
        CGContextSaveGState(cg);
        CGContextBeginPath(cg);
        const Color &borderColor = style.GetBorderColor();
        CGContextSetRGBStrokeColor(cg, borderColor.GetR(), borderColor.GetG(), borderColor.GetB(), borderColor.GetA());
        const Color &fillColor = style.GetFillColor();
        CGContextSetRGBFillColor(cg, fillColor.GetR(), fillColor.GetG(), fillColor.GetB(), fillColor.GetA());
        CGContextAddRect(cg, CGRectMake(0,0,projection.GetWidth(),projection.GetHeight()));
        CGContextDrawPath(cg, kCGPathFillStroke);
        CGContextRestoreGState(cg);
    }
    
}
