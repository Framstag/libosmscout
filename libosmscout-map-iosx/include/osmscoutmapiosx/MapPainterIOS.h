#ifndef OSMSCOUT_MAP_MAPPAINTERIOS_H
#define OSMSCOUT_MAP_MAPPAINTERIOS_H

/*
 This source is part of the libosmscout-map library
 Copyright (C) 2010  Tim Teulings, Vladimir Vyskocil

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
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#include <string>
#include <unordered_map>
#include <utility>

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#define Font UIFont
#define Image UIImage
#else
#import <AppKit/AppKit.h>
#define Font NSFont
#define Image NSImage
#define OSMSCOUT_REVERSED_Y_AXIS 1
#endif

#import <CoreText/CoreText.h>
#include <osmscoutmap/MapPainter.h>

template<class T>
class cfref_ptr
{
public:
    cfref_ptr()
    : obj_(NULL)
    {}

    cfref_ptr(T obj)
    : obj_(obj)
    {
    }

    ~cfref_ptr()
    {
        if (obj_) {
            CFRelease(obj_);
        }
    }

    cfref_ptr(cfref_ptr const& other)
    : obj_(other.obj_)
    {
        if (obj_) {
            CFRetain(obj_);
        }
    }

    cfref_ptr(cfref_ptr&& other) noexcept
    {
        *this = std::move(other);
    }

    cfref_ptr& operator = (cfref_ptr const& other)
    {
        if (this != &other) {
            cfref_ptr tmp(other);
            tmp.swap(*this);
        }
        return *this;
    }

    cfref_ptr& operator = (cfref_ptr&& other)
    {
        if (this != &other) {
            obj_ = other.obj_;
            other.obj_ = NULL;
        }
        return *this;
    }

    bool boolean_test() const
    {
        return !!obj_;
    }

    T get()
    {
        return obj_;
    }

    T release()
    {
        T ret = obj_;
        obj_ = NULL;
        return ret;
    }

    void swap(cfref_ptr& other)
    {
        std::swap(obj_, other.obj_);
    }

    void reset(T obj = NULL)
    {
        if (obj_) {
            CFRelease(obj_);
        }
        obj_ = obj;
    }

private:
    T obj_;
};

namespace osmscout {
    struct IOSGlyphInRun {
        cfref_ptr<CTLineRef> line;
        CTRunRef run;
        int index;
    };
    struct IOSRunInLine {
        std::vector<cfref_ptr<CTLineRef>> line;
        std::vector<CTRunRef> run;
        CGFloat lineWidth;
        CGFloat lineHeight;
    };
    using IOSGlyph = Glyph<IOSGlyphInRun>;
    using IOSLabel = Label<IOSGlyphInRun, IOSRunInLine>;

    class MapPainterIOS : public MapPainter {
    public:
        typedef struct {
            bool closeWay;
            size_t transStart;
            size_t transEnd;
            size_t i;
            size_t nVertex;
            ssize_t direction;
        } FollowPathHandle;

        using IOSLabelLayouter = LabelLayouter<IOSGlyphInRun, IOSRunInLine, MapPainterIOS>;
        friend IOSLabelLayouter;

    private:
        const float                 ProposedWidthExpandRatio = 1.6;
        const float                 ProposedWidthMinWidth = 50.0;

        CGContextRef                cg;
        CGFloat                     contentScale;

        IOSLabelLayouter            labelLayouter;

        std::vector<CGImageRef>     images;             // Cached CGImage for icons
        std::vector<CGImageRef>     patternImages;      // Cached CGImage for patterns
        std::map<size_t,Font *>     fonts;              // Cached fonts
        std::map<size_t,double>     averageCharWidth;   // Average char width for a font size

    public:
        OSMSCOUT_API MapPainterIOS(const StyleConfigRef& styleConfig);
        virtual ~MapPainterIOS();

        OSMSCOUT_API bool DrawMap(const StyleConfig& styleConfig,
                                  const Projection& projection,
                                  const MapParameter& parameter,
                                  const MapData& data,
                                  CGContextRef paintCG);

        OSMSCOUT_API bool DrawMap(const StyleConfig& styleConfig,
                                  const Projection& projection,
                                  const MapParameter& parameter,
                                  const MapData& data,
                                  CGContextRef paintCG,
                                  RenderSteps startStep,
                                  RenderSteps endStep);

        OSMSCOUT_API void DrawGroundTiles(const Projection& projection,
                                          const MapParameter& parameter,
                                          const std::list<GroundTile>& groundTiles,
                                          CGContextRef paintCG);
    protected:
        bool HasIcon(const StyleConfig& styleConfig,
                     const Projection& projection,
                     const MapParameter& parameter,
                     IconStyle& style) override;

        bool HasPattern(const MapParameter& parameter,
                        const FillStyle& style);

        double GetFontHeight(const Projection& projection,
                             const MapParameter& parameter,
                             double fontSize) override;

        void DrawContourSymbol(const Projection& projection,
                               const MapParameter& parameter,
                               const Symbol& symbol,
                               const ContourSymbolData& data) override;

        void DrawLabel(const Projection& projection,
                       const MapParameter& parameter,
                       const ScreenVectorRectangle& labelRectangle,
                       const LabelData& label,
                       const IOSRunInLine& layout);

        virtual void BeforeDrawing(const StyleConfig& styleConfig,
                                   const Projection& projection,
                                   const MapParameter& parameter,
                                   const MapData& data) override;

        virtual void RegisterRegularLabel(const Projection &projection,
                                          const MapParameter &parameter,
                                          const ObjectFileRef& ref,
                                          const std::vector<LabelData> &labels,
                                          const Vertex2D &position,
                                          double objectWidth) override;

        virtual void RegisterContourLabel(const Projection &projection,
                                          const MapParameter &parameter,
                                          const ObjectFileRef& ref,
                                          const PathLabelData &label,
                                          const LabelPath &labelPath) override;

        virtual void DrawLabels(const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data) override;

        void DrawIcon(const IconStyle* style,
                      const Vertex2D& centerPos,
                      double width, double height) override;

        void DrawSymbol(const Projection& projection,
                        const MapParameter& parameter,
                        const Symbol& symbol,
                        const Vertex2D& screenPos,
                        double scaleFactor) override;

        void DrawPath(const Projection& projection,
                      const MapParameter& parameter,
                      const Color& color,
                      double width,
                      const std::vector<double>& dash,
                      LineStyle::CapStyle startCap,
                      LineStyle::CapStyle endCap,
                      const CoordBufferRange& coordRange) override;

        void DrawArea(const Projection& projection,
                      const MapParameter& parameter,
                      const AreaData& area) override;

        void DrawGround(const Projection& projection,
                        const MapParameter& parameter,
                        const FillStyle& style) override;

        void SetBorder(const Projection& projection,
                       const MapParameter& parameter,
                       const BorderStyle& borderStyle);

        void SetFill(const Projection& projection,
                     const MapParameter& parameter,
                     const FillStyle& fillStyle,
                     CGFloat xOffset=0.0,
                     CGFloat yOffset=0.0);

        void SetPen(const LineStyle& style,
                    double lineWidth);

        double textLength(const Projection& projection, const MapParameter& parameter, double fontSize, std::string text);
        double textHeight(const Projection& projection, const MapParameter& parameter, double fontSize, std::string text);

    private:
        Font *GetFont(const Projection& projection, const MapParameter& parameter, double fontSize);
        double GetAverageCharWidth(const Projection& projection,
                                   const MapParameter& parameter,
                                   double fontSize);
        bool followPath(FollowPathHandle &hnd, double l, Vertex2D &origin);
        void followPathInit(FollowPathHandle &hnd, Vertex2D &origin, size_t transStart, size_t transEnd, bool isClosed, bool keepOrientation);
        std::shared_ptr<IOSLabel> Layout(const Projection& projection,
                                         const MapParameter& parameter,
                                         const std::string& text,
                                         double fontSize,
                                         double objectWidth,
                                         bool enableWrapping = false,
                                         bool contourLabel = false);
        ScreenVectorRectangle GlyphBoundingBox(const IOSGlyphInRun &glyph) const;
        void DrawGlyphs(const Projection &projection,
                        const MapParameter &parameter,
                        const osmscout::PathTextStyleRef style,
                        const std::vector<IOSGlyph> &glyphs);
        void LayoutDrawLabel(const IOSRunInLine& layout,
                             const CGPoint& coords,
                             const Color &color,
                             bool emphasize);
    };
}

#endif
