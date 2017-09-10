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

#if ! __has_feature(objc_arc)
#error This file must be compiled with ARC. Either turn on ARC for the project or use -fobjc-arc flag
#endif

namespace osmscout {
        
    MapPainterIOS::MapPainterIOS(const StyleConfigRef& styleConfig)
    : MapPainter(styleConfig, new CoordBufferImpl<Vertex2D>()),
    coordBuffer((CoordBufferImpl<Vertex2D>*)transBuffer.buffer)
    {
#if TARGET_OS_IPHONE
        contentScale = [[UIScreen mainScreen] scale];
#else
        contentScale = 1.0;
#endif
    }
        
    MapPainterIOS::~MapPainterIOS(){
        for(std::vector<Image>::const_iterator image=images.begin(); image<images.end();image++){
            CGImageRelease(*image);
        }
        for(std::vector<Image>::const_iterator image=patternImages.begin(); image<patternImages.end();image++){
            CGImageRelease(*image);
        }
    }
    
    Font *MapPainterIOS::GetFont(const Projection& projection,
                                 const MapParameter& parameter,
                                 double fontSize)
    {
        std::map<size_t,Font *>::const_iterator f;
        
        fontSize=fontSize*projection.ConvertWidthToPixel(parameter.GetFontSize());
        
        f=fonts.find(fontSize);
        
        if (f!=fonts.end()) {
            return f->second;
        }
        
        Font *font = [Font fontWithName:[NSString stringWithUTF8String: parameter.GetFontName().c_str()] size:fontSize];
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
        cg = paintCG;
        Draw(projection,
             parameter,
             data);
        return true;
    }

    /*
     * DrawGroundTiles(const Projection& projection,
     *                 const MapParameter& parameter,
     *                 const std::list<GroundTile>& groundTiles,
     *                 CGContextRef paintCG)
     */
    void MapPainterIOS::DrawGroundTiles(const Projection& projection,
                                        const MapParameter& parameter,
                                        const std::list<GroundTile>& groundTiles,
                                        CGContextRef paintCG){
        cg = paintCG;
        
        FillStyleRef      landFill;
        
        styleConfig->GetLandFillStyle(projection,
                                      landFill);
        
        if (!landFill) {
            return;
        }
        
        if (parameter.GetRenderBackground()) {
            DrawGround(projection,
                       parameter,
                       *landFill);
        }
        
        FillStyleRef       seaFill;
        FillStyleRef       coastFill;
        FillStyleRef       unknownFill;
        LineStyleRef       coastlineLine;
        std::vector<Point> points;
        size_t             start=0;
        size_t             end=0;
        
        styleConfig->GetSeaFillStyle(projection,
                                     seaFill);
        styleConfig->GetCoastFillStyle(projection,
                                       coastFill);
        styleConfig->GetUnknownFillStyle(projection,
                                         unknownFill);
        styleConfig->GetCoastlineLineStyle(projection,
                                           coastlineLine);
        
        if (!seaFill) {
            return;
        }
        
        double             errorTolerancePixel=projection.ConvertWidthToPixel(parameter.GetOptimizeErrorToleranceMm());
        FeatureValueBuffer coastlineSegmentAttributes;
        
        for (const auto& tile : groundTiles) {
            AreaData areaData;
            
            if (tile.type==GroundTile::unknown &&
                !parameter.GetRenderUnknowns()) {
                continue;
            }
            
            switch (tile.type) {
                case GroundTile::land:
                    areaData.fillStyle=landFill;
                    break;
                case GroundTile::water:
                    areaData.fillStyle=seaFill;
                    break;
                case GroundTile::coast:
                    areaData.fillStyle=coastFill;
                    break;
                case GroundTile::unknown:
                    areaData.fillStyle=unknownFill;
                    break;
            }
            if (!areaData.fillStyle){
                continue;
            }
            
            GeoCoord minCoord(tile.yAbs*tile.cellHeight-90.0,
                              tile.xAbs*tile.cellWidth-180.0);
            GeoCoord maxCoord(minCoord.GetLat()+tile.cellHeight,
                              minCoord.GetLon()+tile.cellWidth);
            
            areaData.boundingBox.Set(minCoord,maxCoord);
            
            if (tile.coords.empty()) {
                // Fill the cell completely with the fill for the given cell type
                points.resize(5);
                
                points[0].SetCoord(areaData.boundingBox.GetMinCoord());
                points[1].SetCoord(GeoCoord(areaData.boundingBox.GetMinCoord().GetLat(),
                                            areaData.boundingBox.GetMaxCoord().GetLon()));
                points[2].SetCoord(areaData.boundingBox.GetMaxCoord());
                points[3].SetCoord(GeoCoord(areaData.boundingBox.GetMaxCoord().GetLat(),
                                            areaData.boundingBox.GetMinCoord().GetLon()));
                points[4]=points[0];
                
                transBuffer.transPolygon.TransformArea(projection,
                                                       TransPolygon::none,
                                                       points,
                                                       errorTolerancePixel);
                
                size_t s=transBuffer.transPolygon.GetStart();
                
                start=transBuffer.buffer->PushCoord(floor(transBuffer.transPolygon.points[s+0].x),
                                                    ceil(transBuffer.transPolygon.points[s+0].y));
                
                
                transBuffer.buffer->PushCoord(ceil(transBuffer.transPolygon.points[s+1].x),
                                              ceil(transBuffer.transPolygon.points[s+1].y));
                
                transBuffer.buffer->PushCoord(ceil(transBuffer.transPolygon.points[s+2].x),
                                              floor(transBuffer.transPolygon.points[s+2].y));
                
                transBuffer.buffer->PushCoord(floor(transBuffer.transPolygon.points[s+3].x),
                                              floor(transBuffer.transPolygon.points[s+3].y));
                
                end=transBuffer.buffer->PushCoord(floor(transBuffer.transPolygon.points[s+4].x),
                                                  ceil(transBuffer.transPolygon.points[s+4].y));
            } else {
                points.resize(tile.coords.size());
                
                for (size_t i=0; i<tile.coords.size(); i++) {
                    double lat;
                    double lon;
                    
                    lat=areaData.boundingBox.GetMinCoord().GetLat()+tile.coords[i].y*tile.cellHeight/GroundTile::Coord::CELL_MAX;
                    lon=areaData.boundingBox.GetMinCoord().GetLon()+tile.coords[i].x*tile.cellWidth/GroundTile::Coord::CELL_MAX;
                    
                    points[i].SetCoord(GeoCoord(lat,lon));
                }
                
                transBuffer.transPolygon.TransformArea(projection,
                                                       TransPolygon::none,
                                                       points,
                                                       errorTolerancePixel);
                
                for (size_t i=transBuffer.transPolygon.GetStart(); i<=transBuffer.transPolygon.GetEnd(); i++) {
                    double x,y;
                    
                    if (tile.coords[i].x==0) {
                        x=floor(transBuffer.transPolygon.points[i].x);
                    }
                    else if (tile.coords[i].x==GroundTile::Coord::CELL_MAX) {
                        x=ceil(transBuffer.transPolygon.points[i].x);
                    }
                    else {
                        x=transBuffer.transPolygon.points[i].x;
                    }
                    
                    if (tile.coords[i].y==0) {
                        y=ceil(transBuffer.transPolygon.points[i].y);
                    }
                    else if (tile.coords[i].y==GroundTile::Coord::CELL_MAX) {
                        y=floor(transBuffer.transPolygon.points[i].y);
                    }
                    else {
                        y=transBuffer.transPolygon.points[i].y;
                    }
                    
                    size_t idx=transBuffer.buffer->PushCoord(x,y);
                    
                    if (i==transBuffer.transPolygon.GetStart()) {
                        start=idx;
                    }
                    else if (i==transBuffer.transPolygon.GetEnd()) {
                        end=idx;
                    }
                }
                
                if (coastlineLine) {
                    size_t lineStart=0;
                    size_t lineEnd;
                    
                    while (lineStart<tile.coords.size()) {
                        while (lineStart<tile.coords.size() &&
                               !tile.coords[lineStart].coast) {
                            lineStart++;
                        }
                        
                        if (lineStart>=tile.coords.size()) {
                            continue;
                        }
                        
                        lineEnd=lineStart;
                        
                        while (lineEnd<tile.coords.size() &&
                               tile.coords[lineEnd].coast) {
                            lineEnd++;
                        }
                        
                        if (lineStart!=lineEnd) {
                            WayData data;
                            
                            data.buffer=&coastlineSegmentAttributes;
                            data.layer=0;
                            data.lineStyle=coastlineLine;
                            data.wayPriority=std::numeric_limits<int>::max();
                            data.transStart=start+lineStart;
                            data.transEnd=start+lineEnd;
                            data.lineWidth=GetProjectedWidth(projection,
                                                             projection.ConvertWidthToPixel(coastlineLine->GetDisplayWidth()),
                                                             coastlineLine->GetWidth());
                            data.startIsClosed=false;
                            data.endIsClosed=false;
                            
                            DrawWay(*styleConfig,
                                    projection,
                                    parameter,
                                    data);
                        }
                        
                        lineStart=lineEnd+1;
                    }
                }
            }
            
            areaData.ref=ObjectFileRef();
            areaData.transStart=start;
            areaData.transEnd=end;
            
            DrawArea(projection,parameter,areaData);
        }
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
            //std::cout << "Trying to Load image " << filename << std::endl;
            
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
                //std::cout << "Loaded image '" << filename << "'" << std::endl;

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
        
        if (idx<patternImages.size() &&
            patternImages[idx]) {

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
#if TARGET_OS_IPHONE
                CGImageRef imgRef= [image CGImage];
#else
                NSRect rect = CGRectMake(0, 0, 16, 16);
                NSImageRep *imageRep = [image bestRepresentationForRect:rect context:[NSGraphicsContext currentContext] hints:0];
                NSInteger imgWidth = [imageRep pixelsWide];
                NSInteger imgHeight = [imageRep pixelsHigh];
                rect = CGRectMake(0, 0, imgWidth, imgHeight);
                CGImageRef imgRef= [image CGImageForProposedRect:&rect context:[NSGraphicsContext currentContext] hints:NULL];
#endif
                CGImageRetain(imgRef);
                patternImages.resize(patternImages.size()+1,imgRef);
                style.SetPatternId(patternImages.size());
                //std::cout << "Loaded image " << filename << " (" <<  imgWidth << "x" << imgHeight <<  ") => id " << style.GetPatternId() << std::endl;
                return true;
            }
        }
        
        std::cerr << "ERROR while loading icon file '" << style.GetPatternName() << "'" << std::endl;
        style.SetPatternId(std::numeric_limits<size_t>::max());
        
        return false;
    }
    
    /**
     * Returns the height of the font.
     */
    void MapPainterIOS::GetFontHeight(const Projection& projection,
                                      const MapParameter& parameter,
                                      double fontSize,
                                      double& height){
        Font *font = GetFont(projection,parameter,fontSize);
#if TARGET_OS_IPHONE
        CGSize size = [@"Aj" sizeWithFont:font];
#else
        NSRect stringBounds = [@"Aj" boundingRectWithSize:CGSizeMake(500, 50) options:NSStringDrawingUsesLineFragmentOrigin attributes:[NSDictionary dictionaryWithObject:font forKey:NSFontAttributeName]];
        CGSize size = stringBounds.size;
#endif
        height = size.height;
    }
    
    /*
     * GetTextDimension()
     */
    void MapPainterIOS::GetTextDimension(const Projection& projection,
                                         const MapParameter& parameter,
                                         double objectWidth,
                                         double fontSize,
                                         const std::string& text,
                                         double& xOff,
                                         double& yOff,
                                         double& width,
                                         double& height){
        Font *font = GetFont(projection,parameter,fontSize);
        NSString *str = [NSString stringWithUTF8String:text.c_str()];
        CGRect rect = CGRectZero;

#if TARGET_OS_IPHONE
        NSMutableParagraphStyle *textStyle = [[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy];
        textStyle.lineBreakMode = NSLineBreakByWordWrapping;
        textStyle.alignment = NSTextAlignmentCenter;
        
        if(objectWidth > 0){
            CGSize averageFontSize = [@"a" sizeWithFont:font];
            CGFloat proposedWidth = proposedLabelWidth(parameter,
                                                       averageFontSize.width,
                                                       objectWidth,
                                                       text.length());
            
            rect = [str boundingRectWithSize: CGSizeMake(proposedWidth, CGFLOAT_MAX) options:NSStringDrawingUsesLineFragmentOrigin
                                  attributes:@{NSFontAttributeName:font, NSParagraphStyleAttributeName:textStyle}
                                     context:nil];
        } else {
            CGSize size = [str sizeWithAttributes:@{NSFontAttributeName:font, NSParagraphStyleAttributeName:textStyle}];
            rect.size.width = size.width;
            rect.size.height = size.height;
        }
#else
        NSRect stringBounds = [str boundingRectWithSize:CGSizeMake(500, 50) options:NSStringDrawingUsesLineFragmentOrigin attributes:[NSDictionary dictionaryWithObject:font forKey:NSFontAttributeName]];
        CGSize size = stringBounds.size;
        rect.size.width = size.width;
        rect.size.height = size.height;
#endif
        xOff = rect.origin.x;
        yOff = rect.origin.y;
        width = rect.size.width;
        height = rect.size.height;
    }
 
    double MapPainterIOS::textLength(const Projection& projection, const MapParameter& parameter, double fontSize, std::string text){
        double xOff;
        double yOff;
        double width;
        double height;
        GetTextDimension(projection,parameter,/*objectWidth*/-1,fontSize,text,xOff,yOff,width,height);
        return width;
    }
    
    double MapPainterIOS::textHeight(const Projection& projection, const MapParameter& parameter, double fontSize, std::string text){
        double xOff;
        double yOff;
        double width;
        double height;
        GetTextDimension(projection, parameter,/*objectWidth*/-1,fontSize,text,xOff,yOff,width,height);
        return height;
    }
    
    /*
     * DrawLabel(const Projection& projection, const MapParameter& parameter, const LabelData& label)
     */
    void MapPainterIOS::DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const LabelData& label){
        
        if (dynamic_cast<const TextStyle*>(label.style.get())!=NULL) {
            const TextStyle* style=dynamic_cast<const TextStyle*>(label.style.get());
            double           r=style->GetTextColor().GetR();
            double           g=style->GetTextColor().GetG();
            double           b=style->GetTextColor().GetB();
            
            CGContextSaveGState(cg);
            CGContextSetTextDrawingMode(cg, kCGTextFill);
            Font *font = GetFont(projection, parameter, label.fontSize);
            NSString *str = [NSString stringWithCString:label.text.c_str() encoding:NSUTF8StringEncoding];
            //std::cout << "label : "<< label.text << " font size : " << label.fontSize << std::endl;
            
#if TARGET_OS_IPHONE
            NSMutableParagraphStyle *textStyle = [[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy];
            textStyle.lineBreakMode = NSLineBreakByWordWrapping;
            textStyle.alignment = NSTextAlignmentCenter;
            NSDictionary *attrDict = @{NSFontAttributeName:font, NSParagraphStyleAttributeName:textStyle};
            CGRect rect = CGRectMake(label.bx1, label.by1, label.bx2 - label.bx1, label.by2 - label.by1);
#endif
            
            if (style->GetStyle()==TextStyle::normal) {
                CGContextSetRGBFillColor(cg, r, g, b, label.alpha);
#if TARGET_OS_IPHONE
                [str drawInRect:rect withAttributes:attrDict];
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
                [str drawInRect:rect withAttributes:attrDict];
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
        
        else if (dynamic_cast<const ShieldStyle*>(label.style.get())!=NULL) {
            const ShieldStyle* style=dynamic_cast<const ShieldStyle*>(label.style.get());
            
            
            if(label.bx1 <= MapPainterIOS::plateLabelMargin ||
               label.by1 <= MapPainterIOS::plateLabelMargin ||
               label.bx2 >= projection.GetWidth() - MapPainterIOS::plateLabelMargin ||
               label.by2 >= projection.GetHeight() - MapPainterIOS::plateLabelMargin
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
            
            
            Font *font = GetFont(projection, parameter, label.fontSize);
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
    
    void MapPainterIOS::followPathInit(FollowPathHandle &hnd, Vertex2D &origin, size_t transStart, size_t transEnd,
                                       bool isClosed, bool keepOrientation) {
        hnd.i = 0;
        hnd.nVertex = transEnd - transStart;
        bool isReallyClosed = (coordBuffer->buffer[transStart] == coordBuffer->buffer[transEnd]);
        if(isClosed && !isReallyClosed){
            hnd.nVertex++;
            hnd.closeWay = true;
        } else {
            hnd.closeWay = false;
        }
        if(keepOrientation || coordBuffer->buffer[transStart].GetX()<coordBuffer->buffer[transEnd].GetX()){
            hnd.transStart = transStart;
            hnd.transEnd = transEnd;
        } else {
            hnd.transStart = transEnd;
            hnd.transEnd = transStart;
        }
        hnd.direction = (hnd.transStart < hnd.transEnd) ? 1 : -1;
        origin.Set(coordBuffer->buffer[hnd.transStart].GetX(), coordBuffer->buffer[hnd.transStart].GetY());
    }
    
    bool MapPainterIOS::followPath(FollowPathHandle &hnd, double l, Vertex2D &origin) {
        
        double x = origin.GetX();
        double y = origin.GetY();
        double x2,y2;
        double deltaX, deltaY, len, fracToGo;
        while(hnd.i < hnd.nVertex) {
            if(hnd.closeWay && hnd.nVertex - hnd.i == 1){
                x2 = coordBuffer->buffer[hnd.transStart].GetX();
                y2 = coordBuffer->buffer[hnd.transStart].GetY();
            } else {
                x2 = coordBuffer->buffer[hnd.transStart+(hnd.i+1)*hnd.direction].GetX();
                y2 = coordBuffer->buffer[hnd.transStart+(hnd.i+1)*hnd.direction].GetY();
            }
            deltaX = (x2 - x);
            deltaY = (y2 - y);
            len = sqrt(deltaX*deltaX + deltaY*deltaY);
            
            fracToGo = l/len;
            if(fracToGo <= 1.0) {
                origin.Set(x + deltaX*fracToGo,y + deltaY*fracToGo);
                return true;
            }
            
            //advance to next point on the path
            l -= len;
            x = x2;
            y = y2;
            hnd.i++;
        }
        return false;
    }

    void MapPainterIOS::DrawContourSymbol(const Projection& projection,
                                          const MapParameter& parameter,
                                          const Symbol& symbol,
                                          double space,
                                          size_t transStart, size_t transEnd){
        
        double minX,minY,maxX,maxY;
        symbol.GetBoundingBox(minX,minY,maxX,maxY);
        
        double width=projection.ConvertWidthToPixel(maxX-minX);
        double height=projection.ConvertWidthToPixel(maxY-minY);
        bool isClosed = false;
        CGAffineTransform transform=CGAffineTransformMake(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
        Vertex2D origin;
        double slope;
        double x1,y1,x2,y2,x3,y3;
        FollowPathHandle followPathHnd;
        followPathInit(followPathHnd, origin, transStart, transEnd, isClosed, true);
        if(!isClosed && !followPath(followPathHnd, space/2, origin)){
            return;
        }
        bool loop = true;
        while (loop){
            x1 = origin.GetX();
            y1 = origin.GetY();
            loop = followPath(followPathHnd, width/2, origin);
            if(loop){
                x2 = origin.GetX();
                y2 = origin.GetY();
                loop = followPath(followPathHnd, width/2, origin);
                if(loop){
                    x3 = origin.GetX();
                    y3 = origin.GetY();
                    slope = atan2(y3-y1,x3-x1);
                    CGContextSaveGState(cg);
                    CGContextTranslateCTM(cg, x2, y2);
                    CGAffineTransform ct = CGAffineTransformConcat(transform, CGAffineTransformMakeRotation(slope));
                    CGContextConcatCTM(cg, ct);
                    DrawSymbol(projection, parameter, symbol, 0, -height/2);
                    CGContextRestoreGState(cg);
                    loop = followPath(followPathHnd, space, origin);
                }
            }
        }
    }

    static inline double distSq(const Vertex2D &v1, const Vertex2D &v2){
        double dx = v1.GetX() - v2.GetX();
        double dy = v1.GetY() - v2.GetY();
        return dx*dx+dy*dy;
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
        Font *font = GetFont(projection, parameter, style.GetSize());
        Vertex2D charOrigin;
        FollowPathHandle followPathHnd;
        followPathInit(followPathHnd, charOrigin, transStart, transEnd, false, false);
        if(!followPath(followPathHnd, contourLabelMargin, charOrigin)){
            return;
        }
        
        // check if the same label has been drawn near this one
        Vertex2D textOrigin(charOrigin);
        auto its = wayLabels.equal_range(text);
        for (auto it = its.first; it != its.second; ++it) {
            if(distSq(textOrigin, *it->second) < MapPainterIOS::sameLabelMinDistanceSq){
                return;
            }
        }
        
        CGContextSaveGState(cg);
#if TARGET_OS_IPHONE
        CGContextSetTextDrawingMode(cg, kCGTextFill);
        CGContextSetLineWidth(cg, 1.0);
        CGContextSetRGBFillColor(cg, style.GetTextColor().GetR(), style.GetTextColor().GetG(), style.GetTextColor().GetB(), style.GetTextColor().GetA());
        CGContextSetRGBStrokeColor(cg, 1, 1, 1, 1);
        CGContextSetFont(cg, (__bridge CGFontRef)font);
#else
        NSColor *color = [NSColor colorWithSRGBRed:style.GetTextColor().GetR() green:style.GetTextColor().GetG() blue:style.GetTextColor().GetB() alpha:style.GetTextColor().GetA()];
        NSDictionary *attrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:font,NSFontAttributeName,color,NSForegroundColorAttributeName, nil];
#endif
        
        NSString *nsText= [NSString stringWithCString:text.c_str() encoding:NSUTF8StringEncoding];
        double x1,y1,x2,y2,slope;
        NSUInteger charsCount = [nsText length];
        Vertex2D *coords = new Vertex2D[charsCount];
        double *slopes = new double[charsCount];
        double nww,nhh,xOff,yOff;
        int labelRepeatCount = 0;
        while(labelRepeatCount++ < labelRepeatMaxCount){
            int i = 0;
            while(i<charsCount){
                
                NSString *str = [nsText substringWithRange:NSMakeRange(i, 1)];
                
                GetTextDimension(projection, parameter,/*objectWidth*/-1,style.GetSize(), [str cStringUsingEncoding:NSUTF8StringEncoding], xOff, yOff, nww, nhh);
                x1 = charOrigin.GetX();
                y1 = charOrigin.GetY();
                if(!followPath(followPathHnd,nww, charOrigin)){
                    goto exit;
                }
                x2 = charOrigin.GetX();
                y2 = charOrigin.GetY();
                slope = atan2(y2-y1, x2-x1);
                if(i>0 && fabs(slope - slopes[i-1])>=M_PI_4){
                    i=0;
                    continue;
                }
                coords[i].Set(x1, y1);
                slopes[i] = slope;
                
                if(!followPath(followPathHnd, 2, charOrigin)){
                    goto exit;
                }
                i++;
            }
            CGAffineTransform ct;
            for(int i=0;i<charsCount;i++) {
                NSString *str = [nsText substringWithRange:NSMakeRange(i, 1)];
                CGContextSaveGState(cg);
                CGContextTranslateCTM(cg, coords[i].GetX(),coords[i].GetY());
                ct = CGAffineTransformMakeRotation(slopes[i]);
                CGContextConcatCTM(cg, ct);
#if TARGET_OS_IPHONE
                [str drawAtPoint:CGPointMake(0,-nhh/2) withFont:font];
#else
                [str drawAtPoint:CGPointMake(0,-nhh/2) withAttributes:attrsDictionary];
#endif
                CGContextRestoreGState(cg);
            }
            if(!followPath(followPathHnd, contourLabelSpace, charOrigin)){
                goto exit2;
            }
        }
    exit2:
        // insert this label with its start point in the map
        wayLabels.insert(WayLabelsMap::value_type(text,new Vertex2D(textOrigin)));
    exit:
        delete[] coords;
        delete[] slopes;
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
        
        CGFloat w = CGImageGetWidth(images[idx]);
        CGFloat h = CGImageGetHeight(images[idx]);
        CGRect rect = CGRectMake(x-w/2, -h/2-y, w, h);
        CGContextSaveGState(cg);
        CGContextScaleCTM(cg, 1.0, -1.0);
        CGContextDrawImage(cg, rect, images[idx]);
        CGContextRestoreGState(cg);
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
  
            DrawPrimitive* primitive=p->get();
            double         centerX=(maxX+minX)/2;
            double         centerY=(maxY+minY)/2;
            
            if (dynamic_cast<PolygonPrimitive*>(primitive)!=NULL) {
                PolygonPrimitive* polygon=dynamic_cast<PolygonPrimitive*>(primitive);
                FillStyleRef fillStyle=polygon->GetFillStyle();
                BorderStyleRef borderStyle=polygon->GetBorderStyle();

                if (fillStyle) {
                    SetFill(projection, parameter, *fillStyle);
                } else {
                    CGContextSetRGBFillColor(cg,0,0,0,0);
                }
                
                if (borderStyle) {
                    SetBorder(projection, parameter, *borderStyle);
                } else {
                    CGContextSetRGBStrokeColor(cg,0,0,0,0);
                }
                
                CGContextBeginPath(cg);
                
                for (std::list<Vertex2D>::const_iterator pixel=polygon->GetCoords().begin();
                     pixel!=polygon->GetCoords().end();
                     ++pixel) {
                    if (pixel==polygon->GetCoords().begin()) {
                        CGContextMoveToPoint(cg,x+projection.ConvertWidthToPixel(pixel->GetX()-centerX),
                                             y+projection.ConvertWidthToPixel(maxY-pixel->GetY()-centerY));
                    } else {
                        CGContextAddLineToPoint(cg,x+projection.ConvertWidthToPixel(pixel->GetX()-centerX),
                                                y+projection.ConvertWidthToPixel(maxY-pixel->GetY()-centerY));
                    }
                }
                
                CGContextDrawPath(cg, kCGPathFillStroke);
            }
            else if (dynamic_cast<RectanglePrimitive*>(primitive)!=NULL) {
                RectanglePrimitive* rectangle=dynamic_cast<RectanglePrimitive*>(primitive);
                FillStyleRef fillStyle=rectangle->GetFillStyle();
                BorderStyleRef borderStyle=rectangle->GetBorderStyle();
                if (fillStyle) {
                    SetFill(projection, parameter, *fillStyle);
                } else {
                    CGContextSetRGBFillColor(cg,0,0,0,0);
                }
                
                if (borderStyle) {
                    SetBorder(projection, parameter, *borderStyle);
                } else {
                    CGContextSetRGBStrokeColor(cg,0,0,0,0);
                }
                CGRect rect = CGRectMake(x+projection.ConvertWidthToPixel(rectangle->GetTopLeft().GetX()-centerX),
                                         y+projection.ConvertWidthToPixel(maxY-rectangle->GetTopLeft().GetY()-centerY),
                                         projection.ConvertWidthToPixel(rectangle->GetWidth()),
                                         projection.ConvertWidthToPixel(rectangle->GetHeight()));
                CGContextAddRect(cg,rect);
                CGContextDrawPath(cg, kCGPathFillStroke);
            }
            else if (dynamic_cast<CirclePrimitive*>(primitive)!=NULL) {
                CirclePrimitive* circle=dynamic_cast<CirclePrimitive*>(primitive);
                FillStyleRef fillStyle=circle->GetFillStyle();
                BorderStyleRef borderStyle=circle->GetBorderStyle();
                if (fillStyle) {
                    SetFill(projection, parameter, *fillStyle);
                } else {
                    CGContextSetRGBFillColor(cg,0,0,0,0);
                }
                
                if (borderStyle) {
                    SetBorder(projection, parameter, *borderStyle);
                } else {
                    CGContextSetRGBStrokeColor(cg,0,0,0,0);
                }
                CGRect rect = CGRectMake(x+projection.ConvertWidthToPixel(circle->GetCenter().GetX()-centerX),
                                         y+projection.ConvertWidthToPixel(maxY-circle->GetCenter().GetY()-centerY),
                                         projection.ConvertWidthToPixel(circle->GetRadius()),
                                         projection.ConvertWidthToPixel(circle->GetRadius()));
                CGContextAddEllipseInRect(cg, rect);
                CGContextDrawPath(cg, kCGPathFillStroke);
            }
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
                dashes[i] = dash[i]*width;
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
     * SetFill(const Projection& projection,
     *          const MapParameter& parameter,
     *          const FillStyle& fillStyle)
     */
    void MapPainterIOS::SetFill(const Projection& projection,
                                const MapParameter& parameter,
                                const FillStyle& fillStyle,
                                CGFloat xOffset, CGFloat yOffset) {
        
        if (fillStyle.HasPattern() &&
            projection.GetMagnification()>=fillStyle.GetPatternMinMag() &&
            HasPattern(parameter,fillStyle)) {
            CGColorSpaceRef sp = CGColorSpaceCreatePattern(NULL);
            CGContextSetFillColorSpace (cg, sp);
            CGColorSpaceRelease (sp);
            CGFloat components = 1.0;
            size_t patternIndex = fillStyle.GetPatternId()-1;
            CGFloat imgWidth = CGImageGetWidth(patternImages[patternIndex]);
            CGFloat imgHeight = CGImageGetHeight(patternImages[patternIndex]);
            xOffset = remainder(xOffset, imgWidth);
            yOffset = remainder(yOffset, imgHeight);
            CGPatternRef pattern = CGPatternCreate(patternImages[patternIndex], CGRectMake(0,0, imgWidth, imgHeight), CGAffineTransformTranslate(CGAffineTransformIdentity, xOffset, yOffset), imgWidth, imgHeight, kCGPatternTilingNoDistortion, true, &patternCallbacks);
            CGContextSetFillPattern(cg, pattern, &components);
            CGPatternRelease(pattern);
        } else if (fillStyle.GetFillColor().IsVisible()) {
            
            CGContextSetRGBFillColor(cg, fillStyle.GetFillColor().GetR(), fillStyle.GetFillColor().GetG(),
                                     fillStyle.GetFillColor().GetB(), fillStyle.GetFillColor().GetA());
        } else {
            CGContextSetRGBFillColor(cg,0,0,0,0);
        }
    }
    
    /*
     * SetPen(const LineStyle& style,
     *        double lineWidth)
     */
    void MapPainterIOS::SetPen(const LineStyle& style,
                              double lineWidth) {
        CGContextSetRGBStrokeColor(cg,style.GetLineColor().GetR(),
                                      style.GetLineColor().GetG(),
                                      style.GetLineColor().GetB(),
                                      style.GetLineColor().GetA());
        CGContextSetLineWidth(cg,lineWidth);
        
        if (style.GetDash().empty()) {
            CGContextSetLineDash(cg, 0.0, NULL, 0);
            CGContextSetLineCap(cg, kCGLineCapRound);
        } else {
            CGFloat *dashes = (CGFloat *)malloc(sizeof(CGFloat)*style.GetDash().size());
            for (size_t i=0; i<style.GetDash().size(); i++) {
                dashes[i] = style.GetDash()[i]*lineWidth;
            }
            CGContextSetLineDash(cg, 0.0, dashes, style.GetDash().size());
            free(dashes); dashes = NULL;
            CGContextSetLineCap(cg, kCGLineCapButt);
        }

    }

    
    /*
     * DrawArea(const Projection& projection,
     *          const MapParameter& parameter,
     *          const AreaData& area)
     */
    void MapPainterIOS::DrawArea(const Projection& projection,
                                const MapParameter& parameter,
                                const MapPainter::AreaData& area) {
        CGContextSaveGState(cg);
        CGContextBeginPath(cg);
        CGContextMoveToPoint(cg,coordBuffer->buffer[area.transStart].GetX(),
                    coordBuffer->buffer[area.transStart].GetY());
        for (size_t i=area.transStart+1; i<=area.transEnd; i++) {
            CGContextAddLineToPoint(cg,coordBuffer->buffer[i].GetX(),
                        coordBuffer->buffer[i].GetY());
        }
        CGContextAddLineToPoint(cg,coordBuffer->buffer[area.transStart].GetX(),
                                coordBuffer->buffer[area.transStart].GetY());
        
        if (!area.clippings.empty()) {
            for (std::list<PolyData>::const_iterator c=area.clippings.begin();
                 c!=area.clippings.end();
                 c++) {
                const PolyData& data=*c;
                
                CGContextMoveToPoint(cg,coordBuffer->buffer[data.transStart].GetX(),
                            coordBuffer->buffer[data.transStart].GetY());
                for (size_t i=data.transStart+1; i<=data.transEnd; i++) {
                    CGContextAddLineToPoint(cg,coordBuffer->buffer[i].GetX(),
                                coordBuffer->buffer[i].GetY());
                }
                CGContextAddLineToPoint(cg,coordBuffer->buffer[data.transStart].GetX(),
                                        coordBuffer->buffer[data.transStart].GetY());
            }
        }
        
        if (area.fillStyle) {
            SetFill(projection, parameter, *area.fillStyle);
        } else {
            CGContextSetRGBFillColor(cg,0,0,0,0);
        }
        
        if (area.borderStyle) {
            SetBorder(projection, parameter, *area.borderStyle);
        } else {
            CGContextSetRGBStrokeColor(cg,0,0,0,0);
        }

        CGContextDrawPath(cg,  kCGPathEOFillStroke);
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
        const Color &fillColor = style.GetFillColor();
        CGContextSetRGBFillColor(cg, fillColor.GetR(), fillColor.GetG(), fillColor.GetB(), fillColor.GetA());
        CGContextAddRect(cg, CGRectMake(0,0,projection.GetWidth(),projection.GetHeight()));
        CGContextDrawPath(cg, kCGPathFill);
        CGContextRestoreGState(cg);
    }
    
    /*
     * SetBorder(const Projection& projection,
     *           const MapParameter& parameter,
     *           const BorderStyle& borderStyle)
     */
    void MapPainterIOS::SetBorder(const Projection& projection,
                                  const MapParameter& parameter,
                                  const BorderStyle& borderStyle){
        double borderWidth=projection.ConvertWidthToPixel(borderStyle.GetWidth());
        
        if (borderWidth>=parameter.GetLineMinWidthPixel()) {
            CGContextSetRGBStrokeColor(cg,borderStyle.GetColor().GetR(),
                                       borderStyle.GetColor().GetG(),
                                       borderStyle.GetColor().GetB(),
                                       borderStyle.GetColor().GetA());
            CGContextSetLineWidth(cg, borderWidth);
            
            if (borderStyle.GetDash().empty()) {
                CGContextSetLineDash(cg, 0.0, NULL, 0);
                CGContextSetLineCap(cg, kCGLineCapRound);
            } else {
                CGFloat *dashes = (CGFloat *)malloc(sizeof(CGFloat)*borderStyle.GetDash().size());
                for (size_t i=0; i<borderStyle.GetDash().size(); i++) {
                    dashes[i] = borderStyle.GetDash()[i]*borderWidth;
                }
                CGContextSetLineDash(cg, 0.0, dashes, borderStyle.GetDash().size());
                free(dashes); dashes = NULL;
                CGContextSetLineCap(cg, kCGLineCapButt);
            }
        } else {
            CGContextSetRGBStrokeColor(cg,0,0,0,0);
        }
    }

}
