#ifndef OSMSCOUT_MAP_LABELLAYOUTER_H
#define OSMSCOUT_MAP_LABELLAYOUTER_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2018 Lukas Karas

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

#include <memory>
#include <set>

#include <osmscout/MapImportExport.h>

#include <osmscout/StyleConfig.h>
#include <osmscout/LabelPath.h>
#include <iostream>

// #define DEBUG_LABEL_LAYOUTER

#if defined(LABEL_LAYOUTER_DEBUG)
#include <iostream>
#endif

namespace osmscout {

  template <typename T>
  struct Rectangle {
    T x;
    T y;
    T width;
    T height;

    // not initialised viewport
    Rectangle() {}

    Rectangle(T x, T y, T width, T height):
        x(x), y(y), width(width), height(height)
    {
    }

    Rectangle<T>& Set(T nx, T ny, T nw, T nh)
    {
      x = nx;
      y = ny;
      width = nw;
      height = nh;
      return *this;
    }

    /**
     * Test if this Rectangle intersects with another.
     * It is using open interval, so if two rectangles are just touching
     * each other, these don't intersects.
     *
     * @param other
     * @return true if intersects
     */
    inline bool Intersects(const Rectangle<T> &other)
    {
      return !(
          (x + width) < other.x ||
          x > (other.x + other.width) ||
          (y + height) < other.y ||
          y > (other.y + other.height)
      );
    }
  };

  using IntRectangle = Rectangle<int>;
  using DoubleRectangle = Rectangle<double>;

  class OSMSCOUT_MAP_API PathLabelData
  {
  public:
    size_t            priority{0}; //!< Priority of the entry
    std::string       text;        //!< The label text (type==Text|PathText)
    PathTextStyleRef  style;
    double            contourLabelOffset;
    double            contourLabelSpace;
  };

  class OSMSCOUT_MAP_API LabelData
  {
  public:
    enum Type
    {
      Icon,
      Symbol,
      Text
    };
  public:
    Type              type{Type::Text};
    size_t            priority{0}; //!< Priority of the entry
    size_t            position{0}; //!< Relative position of the label

    double            alpha{1.0};   //!< Alpha value of the label; 0.0 = fully transparent, 1.0 = solid
    double            fontSize{0};  //!< Font size to be used

    LabelStyleRef     style;    //!< Style for drawing
    std::string       text;     //!< The label text (type==Text|PathText)

    IconStyleRef      iconStyle; //!< Icon or symbol style
    double            iconWidth{0};
    double            iconHeight{0};

  public:
    LabelData(){};
    ~LabelData(){};
  };


  template<class NativeGlyph>
  class Glyph {
  public:
    NativeGlyph glyph;
    osmscout::Vertex2D position;        //!< glyph baseline position
    double angle{0};                    //!< clock-wise rotation in radians

    osmscout::Vertex2D trPosition{0,0}; //!< top-left position after rotation
    double trWidth{0};                  //!< width after rotation
    double trHeight{0};                 //!< height after rotation
  };

  /**
   * Position independent representation of layouted label
   */
  template<class NativeGlyph, class NativeLabel>
  class Label
  {
  public:
    NativeLabel             label;

    double                  width{-1};
    double                  height{-1};

    double                  fontSize{1}; //!< Font size to be used
    std::string             text;     //!< The label text

    Label() {};

    template<typename... Args>
    Label(Args&&... args):
      label(std::forward<Args>(args)...)
    {}

    /**
     * Implementation have to be provided by backend.
     * Glyph positions should be relative to label baseline.
     *
     * @return vector of glyphs
     */
    std::vector<Glyph<NativeGlyph>> ToGlyphs() const;
  };

  template<class NativeGlyph, class NativeLabel>
  class LabelInstance
  {
  public:
    struct Element
    {
      LabelData labelData;
      double    x;        //!< Coordinate of the left, top edge of the text / icon / symbol
      double    y;        //!< Coordinate of the left, top edge of the text / icon / symbol
      std::shared_ptr<Label<NativeGlyph, NativeLabel>>
                label;
    };

  public:
    size_t                priority; //!< Priority of the entry (minimum of priority label elements)
    // TODO: move priority from label to element
    std::vector<Element>  elements;
  };

  template<class NativeGlyph>
  class ContourLabel
  {
  public:
    size_t priority;
    std::vector<Glyph<NativeGlyph>> glyphs;
    osmscout::PathTextStyleRef style;    //!< Style for drawing
  };

  class OSMSCOUT_MAP_API Mask
  {
  public:
    Mask(size_t rowSize) : d(rowSize)
    {
    };

    Mask(const Mask &m) :
        d(m.d), cellFrom(m.cellFrom), cellTo(m.cellTo), rowFrom(m.rowFrom), rowTo(m.rowTo)
    {
    };

    ~Mask()
    {
    }

    Mask(Mask &&m) = delete;
    Mask &operator=(const Mask &m) = delete;
    Mask &operator=(Mask &&m) = delete;

    void prepare(const IntRectangle &rect);

    inline int64_t size() const
    { return d.size(); };

    std::vector<uint64_t> d;

    int cellFrom{0};
    int cellTo{0};
    int rowFrom{0};
    int rowTo{0};
  };

  template <class NativeGlyph, class NativeLabel>
  static bool LabelInstanceSorter(const LabelInstance<NativeGlyph, NativeLabel> &a,
                                  const LabelInstance<NativeGlyph, NativeLabel> &b)
  {
    if (a.priority == b.priority) {
      assert(!a.elements.empty());
      assert(!b.elements.empty());
      return a.elements[0].x < b.elements[0].x;
    }
    return a.priority < b.priority;
  }

  template <class NativeGlyph>
  static bool ContourLabelSorter(const ContourLabel<NativeGlyph> &a,
                                 const ContourLabel<NativeGlyph> &b)
  {
    if (a.priority == b.priority) {
      assert(!a.glyphs.empty());
      assert(!b.glyphs.empty());
      return a.glyphs[0].trPosition.GetX() < b.glyphs[0].trPosition.GetX();
    }
    return a.priority < b.priority;
  }

  /**
   *
   * @tparam NativeGlyph
   * @tparam NativeLabel
   * @tparam TextLayouter - class providing low level text layouting
   *   required methods:
   *
   *    // glyph bounding box relative to its base point
   *    DoubleRectangle GlyphBoundingBox(const NativeGlyph &) const
   *
   *    // layout text for label
   *    std::shared_ptr<Label<NativeGlyph, NativeLabel>> Layout(
   *                                        const Projection& projection,
   *                                        const MapParameter& parameter,
   *                                        const std::string& text,
   *                                        double fontSize,
   *                                        double objectWidth,
   *                                        bool enableWrapping = false,
   *                                        bool contourLabel = false);
   *
   */
  template <class NativeGlyph, class NativeLabel, class TextLayouter>
  class OSMSCOUT_MAP_API LabelLayouter
  {

  public:
    using ContourLabelType = ContourLabel<NativeGlyph>;
    using LabelType = Label<NativeGlyph, NativeLabel>;
    using LabelPtr = std::shared_ptr<LabelType>;
    using LabelInstanceType = LabelInstance<NativeGlyph, NativeLabel>;

  public:
    LabelLayouter(TextLayouter *textLayouter):
        textLayouter(textLayouter),
        visibleViewport{0,0,0,0},
        layoutViewport{0,0,0,0},
        layoutOverlap{0}
    {};

    void SetViewport(DoubleRectangle v)
    {
      visibleViewport = v;
      SetLayoutOverlap(layoutOverlap);
    }

    void SetLayoutOverlap(double overlap)
    {
      if (overlap < 0){
        overlap = 0;
      }
      layoutOverlap = overlap;
      layoutViewport.width = visibleViewport.width * (overlap + 1);
      layoutViewport.height = visibleViewport.height * (overlap + 1);
      layoutViewport.x = visibleViewport.x - (visibleViewport.width * overlap) / 2;
      layoutViewport.y = visibleViewport.y - (visibleViewport.height * overlap) / 2;
    }

    void Reset()
    {
      contourLabelInstances.clear();
      labelInstances.clear();
    }

    inline bool CheckLabelCollision(const std::vector<uint64_t> &canvas,
                                    const Mask &mask,
                                    int64_t viewportHeight
    )
    {
      bool collision=false;
      for (int r=std::max(0,mask.rowFrom); !collision && r<=std::min((int)viewportHeight-1, mask.rowTo); r++){
        for (int c=std::max(0,mask.cellFrom); !collision && c<=std::min((int)mask.size()-1,mask.cellTo); c++){
          collision |= (mask.d[c] & canvas[r*mask.size() + c]) != 0;
        }
      }
      return collision;
    }

    inline void MarkLabelPlace(std::vector<uint64_t> &canvas,
                               const Mask &mask,
                               int viewportHeight
    )
    {
      for (int r=std::max(0,mask.rowFrom); r<=std::min((int)viewportHeight-1, mask.rowTo); r++){
        for (int c=std::max(0,mask.cellFrom); c<=std::min((int)mask.size()-1, mask.cellTo); c++){
          canvas[r*mask.size() + c] = mask.d[c] | canvas[r*mask.size() + c];
        }
      }
    }

    void Layout()
    {
      std::vector<ContourLabelType> allSortedContourLabels;
      std::vector<LabelInstanceType> allSortedLabels;

      std::swap(allSortedLabels, labelInstances);
      std::swap(allSortedContourLabels, contourLabelInstances);

      // sort labels by priority and position (to be deterministic)
      std::stable_sort(allSortedLabels.begin(),
                       allSortedLabels.end(),
                       LabelInstanceSorter<NativeGlyph, NativeLabel>);
      std::stable_sort(allSortedContourLabels.begin(),
                       allSortedContourLabels.end(),
                       ContourLabelSorter<NativeGlyph>);

      // compute collisions, hide some labels
      int64_t rowSize = (layoutViewport.width / 64)+1;
      std::vector<uint64_t> iconCanvas((size_t)(rowSize*layoutViewport.height));
      std::vector<uint64_t> labelCanvas((size_t)(rowSize*layoutViewport.height));
      std::vector<uint64_t> overlayCanvas((size_t)(rowSize*layoutViewport.height));

      auto labelIter = allSortedLabels.begin();
      auto contourLabelIter = allSortedContourLabels.begin();
      while (labelIter != allSortedLabels.end()
             || contourLabelIter != allSortedContourLabels.end()) {

        auto currentLabel = labelIter;
        auto currentContourLabel = contourLabelIter;
        if (currentLabel != allSortedLabels.end()
            && currentContourLabel != allSortedContourLabels.end()) {
          if (currentLabel->priority != currentContourLabel->priority) {
            if (currentLabel->priority < currentContourLabel->priority) {
              currentContourLabel = allSortedContourLabels.end();
            } else {
              currentLabel = allSortedLabels.end();
            }
          }
        }

        if (currentLabel != allSortedLabels.end()){

          Mask m(rowSize);
          std::vector<Mask> masks(currentLabel->elements.size(), m);
          std::vector<std::vector<uint64_t> *> canvases(currentLabel->elements.size(), nullptr);

          std::vector<typename LabelInstance<NativeGlyph, NativeLabel>::Element> visibleElements;

          for (size_t eli=0; eli < currentLabel->elements.size(); eli++){
            const typename LabelInstance<NativeGlyph, NativeLabel>::Element& element = currentLabel->elements[eli];
            Mask& row=masks[eli];

            IntRectangle rectangle{ (int)std::floor(element.x-layoutViewport.x),
                                    (int)std::floor(element.y-layoutViewport.y),
                                    0, 0 };
            std::vector<uint64_t> *canvas = &labelCanvas;
            if (element.labelData.type==LabelData::Icon || element.labelData.type==LabelData::Symbol){
              rectangle.width = std::ceil(element.labelData.iconWidth);
              rectangle.height = std::ceil(element.labelData.iconHeight);
              canvas = &iconCanvas;
            } else {
#ifdef DEBUG_LABEL_LAYOUTER
              std::cout << "Test label prio " << currentLabel->priority << ": " << element.labelData.text << std::endl;
#endif

              rectangle.width = std::ceil(element.label->width);
              rectangle.height = std::ceil(element.label->height);
              // Something is an overlay, if its alpha is <0.8
              if (element.labelData.alpha < 0.8){
                canvas = &overlayCanvas;
              }
            }
            row.prepare(rectangle);
            bool collision = CheckLabelCollision(*canvas, row, layoutViewport.height);
            if (!collision) {
              visibleElements.push_back(element);
              canvases[eli]=canvas;
            }
          }
          LabelInstanceType instanceCopy;
          instanceCopy.priority = currentLabel->priority;
          instanceCopy.elements = visibleElements;
          if (!instanceCopy.elements.empty()) {
            labelInstances.push_back(instanceCopy);

            // mark all labels at once
            for (size_t eli=0; eli < currentLabel->elements.size(); eli++) {
              if (canvases[eli] != nullptr) {
                MarkLabelPlace(*(canvases[eli]), masks[eli], layoutViewport.height);
              }
            }
          }

          labelIter++;
        }

        if (currentContourLabel != allSortedContourLabels.end()){
          int glyphCnt=currentContourLabel->glyphs.size();

#ifdef DEBUG_LABEL_LAYOUTER
          std::cout << "Test contour label prio " << currentContourLabel->priority << std::endl;
#endif

          Mask m(rowSize);
          std::vector<Mask> masks(glyphCnt, m);
          bool collision=false;
          for (int gi=0; !collision && gi<glyphCnt; gi++) {

            auto glyph=currentContourLabel->glyphs[gi];
            IntRectangle rect{
                (int)(glyph.trPosition.GetX()-layoutViewport.x),
                (int)(glyph.trPosition.GetY()-layoutViewport.y),
                (int)glyph.trWidth,
                (int)glyph.trHeight
            };
            masks[gi].prepare(rect);
            collision |= CheckLabelCollision(labelCanvas, masks[gi], layoutViewport.height);
          }
          if (!collision) {
            for (int gi=0; gi<glyphCnt; gi++) {
              MarkLabelPlace(labelCanvas, masks[gi], layoutViewport.height);
            }
            contourLabelInstances.push_back(*currentContourLabel);
          }
          contourLabelIter++;
        }
      }
    }

    /**
     *
     * @tparam Painter
     *  required methods:
     *
     *      void DrawSymbol(const Projection& projection,
     *                      const MapParameter& parameter,
     *                      onst Symbol& symbol,
     *                      double x, double y) override;
     *
     *      void DrawIcon(const IconStyle* style,
     *                    double x, double y) override;
     *
     *      void DrawLabel(const Projection& projection,
     *                     const MapParameter& parameter,
     *                     const DoubleRectangle& labelRectangle,
     *                     const LabelData& label,
     *                     const std::shared_ptr<NativeLabel>& layout);
     *
     *      void DrawGlyphs(const Projection &projection,
     *                      const MapParameter &parameter,
     *                      const osmscout::PathTextStyleRef style,
     *                      const std::vector<Glyph<NativeGlyph>> &glyphs);
     *
     * @param projection
     * @param parameter
     * @param p - painter pointer
     */
    template<class Painter>
    void DrawLabels(const Projection& projection,
                    const MapParameter& parameter,
                    Painter *p)
    {
      // draw symbols and icons first, then standard labels and then overlays
      std::vector<const typename LabelInstanceType::Element*> textElements;

      for (const LabelInstanceType &inst : Labels()){

        for (const typename LabelInstanceType::Element &el : inst.elements) {
          DoubleRectangle elementRectangle;
          if (el.labelData.type==LabelData::Text) {
            elementRectangle.Set(el.x, el.y, el.label->width, el.label->height);
          }else{
            elementRectangle.Set(el.x, el.y, el.labelData.iconWidth, el.labelData.iconHeight);
          }

          if (!visibleViewport.Intersects(elementRectangle)){
            continue;
          }

          if (el.labelData.type==LabelData::Symbol){
            p->DrawSymbol(projection,
                          parameter,
                          *(el.labelData.iconStyle->GetSymbol()),
                          el.x + el.labelData.iconWidth/2,
                          el.y + el.labelData.iconHeight/2);

          } else if (el.labelData.type==LabelData::Icon){
            p->DrawIcon(el.labelData.iconStyle.get(),
                        el.x + el.labelData.iconWidth/2,
                        el.y + el.labelData.iconHeight/2);

          } else {
            // postpone text elements
            textElements.push_back(&el);
          }
        }
      }

      // draw postponed text elements
      for (const typename LabelInstanceType::Element *el : textElements) {

        p->DrawLabel(projection, parameter,
                     DoubleRectangle(el->x, el->y, el->label->width, el->label->height),
                     el->labelData, el->label->label);
      }

      for (const ContourLabelType &label:ContourLabels()){
        p->DrawGlyphs(projection, parameter,
                      label.style, label.glyphs);
      }
    }

    void RegisterLabel(const Projection& projection,
                       const MapParameter& parameter,
                       const Vertex2D& point,
                       const std::vector<LabelData> &data,
                       double objectWidth = 10.0)
    {
      LabelInstanceType instance;
      instance.priority = std::numeric_limits<size_t>::max();

      double offset=-1;
      for (const auto &d:data) {
        typename LabelInstance<NativeGlyph, NativeLabel>::Element element;
        element.labelData=d;
        if (d.type==LabelData::Type::Icon || d.type==LabelData::Type::Symbol){
          // TODO: icons and symbols don't support priority now
          element.x = point.GetX() - d.iconWidth / 2;
          if (offset<0){
            element.y = point.GetY() - d.iconHeight / 2;
            offset = point.GetY() + d.iconHeight / 2;
          } else {
            element.y = offset;
            offset += d.iconHeight;
          }
        }else {
          instance.priority = std::min(d.priority, instance.priority);
          // TODO: should we take style into account?
          // Qt allows to split text layout and style setup
          element.label = textLayouter->Layout(projection, parameter,
                                               d.text, d.fontSize,
                                               objectWidth,
                                               /*enable wrapping*/ true,
                                               /*contour label*/ false);
          element.x = point.GetX() - element.label->width / 2;
          if (offset<0){
            element.y = point.GetY() - element.label->height / 2;
            offset = point.GetY() + element.label->height / 2;
          } else {
            element.y = offset;
            offset += element.label->height;
          }
        }
        instance.elements.push_back(element);
      }

      labelInstances.push_back(instance);
    }

    void RegisterContourLabel(const Projection& projection,
                              const MapParameter& parameter,
                              const PathLabelData &labelData,
                              const LabelPath &labelPath)
    {
      // TODO: cache label for string and font parameters
      LabelPtr label = textLayouter->Layout(
          projection,
          parameter,
          labelData.text,
          labelData.style->GetSize(),
          /* object width */ 0.0,
          /*enable wrapping*/ false,
          /*contour label*/ true);

      // text should be rendered with 0x0 coordinate as left baseline
      // we want to move label little bit bottom, near to line center
      double textBaselineOffset = label->height * 0.25;

      std::vector<Glyph<NativeGlyph>> glyphs = label->ToGlyphs();

      double pLength=labelPath.GetLength();
      double offset=labelData.contourLabelOffset;
      while (offset+label->width < pLength){

        // skip string rendering when path is too much squiggly at this offset
        if (!labelPath.TestAngleVariance(offset,offset+label->width,M_PI_4)){
          // skip drawing current label and let offset point to the next instance
          offset+=label->width + labelData.contourLabelSpace;
          continue;
        }

        ContourLabelType cLabel;
        cLabel.priority = labelData.priority;
        cLabel.style = labelData.style;

        // do the magic to make sure that we don't render label upside-down

        // direction of path at the label drawing starting point
        double initialAngle=std::abs(labelPath.AngleAtLengthDeg(offset));
        bool upwards=initialAngle>90 && initialAngle<270;

        for (const Glyph<NativeGlyph> &glyph:glyphs){
          double glyphOffset = upwards ?
                               offset - glyph.position.GetX() + label->width:
                               offset + glyph.position.GetX();
          osmscout::Vertex2D point=labelPath.PointAtLength(glyphOffset);

          DoubleRectangle textBoundingBox = textLayouter->GlyphBoundingBox(glyph.glyph);
          double w = textBoundingBox.width;
          double h = textBoundingBox.height;
          osmscout::Vertex2D tl(textBoundingBox.x, textBoundingBox.y);

          // glyph angle in radians
          double angle=labelPath.AngleAtLength(upwards ? glyphOffset - w/2 : glyphOffset + w/2)*-1;

          // it is not real diagonal, but maximum distance from glyph
          // point that can be covered after treansformantions
          double diagonal=w+h+std::abs(textBaselineOffset);

          // fast check if current glyph can be visible
          if (!layoutViewport.Intersects(DoubleRectangle{
            point.GetX()-diagonal,
            point.GetY()-diagonal,
            2*diagonal,
            2*diagonal
          })){
            continue;
          }

          if (upwards) {
            angle-=M_PI;
          }
          double  sinA=std::sin(angle);
          double  cosA=std::cos(angle);

          Glyph<NativeGlyph> glyphCopy=glyph;
          glyphCopy.position=osmscout::Vertex2D(point.GetX() - textBaselineOffset * sinA,
                                                point.GetY() + textBaselineOffset * cosA);
          glyphCopy.angle=angle;

          // four coordinates of glyph bounding box; x,y of top-left, top-right, bottom-right, bottom-left
          std::array<double, 4> x{tl.GetX(), tl.GetX() + w, tl.GetX() + w, tl.GetX()};
          std::array<double, 4> y{tl.GetY(), tl.GetY(), tl.GetY() + h, tl.GetY() + h};

          // rotate
          for (int i=0; i<4; i++){
            double ox = x[i];
            double oy = y[i];
            x[i] = ox * cosA - oy * sinA;
            y[i] = ox * sinA + oy * cosA;
          }

          // bounding box after rotation
          double minX=x[0];
          double maxX=x[0];
          double minY=y[0];
          double maxY=y[0];
          for (int i=1; i<4; i++){
            minX = std::min(minX, x[i]);
            maxX = std::max(maxX, x[i]);
            minY = std::min(minY, y[i]);
            maxY = std::max(maxY, y[i]);
          }
          // setup glyph top-left position and dimension after rotation
          glyphCopy.trPosition.Set(minX+glyphCopy.position.GetX(), minY+glyphCopy.position.GetY());
          glyphCopy.trWidth  = maxX - minX;
          glyphCopy.trHeight = maxY - minY;

          cLabel.glyphs.push_back(glyphCopy);
        }
        if (!cLabel.glyphs.empty()) { // is some glyph visible?
          contourLabelInstances.push_back(cLabel);
        }
        offset+=label->width + labelData.contourLabelSpace;
      }
    }

    const std::vector<LabelInstanceType>& Labels() const
    {
      return labelInstances;
    }

    const std::vector<ContourLabelType>& ContourLabels() const
    {
      return contourLabelInstances;
    }

  private:
    TextLayouter *textLayouter;
    std::vector<ContourLabelType> contourLabelInstances;
    std::vector<LabelInstanceType> labelInstances;
    DoubleRectangle visibleViewport;
    DoubleRectangle layoutViewport;
    double layoutOverlap; // overlap ratio used for label layouting
  };

}

#endif
