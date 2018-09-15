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
#include <osmscout/util/String.h>

#if ! __has_feature(objc_arc)
#error This file must be compiled with ARC. Either turn on ARC for the project or use -fobjc-arc flag
#endif

namespace osmscout {

    MapPainterIOS::MapPainterIOS(const StyleConfigRef& styleConfig)
    : MapPainter(styleConfig, new CoordBuffer()), labelLayouter(this)
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

        fontSize=fontSize*projection.ConvertWidthToPixel(parameter.GetFontSize())*contentScale;

        f=fonts.find(fontSize);

        if (f!=fonts.end()) {
            return f->second;
        }

        NSString *fontName = [NSString stringWithUTF8String: parameter.GetFontName().c_str()];
        Font *font = [Font fontWithName:fontName size:fontSize];
        if(!font){
            std::cerr<<"ERROR font '"<< parameter.GetFontName() << "' not found !" << std::endl;
            return 0;
        }
        return fonts.insert(std::pair<size_t,Font *>(fontSize,font)).first->second;
    }

    /*
     * DrawMap()
     */
    bool MapPainterIOS::DrawMap(const StyleConfig& /*styleConfig*/,
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
        FillStyleRef landFill=styleConfig->GetLandFillStyle(projection);


        if (!landFill) {
          return;
        }

        cg = paintCG;

        if (parameter.GetRenderBackground()) {
            DrawGround(projection,
                       parameter,
                       *landFill);
        }

        FillStyleRef       seaFill=styleConfig->GetSeaFillStyle(projection);
        FillStyleRef       coastFill=styleConfig->GetCoastFillStyle(projection);
        FillStyleRef       unknownFill=styleConfig->GetUnknownFillStyle(projection);
        LineStyleRef       coastlineLine=styleConfig->GetCoastlineLineStyle(projection);
        std::vector<Point> points;
        size_t             start=0;
        size_t             end=0;

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
    bool MapPainterIOS::HasIcon(const StyleConfig& /* styleConfig */,
                                const Projection& /*projection*/,
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

            std::string filename=*path+"/"+style.GetIconName()+".png";
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
            std::string filename=*path+"/"+style.GetPatternName()+".png";

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
    double MapPainterIOS::GetFontHeight(const Projection& projection,
                                      const MapParameter& parameter,
                                      double fontSize){
        Font *font = GetFont(projection,parameter,fontSize);
        CGSize size = CGSizeZero;
        if(font){
#if TARGET_OS_IPHONE
            size = [@"Aj" sizeWithAttributes:@{NSFontAttributeName:font}];
#else
            NSRect stringBounds = [@"Aj" boundingRectWithSize:CGSizeMake(500, 50) options:NSStringDrawingUsesLineFragmentOrigin attributes:[NSDictionary dictionaryWithObject:font forKey:NSFontAttributeName]];
            size = stringBounds.size;
#endif
        }
        return size.height;
    }

    DoubleRectangle MapPainterIOS::GlyphBoundingBox(const CTRunRef &glyph) const {
        CFRange range = CFRangeMake(0,1);
        CGRect glyphRect = CTRunGetImageBounds(glyph, cg, range);
        return DoubleRectangle(glyphRect.origin.x,glyphRect.origin.y,glyphRect.size.width,glyphRect.size.height);
    }
    
    // TODO: to finish
    template<> std::vector<IOSGlyph> IOSLabel::ToGlyphs() const {
        std::vector<IOSGlyph> result;
        return result;
    }

    // TODO: to finish
    void MapPainterIOS::DrawGlyph(const IOSGlyph &glyph) const {
    }
    
    // TODO: to finish
    void MapPainterIOS::DrawGlyphs(const Projection &projection,
                    const MapParameter &parameter,
                    const osmscout::PathTextStyleRef style,
                    const std::vector<IOSGlyph> &glyphs){
        
        for (const auto &glyph:glyphs) {
            DrawGlyph(glyph);
        }
        
    }
    
    std::shared_ptr<IOSLabel> MapPainterIOS::Layout(const Projection& projection,
                                                    const MapParameter& parameter,
                                                    const std::string& text,
                                                    double fontSize,
                                                    double objectWidth,
                                                    bool enableWrapping,
                                                    bool contourLabel) {
        std::shared_ptr<IOSLabel> result = std::make_shared<IOSLabel>();
        CGRect rect = CGRectZero;
        Font *font = GetFont(projection, parameter, fontSize);
        NSString *str = [NSString stringWithCString:text.c_str() encoding:NSUTF8StringEncoding];
        NSDictionary<NSAttributedStringKey, id> *attr = @{};
        if(font){
            attr = @{NSFontAttributeName:font};
        }
        NSAttributedString *attrStr = [[NSAttributedString alloc] initWithString:str attributes:attr];
        
        CFAttributedStringRef cfString = (CFAttributedStringRef)CFBridgingRetain(attrStr);
        CTLineRef line = CTLineCreateWithAttributedString(cfString);
        CFArrayRef runArray = CTLineGetGlyphRuns(line);
        if(CFArrayGetCount(runArray) > 0){
            CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runArray, 0);
            
            result->label = run;
            rect = CTRunGetImageBounds(run, cg, CFRangeMake(0, 0));
        }
        // TODO: how to release the line ?
        //CFRelease(line);
        
        result->text = text;
        result->fontSize = fontSize;
        result->width = rect.size.width;
        result->height = rect.size.height;
        
        return result;
    }
    
    void MapPainterIOS::LayoutDrawLabel(const IOSLabel& layout,
                                        const CGPoint& coords,
                                        const Color &color,
                                        bool emphasize){
        
        CTRunRef run = layout.label;
        const CTFontRef font = (CTFontRef)CFDictionaryGetValue(CTRunGetAttributes(run), kCTFontAttributeName);
        CGFloat fontHeight = CTRunGetImageBounds(run, cg, CFRangeMake(0, 0)).size.height;
        CFIndex glyphCount = CTRunGetGlyphCount(run);
        CGGlyph glyphs[glyphCount];
        CGPoint glyphPositions[glyphCount];
        CTRunGetGlyphs(run, CFRangeMake(0, 0), glyphs);
        CTRunGetPositions(run, CFRangeMake(0, 0), glyphPositions);
        for(int index = 0; index < glyphCount; index++){
            glyphPositions[index].x += coords.x;
            glyphPositions[index].y += CGBitmapContextGetHeight(cg) - coords.y - fontHeight;
        }
        
        double r = color.GetR();
        double g = color.GetG();
        double b = color.GetB();
        
        CGContextSaveGState(cg);
        CGContextSetRGBFillColor(cg, r, g, b, 1.0);
        CGContextSetRGBStrokeColor(cg, r, g, b, 1.0);
        if(emphasize){
            CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
            CGColorRef haloColor = CGColorCreate(colorSpace, (CGFloat[]){ 1, 1, 1, 1 });
            CGContextSetShadowWithColor( cg, CGSizeMake( 0.0, 0.0 ), 2.0f, haloColor );
            CGColorRelease(haloColor);
            CGColorSpaceRelease(colorSpace);
        }
        // The text is drawn reversed...
        CGContextTranslateCTM(cg, 0.0, CGBitmapContextGetHeight(cg));
        CGContextScaleCTM(cg, 1.0, -1.0);
        CTFontDrawGlyphs(font, glyphs, glyphPositions, glyphCount, cg);
        CGContextRestoreGState(cg);
    }
    
    void MapPainterIOS::DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const DoubleRectangle& labelRect,
                   const LabelData& label,
                   const IOSLabel& layout) {
        // TODO: check that labelRectangle intersect the viewport
        
        if (dynamic_cast<const TextStyle*>(label.style.get())!=nullptr) {
            const auto *style=dynamic_cast<const TextStyle*>(label.style.get());
            
            if (style->GetStyle()==TextStyle::normal) {
                LayoutDrawLabel(layout, CGPointMake(labelRect.x, labelRect.y), style->GetTextColor(), false);
                
            } else if (style->GetStyle()==TextStyle::emphasize) {
                LayoutDrawLabel(layout, CGPointMake(labelRect.x, labelRect.y), style->GetTextColor(), true);
                
            } else if (dynamic_cast<const ShieldStyle*>(label.style.get())!=nullptr) {
                
                const ShieldStyle* style=dynamic_cast<const ShieldStyle*>(label.style.get());
 
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
                CGContextAddRect(cg, CGRectMake(labelRect.x,
                                                labelRect.y,
                                                labelRect.width,
                                                labelRect.height));
                CGContextDrawPath(cg, kCGPathFillStroke);
                
                CGContextAddRect(cg, CGRectMake(labelRect.x+2,
                                                labelRect.y+2,
                                                labelRect.width-4,
                                                labelRect.height-4));
                CGContextDrawPath(cg, kCGPathStroke);
                
                LayoutDrawLabel(layout, CGPointMake(labelRect.x, labelRect.y), style->GetTextColor(), false);
                
                CGContextRestoreGState(cg);
                
            } else {
                log.Warn() << "Label style not recognised: " << label.style.get();
            }
        }
    }
    
    void MapPainterIOS::BeforeDrawing(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                       const MapData& data){
        labelLayouter.SetViewport(DoubleRectangle(0, 0, CGBitmapContextGetWidth(cg), CGBitmapContextGetHeight(cg)));
        labelLayouter.SetLayoutOverlap(parameter.GetDropNotVisiblePointLabels() ? 0 : 1);
    }
    
    void MapPainterIOS::RegisterRegularLabel(const Projection &projection,
                                             const MapParameter &parameter,
                                             const std::vector<LabelData> &labels,
                                             const Vertex2D &position,
                                             double objectWidth){
        labelLayouter.RegisterLabel(projection, parameter, position, labels, objectWidth);
    }
    
    void MapPainterIOS::RegisterContourLabel(const Projection &projection,
                                      const MapParameter &parameter,
                                      const PathLabelData &label,
                                      const LabelPath &labelPath){
        labelLayouter.RegisterContourLabel(projection, parameter, label, labelPath);
    }
    
    void MapPainterIOS::DrawLabels(const Projection& projection,
                            const MapParameter& parameter,
                            const MapData& data) {
        labelLayouter.Layout(projection, parameter);
        labelLayouter.DrawLabels(projection,
                                 parameter,
                                 this);
        labelLayouter.Reset();
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
    
    /*
     *
     * DrawIcon(const IconStyle* style,
     *          double centerX, double centerY,
     *          double width, double height)
     */
    void MapPainterIOS::DrawIcon(const IconStyle* style,
                                 double x, double y,
                                 double /*width*/, double /*height*/){
        size_t idx=style->GetIconId()-1;

        assert(idx<images.size());
        assert(images[idx]);

        CGFloat w = CGImageGetWidth(images[idx])/contentScale;
        CGFloat h = CGImageGetHeight(images[idx])/contentScale;
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
                                             y+projection.ConvertWidthToPixel(pixel->GetY()-centerY));
                    } else {
                        CGContextAddLineToPoint(cg,x+projection.ConvertWidthToPixel(pixel->GetX()-centerX),
                                                y+projection.ConvertWidthToPixel(pixel->GetY()-centerY));
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
                                         y+projection.ConvertWidthToPixel(rectangle->GetTopLeft().GetY()-centerY),
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
                                         y+projection.ConvertWidthToPixel(circle->GetCenter().GetY()-centerY),
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
    void MapPainterIOS::DrawPath(const Projection& /*projection*/,
                  const MapParameter& /*parameter*/,
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
                                   const MapParameter& /*parameter*/,
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
