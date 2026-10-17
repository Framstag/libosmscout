/*
 * render_label_bounds.h — the label line-wrapping bounds the JNI map render
 * states on its rendering parameter (openspec change
 * "client-java-render-label-wrap-bounds").
 *
 * The render stated no line-wrapping value at all and inherited the library
 * defaults. Three of them already do what this app wants — minimum character
 * count 5, maximum character count 15, fit-to-area enabled — but the fourth, a
 * fit-to-width of 8000 px, does not bind in a viewport of any phone or desktop
 * size. The pixel bound is what makes a long multi-word label wrap, so such a
 * label was drawn on one line and ran across the map however narrow the
 * viewport was. The render therefore states its own fit-to-width: the shorter
 * side of the render buffer.
 *
 * The buffer is the one the caller asked for and carries the pan overrun, so
 * it is larger than the visible viewport; the shorter side is the width of a
 * tall buffer and the height of a wide one. The bound has no division by the
 * canvas overrun here, unlike the Qt plane renderer, whose projection canvas
 * does not carry one: this buffer already does.
 *
 * The decision lives here as a pure function so the render and the host test
 * read the same four values and cannot drift, and so the difference from the
 * non-binding default is pinned without a map database. Kept dependency-free
 * on purpose (no libosmscout includes, no JNI).
 */
#ifndef NAVIVEYLIN_RENDER_LABEL_BOUNDS_H
#define NAVIVEYLIN_RENDER_LABEL_BOUNDS_H

#include <algorithm>
#include <cstddef>

namespace naviveylin {

/**
 * The four label line-wrapping values a render states on its rendering
 * parameter: the character counts between which a label line is wrapped, the
 * fit-to-area flag and the pixel width a line must not exceed.
 */
struct LabelLineBounds
{
  std::size_t minCharCount; //!< Shortest label the layouter wraps
  std::size_t maxCharCount; //!< Longest label the layouter keeps on one line
  bool        fitToArea;    //!< Whether a label is also fitted to its area
  double      fitToWidth;   //!< Widest line, on the shorter buffer side
};

/**
 * The label line-wrapping bounds for a render buffer of `width` x `height`.
 *
 * The character bounds and the fit-to-area flag are the ones this app wants
 * stated rather than inherited, so the render does not depend on a default it
 * means to state. The fit-to-width is the shorter buffer side, never the
 * library default of 8000 px, which binds no label in a viewport of any phone
 * or desktop size.
 */
inline LabelLineBounds LabelLineBoundsForBuffer(double width, double height)
{
  return LabelLineBounds{5, 15, true, std::min(width, height)};
}

} // namespace naviveylin

#endif // NAVIVEYLIN_RENDER_LABEL_BOUNDS_H
