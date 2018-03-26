/*
  This source is part of the libosmscout-map library
  Copyright (C) 2011  Tim Teulings

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

#define NOMINMAX

#include "osmscout/PathTextRenderer.h"
#include <cmath>
#include <algorithm>

// The identity matrix
const DWRITE_MATRIX identityTransform =
{
    1, 0,
    0, 1,
    0, 0
};

//
// Static creation method that takes care of allocating a renderer
// AND registering it as a COM component.
void PathTextRenderer::CreatePathTextRenderer(float pixelsPerDip, PathTextRenderer **textRenderer)
{
  *textRenderer = nullptr;
  // We assume that the allocation never fails.
  *textRenderer = new PathTextRenderer(pixelsPerDip);
  (*textRenderer)->AddRef();
}

//
// Public destruction method for PathTextRenderer. This method
// does the exact opposite of CreatePathTextRenderer.
//
void PathTextRenderer::DestroyPathTextRenderer(PathTextRenderer *textRenderer)
{
  if (textRenderer == nullptr)
    return;
  textRenderer->Release();
}

// The obvious constructor (private)
PathTextRenderer::PathTextRenderer(float pixelsPerDip) :
  m_pixelsPerDip(pixelsPerDip),
  m_ref(0)
{
}

//
// Draws a given glyph run along the geometry specified
// in the given clientDrawingEffect.
//
/*
 * Draw a glyph run along the path specified in the drawing effect.
*/
HRESULT PathTextRenderer::DrawGlyphRun(
  _In_opt_ void* clientDrawingContext,
  float baselineOriginX,
  float baselineOriginY,
  DWRITE_MEASURING_MODE measuringMode,
  _In_ DWRITE_GLYPH_RUN const* glyphRun,
  _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
  _In_opt_ IUnknown* clientDrawingEffect
)
{
  if (clientDrawingContext == nullptr)
    return S_OK;

  HRESULT hr;

  // We assume it's the correct structure that has been passed along.
  PathTextDrawingContext* dc = static_cast<PathTextDrawingContext*>(clientDrawingContext);

  // We need the current transformation to apply it to the path, and to cleanup
  D2D1_MATRIX_3X2_F prevTransform;
  dc->d2DContext->GetTransform(&prevTransform);

  // Compute the length of the geometry.
  float maxLength;
  hr = dc->geometry->ComputeLength(
    prevTransform, &maxLength);
  if (FAILED(hr))
    return hr;

  float totalRunLength = 0;
  for (int idx = 0; idx < glyphRun->glyphCount; ++idx) {
    totalRunLength += glyphRun->glyphAdvances[idx];
  }

  if (!dc->helper->Init(maxLength, totalRunLength)) {
    return S_OK;
  }

  // Set the initial length along the path.
  //float workingLength = baselineOriginX + std::max((maxLength - totalRunLength) / 2, 0.f);

  while (dc->helper->ContinueDrawing()) {
    // Set the index of the first glyph in the current glyph cluster.
    unsigned clusterStartIdx = 0;

    // Set up a partial glyph run that we can modify.
    DWRITE_GLYPH_RUN workingGlyphRun = *glyphRun;


    while (clusterStartIdx < glyphRun->glyphCount)
    {
      // Compute the number of glyphs in this cluster and the total cluster width.
      size_t clusterGlyphCount = 0;
      float clusterWidth = 0;
      for (size_t i = clusterStartIdx;
           i < glyphRun->glyphCount // We're sill in the glyph run
           && glyphRunDescription->clusterMap[i] == glyphRunDescription->clusterMap[clusterStartIdx]; // And we're still in the current cluster
           ++i)
      {
        clusterWidth += glyphRun->glyphAdvances[i];
        clusterGlyphCount++;
      }

      float currentOffset = dc->helper->GetCurrentOffset();

      // Compute the cluster's midpoint along the path.
      float middle = currentOffset + clusterWidth / 2;

      // If we're out of the path, don't bother with this cluster nor the following ones.
      if (middle >= maxLength)
        break;

      // Compute the offset and tangent to the path at the midpoint.
      D2D1_POINT_2F pathOffset, pathTangent;
      hr = dc->geometry->ComputePointAtLength(middle, D2D1::IdentityMatrix(), &pathOffset, &pathTangent);

      if (hr != S_OK)
        return hr;

      // Lifted from the Microsoft sample.
      // Create a rotation matrix to align the cluster to the path.
      // Alternatively, we could use the D2D1::Matrix3x2F::Rotation()
      // helper, but this is more efficient since we already have cos(t)
      // and sin(t).
      auto rotation = D2D1::Matrix3x2F(
        pathTangent.x,
        pathTangent.y,
        -pathTangent.y,
        pathTangent.x,
        (pathOffset.x * (1.f - pathTangent.x) + pathOffset.y * pathTangent.y),
        (pathOffset.y * (1.f - pathTangent.x) - pathOffset.x * pathTangent.y)
      );

      // Create a translation matrix to center the cluster on the tangent point.
      auto translation = D2D1::Matrix3x2F::Translation(-clusterWidth / 2, glyphRun->fontEmSize / 4);

      // Apply the transformations
      dc->d2DContext->SetTransform(translation * rotation * prevTransform);

      // Draw the glyph cluster.
      workingGlyphRun.glyphCount = clusterGlyphCount;
      dc->d2DContext->DrawGlyphRun(D2D1::Point2F(pathOffset.x, pathOffset.y), &workingGlyphRun, dc->brush);

      // Go to the next cluster
      workingGlyphRun.glyphAdvances += clusterGlyphCount;
      workingGlyphRun.glyphIndices += clusterGlyphCount;
      dc->helper->AdvancePartial(clusterWidth);

      clusterStartIdx += clusterGlyphCount;
    }

    dc->helper->AdvanceSpace();
  }

  // Cleanup the transformation.
  dc->d2DContext->SetTransform(prevTransform);

  return S_OK;
}

HRESULT PathTextRenderer::DrawUnderline(
  _In_opt_ void* clientDrawingContext,
  float baselineOriginX,
  float baselineOriginY,
  _In_ DWRITE_UNDERLINE const* underline,
  _In_opt_ IUnknown* clientDrawingEffect
)
{
  // NOPE.
  return E_NOTIMPL;
}

HRESULT PathTextRenderer::DrawStrikethrough(
  _In_opt_ void* clientDrawingContext,
  float baselineOriginX,
  float baselineOriginY,
  _In_ DWRITE_STRIKETHROUGH const* strikethrough,
  _In_opt_ IUnknown* clientDrawingEffect
)
{
  // NOPE
  return E_NOTIMPL;
}

HRESULT PathTextRenderer::DrawInlineObject(
  _In_opt_ void* clientDrawingContext,
  float originX,
  float originY,
  IDWriteInlineObject* inlineObject,
  BOOL isSideways,
  BOOL isRightToLeft,
  _In_opt_ IUnknown* clientDrawingEffect
)
{
  // Hell no.
  return E_NOTIMPL;
}

//
// IDWritePixelSnapping methods
//
HRESULT PathTextRenderer::IsPixelSnappingDisabled(
  _In_opt_ void* clientDrawingContext,
  _Out_ BOOL* isDisabled
)
{
  // yes, we want pixel snapping ?
  *isDisabled = false;
  return S_OK;
}

HRESULT PathTextRenderer::GetCurrentTransform(
  _In_opt_ void* clientDrawingContext,
  _Out_ DWRITE_MATRIX* transform
)
{
  // Since the transform really depends on the text effect, we don't have a general one.
  *transform = identityTransform;
  return S_OK;
}

HRESULT PathTextRenderer::GetPixelsPerDip(
  _In_opt_ void* clientDrawingContext,
  _Out_ float* pixelsPerDip
)
{
  *pixelsPerDip = m_pixelsPerDip;
  return S_OK;
}

//
// IUnknown methods
//
// These use a basic, non-thread-safe implementation of the
// standard reference-counting logic.
//
// Lifted from the MS samples.
HRESULT PathTextRenderer::QueryInterface(
  REFIID riid,
  _Outptr_ void** object
)
{
  *object = nullptr;
  return E_NOTIMPL;
}

ULONG PathTextRenderer::AddRef()
{
  m_ref++;

  return m_ref;
}

ULONG PathTextRenderer::Release()
{
  m_ref--;

  if (m_ref == 0)
  {
    delete this;
    return 0;
  }

  return m_ref;
}
