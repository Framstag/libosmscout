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

#import <osmscoutmapiosx/MapPainterIOS.h>
#import <osmscoutmapiosx/SymbolRendererIOS.h>

#include <cassert>
#include <limits>
#include <string>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/String.h>

#if ! __has_feature(objc_arc)
#error This file must be compiled with ARC. Either turn on ARC for the project or use -fobjc-arc flag
#endif

namespace osmscout {

    MapPainterIOS::MapPainterIOS(const StyleConfigRef& styleConfig)
    : MapPainter(styleConfig), labelLayouter(this){
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        contentScale = [[UIScreen mainScreen] scale];
#else
        contentScale = [[NSScreen mainScreen] backingScaleFactor];
#endif
    }

    MapPainterIOS::~MapPainterIOS(){
        for(std::vector<CGImageRef>::const_iterator image=images.begin(); image<images.end();image++){
            CGImageRelease(*image);
        }
        for(std::vector<CGImageRef>::const_iterator image=patternImages.begin(); image<patternImages.end();image++){
            CGImageRelease(*image);
        }
    }

    Font *MapPainterIOS::GetFont(const Projection& projection,
                                 const MapParameter& parameter,
                                 double fontSize){
        std::map<size_t,Font *>::const_iterator f;

        fontSize=fontSize*projection.ConvertWidthToPixel(parameter.GetFontSize());

        f=fonts.find(fontSize);

        if (f!=fonts.end()) {
            return f->second;
        }

        NSString *fontName = [NSString stringWithUTF8String: parameter.GetFontName().c_str()];
        Font *font = [Font fontWithName:fontName size:fontSize];
        if(!font){
            log.Debug()<<"ERROR font '"<< parameter.GetFontName() << "' not found !";
            return 0;
        }
        return fonts.insert(std::pair<size_t,Font *>(fontSize,font)).first->second;
    }

    /*
     * DrawMap() with start and end RenderSteps
     */
    bool MapPainterIOS::DrawMap(const StyleConfig& /*styleConfig*/,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data,
                                CGContextRef paintCG,
                                RenderSteps startStep,
                                RenderSteps endStep){
        cg = paintCG;
        return Draw(projection,
                    parameter,
                    data,
                    startStep,
                    endStep);
    }

    /*
     * DrawMap()
     */
    bool MapPainterIOS::DrawMap(const StyleConfig& styleConfig,
                            const Projection& projection,
                            const MapParameter& parameter,
                            const MapData& data,
                            CGContextRef paintCG) {
        return DrawMap(styleConfig, projection, parameter, data, paintCG, RenderSteps::FirstStep, RenderSteps::LastStep);
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

        // TODO: remove this method and use standard Draw
        MapData data;
        data.baseMapTiles=groundTiles;

        Draw(projection,
             parameter,
             data,
             RenderSteps::DrawBaseMapTiles,
             RenderSteps::DrawBaseMapTiles);
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

            std::string filename;
            if(contentScale == 1){
                filename = *path+"/"+style.GetIconName()+".png";
            } else {
                filename = *path+"/"+style.GetIconName()+"@"+std::to_string((int)contentScale)+"x.png";
            }

            Image *image = [[Image alloc] initWithContentsOfFile:[NSString stringWithUTF8String: filename.c_str()]];
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

                return true;
            }
        }

        log.Warn() << "ERROR while loading image '" << style.GetIconName() << "'";
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
            std::string filename;
            if(contentScale == 1){
                filename = *path+"/"+style.GetPatternName()+".png";
            } else {
                filename = *path+"/"+style.GetPatternName()+"@"+std::to_string((int)contentScale)+"x.png";
            }

            Image *image = [[Image alloc] initWithContentsOfFile:[NSString stringWithUTF8String: filename.c_str()]];
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
                log.Debug() << "Loaded image " << filename << " (" <<  imgWidth << "x" << imgHeight <<  ") => id " << style.GetPatternId();
#endif
                CGImageRetain(imgRef);
                patternImages.resize(patternImages.size()+1,imgRef);
                style.SetPatternId(patternImages.size());
                return true;
            }
        }

        log.Warn() << "ERROR while loading icon file '" << style.GetPatternName() << "'";
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

    /**
     * Returns the average char width of the font.
     */
    double MapPainterIOS::GetAverageCharWidth(const Projection& projection,
                                const MapParameter& parameter,
                                double fontSize){
        Font *font = GetFont(projection,parameter,fontSize);

        double convFontSize=fontSize*projection.ConvertWidthToPixel(parameter.GetFontSize());
        std::map<size_t,double>::const_iterator it = averageCharWidth.find(convFontSize);
        if (it != averageCharWidth.end()) {
            return it->second;
        }

        CGSize size = CGSizeZero;
        NSString *allChars = @"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        if(font){
#if TARGET_OS_IPHONE
            size = [allChars sizeWithAttributes:@{NSFontAttributeName:font}];
#else
            NSRect stringBounds = [allChars boundingRectWithSize:CGSizeMake(1000, 50) options:NSStringDrawingUsesLineFragmentOrigin attributes:[NSDictionary dictionaryWithObject:font forKey:NSFontAttributeName]];
            size = stringBounds.size;
#endif
        }
        double averageWidth = size.width/allChars.length;
        log.Debug() << "AverageCharWidth for font size=" << convFontSize << ": " <<  averageWidth;
        return averageCharWidth.insert(std::pair<size_t,double>(convFontSize,averageWidth)).first->second;
    }

    ScreenVectorRectangle MapPainterIOS::GlyphBoundingBox(const IOSGlyphInRun &glyph) const {
        CFRange range = CFRangeMake(glyph.index, 1);
        CGRect glyphRect = CTRunGetImageBounds(glyph.run, cg, range);
        return ScreenVectorRectangle(glyphRect.origin.x,glyphRect.origin.y,glyphRect.size.width,glyphRect.size.height);
    }

    template<> std::vector<IOSGlyph> IOSLabel::ToGlyphs() const {
        std::vector<IOSGlyph> result;
        int i = 0;
        for(const auto &run:label.run){
            CFIndex glyphCount = CTRunGetGlyphCount(run);
            CGGlyph glyphs[glyphCount];
            CGPoint glyphPositions[glyphCount];
            CTRunGetGlyphs(run, CFRangeMake(0, 0), glyphs);
            CTRunGetPositions(run, CFRangeMake(0, 0), glyphPositions);
            for(int index = 0; index < glyphCount; index++){
                IOSGlyph glyph;
                glyph.glyph.line = label.line[i];
                glyph.glyph.run = run;
                glyph.glyph.index = index;
                glyph.position=Vertex2D(glyphPositions[index].x, glyphPositions[index].y);
                result.push_back(std::move(glyph));
            }
            i++;
        }
        return result;
    }

    void MapPainterIOS::DrawGlyphs(const Projection &/*projection*/,
                    const MapParameter &/*parameter*/,
                    const osmscout::PathTextStyleRef style,
                    const std::vector<IOSGlyph> &glyphs){
        if(glyphs.size() == 0){
            return;
        }
        CTRunRef run = glyphs[0].glyph.run;
        const CTFontRef font = (CTFontRef)CFDictionaryGetValue(CTRunGetAttributes(run), kCTFontAttributeName);
        CGGlyph glyphToDraw[1];
        CGPoint glyphPositions[1];

        double r = style->GetTextColor().GetR();
        double g = style->GetTextColor().GetG();
        double b = style->GetTextColor().GetB();
        double a = style->GetTextColor().GetA();

        CGContextSaveGState(cg);
        CGContextSetRGBFillColor(cg, r, g, b, a);
        CGContextSetRGBStrokeColor(cg, r, g, b, a);
        int index = 0;
        for (const auto &glyph:glyphs) {
            CTRunGetGlyphs(run, CFRangeMake(index, 1), glyphToDraw);
            glyphPositions[0] = CGPointMake(0,0);
            CGContextSaveGState(cg);
            CGContextTranslateCTM(cg, glyph.position.GetX(), glyph.position.GetY());
            CGContextRotateCTM(cg, glyph.angle);
            CGContextScaleCTM(cg, 1.0, -1.0);
            CTFontDrawGlyphs(font, glyphToDraw, glyphPositions, 1, cg);
            CGContextRestoreGState(cg);
            index++;
        }
        CGContextRestoreGState(cg);

    }

    std::shared_ptr<IOSLabel> MapPainterIOS::Layout(const Projection& projection,
                                                    const MapParameter& parameter,
                                                    const std::string& text,
                                                    double fontSize,
                                                    double objectWidth,
                                                    bool enableWrapping,
                                                    bool /*contourLabel*/) {
        std::shared_ptr<IOSLabel> result = std::make_shared<IOSLabel>();
        CGRect rect = CGRectZero;

        double proposedWidth = -1;
        if (enableWrapping && objectWidth > ProposedWidthMinWidth) {
            proposedWidth = ProposedWidthExpandRatio * GetProposedLabelWidth(parameter,
                                                  GetAverageCharWidth(projection, parameter, fontSize),
                                                  objectWidth,
                                                  text.length());

            log.Debug() << "proposedWidth=" << proposedWidth << " for '" << text << "'";
        }


        NSString *str = [NSString stringWithCString:text.c_str() encoding:NSUTF8StringEncoding];
        // replace hyphen by unicode NON-BREAKING HYPHEN
        str = [str stringByReplacingOccurrencesOfString:@"-" withString:@"\u2011"];

        Font *font = GetFont(projection, parameter, fontSize);
        NSMutableDictionary<NSAttributedStringKey, id> *attr = [NSMutableDictionary dictionaryWithDictionary: @{}];
        if(font){
            attr[NSFontAttributeName] = font;
        }
        NSAttributedString *attrStr = [[NSAttributedString alloc] initWithString:str attributes:attr];

        CFAttributedStringRef cfString = (__bridge CFAttributedStringRef)attrStr;

        CTFramesetterRef framesetter = CTFramesetterCreateWithAttributedString(cfString);
        if (framesetter) {
            CGPathRef path;
            if (proposedWidth > 0){
                path = CGPathCreateWithRect(CGRectMake(0, 0, proposedWidth, std::numeric_limits<double>::max()), NULL);
            } else {
                path = CGPathCreateWithRect(CGRectMake(0, 0, std::numeric_limits<double>::max(), std::numeric_limits<double>::max()), NULL);
            }
            CTFrameRef frame = CTFramesetterCreateFrame(framesetter, CFRangeMake(0, 0), path, NULL);
            CGPathRelease(path);
            CFArrayRef lines =  CTFrameGetLines(frame);
            for (int lineNumber = 0; lineNumber< CFArrayGetCount(lines); lineNumber++){
                CTLineRef line = (CTLineRef)CFArrayGetValueAtIndex(lines, lineNumber);
                result->label.line.push_back(line);
                CFArrayRef runArray = CTLineGetGlyphRuns(line);
                if(CFArrayGetCount(runArray) > 0){
                    CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runArray, 0);
                    result->label.run.push_back(run);
                    rect = CGRectUnion(rect, CTRunGetImageBounds(run, cg, CFRangeMake(0, 0)));
                }
            }
            result->label.lineWidth = rect.size.width;
            result->label.lineHeight = GetFontHeight(projection, parameter, fontSize);

            log.Debug() << "Layout '"<<text<<"' width=" << rect.size.width <<" height=" << rect.size.height;

            result->text = text;
            result->fontSize = fontSize;
            result->width = rect.size.width;
            result->height = rect.size.height;
        } else {
            log.Warn() << "CTFramesetterCreateFrame returned NULL";
        }

        return result;
    }

    void MapPainterIOS::LayoutDrawLabel(const IOSRunInLine& layout,
                                        const CGPoint& coords,
                                        const Color &color,
                                        bool emphasize){
        double r = color.GetR();
        double g = color.GetG();
        double b = color.GetB();
        double a = color.GetA();
        CGFloat lineHeight = layout.lineHeight;
        CGFloat width = layout.lineWidth;

        log.Debug() << "LayoutDrawLabel lineWidth=" << width << " lineHeight=" << lineHeight;

        int lineNumber = 0;
        for (CTRunRef run : layout.run) {
            const CTFontRef font = (CTFontRef)CFDictionaryGetValue(CTRunGetAttributes(run), kCTFontAttributeName);
            const CFIndex glyphCount = CTRunGetGlyphCount(run);
            CGGlyph glyphs[glyphCount];
            CGPoint glyphPositions[glyphCount];
            CGSize glyphAdvances[glyphCount];
            CTRunGetGlyphs(run, CFRangeMake(0, 0), glyphs);
            CTRunGetPositions(run, CFRangeMake(0, 0), glyphPositions);
            CTRunGetAdvances(run, CFRangeMake(0, 0), glyphAdvances);
            CGFloat lineWidth = 0;
            for(int index = 0; index < glyphCount; index++){
                glyphPositions[index].x += coords.x;
                lineWidth += glyphAdvances[index].width;
                glyphPositions[index].y += CGBitmapContextGetHeight(cg) - coords.y - (lineNumber+1) * lineHeight;
            }
            CGFloat centerDelta = (width - lineWidth)/2;
            log.Debug() << "LayoutDrawLabel centerDelta=" << centerDelta;
            if (centerDelta>0){
                for(int index = 0; index < glyphCount; index++){
                    glyphPositions[index].x += centerDelta;
                }
            }
            lineNumber++;

            CGContextSaveGState(cg);
            CGContextSetRGBFillColor(cg, r, g, b, a);
            CGContextSetRGBStrokeColor(cg, r, g, b, a);
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
    }

    void MapPainterIOS::DrawLabel(const Projection& /*projection*/,
                                  const MapParameter& /*parameter*/,
                                  const ScreenVectorRectangle& labelRect,
                                  const LabelData& label,
                                  const IOSRunInLine& layout) {

        if (const auto *style = dynamic_cast<const TextStyle*>(label.style.get());
            style != nullptr) {

            if (style->GetStyle()==TextStyle::normal) {
                LayoutDrawLabel(layout, CGPointMake(labelRect.x, labelRect.y), style->GetTextColor(), false);

            } else if (style->GetStyle()==TextStyle::emphasize) {
                LayoutDrawLabel(layout, CGPointMake(labelRect.x, labelRect.y), style->GetTextColor(), true);
            }

        } else if (const ShieldStyle* style = dynamic_cast<const ShieldStyle*>(label.style.get());
                   style != nullptr) {

            CGContextSaveGState(cg);
            CGContextSetRGBFillColor(cg,
                                     style->GetBgColor().GetR(),
                                     style->GetBgColor().GetG(),
                                     style->GetBgColor().GetB(),
                                     style->GetBorderColor().GetA());
            CGContextSetRGBStrokeColor(cg,style->GetBorderColor().GetR(),
                                       style->GetBorderColor().GetG(),
                                       style->GetBorderColor().GetB(),
                                       style->GetBorderColor().GetA());
            CGContextAddRect(cg, CGRectMake(labelRect.x - 5,
                                            labelRect.y - 5,
                                            labelRect.width + 10,
                                            labelRect.height + 10));
            CGContextDrawPath(cg, kCGPathFillStroke);

            CGContextAddRect(cg, CGRectMake(labelRect.x - 5 + 2,
                                            labelRect.y - 5 + 2,
                                            labelRect.width + 10 - 4,
                                            labelRect.height + 10 - 4));
            CGContextDrawPath(cg, kCGPathStroke);

            LayoutDrawLabel(layout, CGPointMake(labelRect.x, labelRect.y - layout.lineHeight/3), style->GetTextColor(), false);

            CGContextRestoreGState(cg);

        } else {
            log.Warn() << "Label style not recognised: " << label.style.get();
        }
    }

    void MapPainterIOS::BeforeDrawing(const StyleConfig& /*styleConfig*/,
                                      const Projection& projection,
                                      const MapParameter& parameter,
                                      const MapData& /*data*/){
        labelLayouter.SetViewport(ScreenVectorRectangle(0, 0, CGBitmapContextGetWidth(cg), CGBitmapContextGetHeight(cg)));
        labelLayouter.SetLayoutOverlap(projection.ConvertWidthToPixel(parameter.GetLabelLayouterOverlap()));
    }

    void MapPainterIOS::RegisterRegularLabel(const Projection &projection,
                                             const MapParameter &parameter,
                                             const ObjectFileRef& ref,
                                             const std::vector<LabelData> &labels,
                                             const Vertex2D &position,
                                             double objectWidth){
        labelLayouter.RegisterLabel(projection, parameter, ref, position, labels, objectWidth);
    }

    void MapPainterIOS::RegisterContourLabel(const Projection &projection,
                                      const MapParameter &parameter,
                                      const ObjectFileRef& ref,
                                      const PathLabelData &label,
                                      const LabelPath &labelPath){
        labelLayouter.RegisterContourLabel(projection, parameter, ref, label, labelPath);
    }

    void MapPainterIOS::DrawLabels(const Projection& projection,
                            const MapParameter& parameter,
                            const MapData& /*data*/) {
        labelLayouter.Layout(projection, parameter);
        labelLayouter.DrawLabels(projection,
                                 parameter,
                                 this);
        labelLayouter.Reset();
    }

    void MapPainterIOS::followPathInit(FollowPathHandle &hnd, Vertex2D &origin, size_t transStart, size_t transEnd,
                                       bool isClosed, bool keepOrientation) {
        hnd.i = 0;
        hnd.nVertex = transEnd - transStart;
        bool isReallyClosed = (coordBuffer.buffer[transStart] == coordBuffer.buffer[transEnd]);
        if(isClosed && !isReallyClosed){
            hnd.nVertex++;
            hnd.closeWay = true;
        } else {
            hnd.closeWay = false;
        }
        if(keepOrientation || coordBuffer.buffer[transStart].GetX()<coordBuffer.buffer[transEnd].GetX()){
            hnd.transStart = transStart;
            hnd.transEnd = transEnd;
        } else {
            hnd.transStart = transEnd;
            hnd.transEnd = transStart;
        }
        hnd.direction = (hnd.transStart < hnd.transEnd) ? 1 : -1;
        origin=Vertex2D(coordBuffer.buffer[hnd.transStart].GetX(), coordBuffer.buffer[hnd.transStart].GetY());
    }

    bool MapPainterIOS::followPath(FollowPathHandle &hnd, double l, Vertex2D &origin) {

        double x = origin.GetX();
        double y = origin.GetY();
        double x2,y2;
        double deltaX, deltaY, len, fracToGo;
        while(hnd.i < hnd.nVertex) {
            if(hnd.closeWay && hnd.nVertex - hnd.i == 1){
                x2 = coordBuffer.buffer[hnd.transStart].GetX();
                y2 = coordBuffer.buffer[hnd.transStart].GetY();
            } else {
                x2 = coordBuffer.buffer[hnd.transStart+(hnd.i+1)*hnd.direction].GetX();
                y2 = coordBuffer.buffer[hnd.transStart+(hnd.i+1)*hnd.direction].GetY();
            }
            deltaX = (x2 - x);
            deltaY = (y2 - y);
            len = sqrt(deltaX*deltaX + deltaY*deltaY);

            fracToGo = l/len;
            if(fracToGo <= 1.0) {
                origin=Vertex2D(x + deltaX*fracToGo,y + deltaY*fracToGo);
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
                                          const MapParameter& /* parameter */,
                                          const Symbol& symbol,
                                          const ContourSymbolData& data){
        SymbolRendererIOS renderer(cg);
        ScreenBox boundingBox=symbol.GetBoundingBox(projection);
        double width=boundingBox.GetWidth();
        bool isClosed = false;
        CGAffineTransform transform=CGAffineTransformMake(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
        Vertex2D origin;
        double slope;
        double x1,y1,x2,y2,x3,y3;
        FollowPathHandle followPathHnd;
        followPathInit(followPathHnd, origin, data.coordRange.GetStart(), data.coordRange.GetEnd(), isClosed, true);
        if(!isClosed && !followPath(followPathHnd, data.symbolOffset, origin)){
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
                    renderer.Render(projection,
                                    symbol,
                                    Vertex2D(0.0, 0.0),
                                    []() {
                                    },
                                    []() {
                                    },
                                    data.symbolScale);
                    CGContextRestoreGState(cg);
                    loop = followPath(followPathHnd, data.symbolSpace, origin);
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
                                 const Vertex2D& centerPos,
                                 double /*width*/, double /*height*/) {
        size_t idx=style->GetIconId()-1;

        assert(idx<images.size());
        assert(images[idx]);

        CGFloat w = CGImageGetWidth(images[idx]);
        CGFloat h = CGImageGetHeight(images[idx]);
        CGRect rect = CGRectMake(centerPos.GetX()-w/2,
                                 -h/2-centerPos.GetY(),
                                 w, h);
        CGContextSaveGState(cg);
        CGContextScaleCTM(cg, 1.0, -1.0);
        CGContextDrawImage(cg, rect, images[idx]);
        CGContextRestoreGState(cg);
    }


    /*
     * DrawSymbol(const Projection& projection,
     *            const MapParameter& parameter,
     *            const SymbolRef& symbol,
     *            double x, double y, double scaleFactor)
     */
    void MapPainterIOS::DrawSymbol(const Projection& projection,
                    const MapParameter& /* parameter */,
                    const Symbol& symbol,
                    const Vertex2D& screenPos,
                    double scaleFactor) {

        SymbolRendererIOS renderer(cg);

        renderer.Render(projection,
                        symbol,
                        screenPos,
                        scaleFactor);
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
                  const CoordBufferRange& coordRange){

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
        CGContextMoveToPoint(cg,coordBuffer.buffer[coordRange.GetStart()].GetX(),coordBuffer.buffer[coordRange.GetStart()].GetY());
        for (size_t i=coordRange.GetStart()+1; i<=coordRange.GetEnd(); i++) {
            CGContextAddLineToPoint (cg,coordBuffer.buffer[i].GetX(),coordBuffer.buffer[i].GetY());
        }
        CGContextStrokePath(cg);
        if (startCap==LineStyle::capRound) {
            CGContextSetRGBFillColor(cg, color.GetR(), color.GetG(), color.GetB(), color.GetA());
            CGContextFillEllipseInRect(cg, CGRectMake(coordBuffer.buffer[coordRange.GetStart()].GetX()-width/2,
                                                     coordBuffer.buffer[coordRange.GetStart()].GetY()-width/2,
                                                     width,width));
        }
        if (endCap==LineStyle::capRound) {
            CGContextSetRGBFillColor(cg, color.GetR(), color.GetG(), color.GetB(), color.GetA());
            CGContextFillEllipseInRect(cg, CGRectMake(coordBuffer.buffer[coordRange.GetEnd()].GetX()-width/2,
                                                      coordBuffer.buffer[coordRange.GetEnd()].GetY()-width/2,
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
     * DrawArea(const Projection& projection,
     *          const MapParameter& parameter,
     *          const AreaData& area)
     */
    void MapPainterIOS::DrawArea(const Projection& projection,
                                const MapParameter& parameter,
                                const MapPainter::AreaData& area) {
        CGContextSaveGState(cg);
        CGContextBeginPath(cg);
        CGContextMoveToPoint(cg,coordBuffer.buffer[area.coordRange.GetStart()].GetX(),
                    coordBuffer.buffer[area.coordRange.GetStart()].GetY());
        for (size_t i=area.coordRange.GetStart()+1; i<=area.coordRange.GetEnd(); i++) {
            CGContextAddLineToPoint(cg,coordBuffer.buffer[i].GetX(),
                        coordBuffer.buffer[i].GetY());
        }
        CGContextClosePath(cg);

        if (!area.clippings.empty()) {
            for (const auto& data : area.clippings) {
                CGContextMoveToPoint(cg,coordBuffer.buffer[data.GetStart()].GetX(),
                            coordBuffer.buffer[data.GetStart()].GetY());
                for (size_t i=data.GetStart()+1; i<=data.GetEnd(); i++) {
                    CGContextAddLineToPoint(cg,coordBuffer.buffer[i].GetX(),
                                coordBuffer.buffer[i].GetY());
                }
                CGContextClosePath(cg);
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
