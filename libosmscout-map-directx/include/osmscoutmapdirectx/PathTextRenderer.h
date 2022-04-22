#pragma once

#define NOMINMAX 1

#include <d2d1.h>
#include <dwrite.h>

#include <osmscoutmap/MapPainter.h>

/**
 * Helper class for drawing contours. Allows the MapPainter base class
 * to inject itself at certain points in the contour label rendering code of
 * the actual backend.
 */
class ContourLabelHelper
{
private:
  double contourLabelOffset;
  double contourLabelSpace;
  double pathLength;
  double textWidth;
  double currentOffset;
public:
  explicit ContourLabelHelper(double contourLabelOffset, double contourLabelSpace);

  bool Init(double pathLength,
            double textWidth);

  inline bool ContinueDrawing() const
  {
    return currentOffset<pathLength;
  }

  inline double GetCurrentOffset() const
  {
    return currentOffset;
  }

  inline void AdvancePartial(double width)
  {
    currentOffset+=width;
  }

  inline void AdvanceText()
  {
    currentOffset+=textWidth;
  }

  inline void AdvanceSpace()
  {
    currentOffset+=contourLabelSpace;
  }
};

/*
 * This structure is needed to hold the renderer context. It is passed around as a void* pointer,
 * typically the clientDrawingContext in the methods below.
 */
struct PathTextDrawingContext
{
    ContourLabelHelper* helper;
    ID2D1RenderTarget* d2DContext;
    ID2D1Geometry* geometry;
    ID2D1Brush* brush;
};

class PathTextRenderer final : public IDWriteTextRenderer
{
public:
  // Static creation method that takes care of allocating a renderer
  // AND registering it as a COM component.
  static void CreatePathTextRenderer(float pixelsPerDip, PathTextRenderer **textRenderer);

  //
  // Public destruction method for PathTextRenderer. This method
  // does the exact opposite of CreatePathTextRenderer.
  //
  static void DestroyPathTextRenderer(PathTextRenderer *textRenderer);

  // All the STDMETHOD have been lifted from the IDWriteTextRenderer interface
    STDMETHOD(DrawGlyphRun)(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        _In_ DWRITE_GLYPH_RUN const* glyphRun,
        _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        _In_opt_ IUnknown* clientDrawingEffect
        ) override;

    STDMETHOD(DrawUnderline)(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ DWRITE_UNDERLINE const* underline,
        _In_opt_ IUnknown* clientDrawingEffect
        ) override;

    STDMETHOD(DrawStrikethrough)(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ DWRITE_STRIKETHROUGH const* strikethrough,
        _In_opt_ IUnknown* clientDrawingEffect
        ) override;

    STDMETHOD(DrawInlineObject)(
        _In_opt_ void* clientDrawingContext,
        FLOAT originX,
        FLOAT originY,
        IDWriteInlineObject* inlineObject,
        BOOL isSideways,
        BOOL isRightToLeft,
        _In_opt_ IUnknown* clientDrawingEffect
        ) override;

    STDMETHOD(IsPixelSnappingDisabled)(
        _In_opt_ void* clientDrawingContext,
        _Out_ BOOL* isDisabled
        ) override;

    STDMETHOD(GetCurrentTransform)(
        _In_opt_ void* clientDrawingContext,
        _Out_ DWRITE_MATRIX* transform
        ) override;

    STDMETHOD(GetPixelsPerDip)(
        _In_opt_ void* clientDrawingContext,
        _Out_ FLOAT* pixelsPerDip
        ) override;

    STDMETHOD(QueryInterface)(
        REFIID riid,
        _Outptr_ void** object
        ) override;

    STDMETHOD_(ULONG, AddRef)() override;

    STDMETHOD_(ULONG, Release)() override;

private:
    float m_pixelsPerDip;   // Number of pixels per DIP.
    UINT m_ref;             // Reference count for AddRef and Release.

    PathTextRenderer(float pixelsPerDip);
};
