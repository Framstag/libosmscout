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

namespace osmscout {

  struct IntRectangle {
    int x;
    int y;
    int width;
    int height;

    inline bool Intersects(const IntRectangle &other)
    {
      return !(
          (x + width) < other.x ||
          x > (other.x + other.width) ||
          (y + height) < other.y ||
          y > (other.y + other.height)
      );
    }
  };

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
    //double          proposedWidth{-1};

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
    osmscout::Vertex2D position;
    double angle{0}; //!< clock-wise rotation in radians

    // osmscout::Vertex2D tl{0,0};
    // osmscout::Vertex2D tr{0,0};
    // osmscout::Vertex2D br{0,0};
    // osmscout::Vertex2D bl{0,0};

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
    osmscout::LabelStyleRef style;    //!< Style for drawing
    std::string             text;     //!< The label text

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
      //std::cout << "create " << this << std::endl;
    };

    Mask(const Mask &m) :
        d(m.d), cellFrom(m.cellFrom), cellTo(m.cellTo), rowFrom(m.rowFrom), rowTo(m.rowTo)
    {
      //std::cout << "create(2) " << this << std::endl;
    };

    ~Mask()
    {
      //std::cout << "delete " << this << std::endl;
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

  /*
  template <class NativeGlyph>
  osmscout::Vertex2D GlyphTopLeft(const NativeGlyph &glyph);

  template <class NativeGlyph>
  double GlyphWidth(const NativeGlyph &glyph);

  template <class NativeGlyph>
  double GlyphHeight(const NativeGlyph &glyph);
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
        textLayouter(textLayouter), viewport{}
    {};

    void SetViewport(int x, int y, int w, int h)
    {
      viewport.x=x;
      viewport.y=y;
      viewport.width=w;
      viewport.height=h;
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

      // TODO: sort labels by priority and position (to be deterministic)

      // TODO: layout labels outside viewport

      // compute collisions, hide some labels
      int64_t rowSize = (viewport.width / 64)+1;
      //int64_t binaryWidth = rowSize * 8;
      //size_t binaryHeight = (viewportHeight / 8)+1;
      std::vector<uint64_t> iconCanvas((size_t)(rowSize*viewport.height));
      std::vector<uint64_t> labelCanvas((size_t)(rowSize*viewport.height));
      std::vector<uint64_t> overlayCanvas((size_t)(rowSize*viewport.height));
      //canvas.data()
      //Mask row(rowSize);

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

            IntRectangle rectangle{ (int)element.x-viewport.x, (int)element.y-viewport.y, 0, 0 };
            std::vector<uint64_t> *canvas = &labelCanvas;
            if (element.labelData.type==LabelData::Icon || element.labelData.type==LabelData::Symbol){
              rectangle.width = element.labelData.iconWidth;
              rectangle.height = element.labelData.iconHeight;
              canvas = &iconCanvas;
            } else {
              rectangle.width = element.label->width;
              rectangle.height = element.label->height;
              // Something is an overlay, if its alpha is <0.8
              if (element.labelData.alpha < 0.8){
                canvas = &overlayCanvas;
              }
            }
            row.prepare(rectangle);
            bool collision = CheckLabelCollision(*canvas, row, viewport.height);
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
                MarkLabelPlace(*(canvases[eli]), masks[eli], viewport.height);
              }
            }
          }

          labelIter++;
        }

        if (currentContourLabel != allSortedContourLabels.end()){
          int glyphCnt=currentContourLabel->glyphs.size();
          //std::vector<uint64_t> rowBuff((size_t)(rowSize * glyphCnt));
          Mask m(rowSize);
          std::vector<Mask> masks(glyphCnt, m);
          bool collision=false;
          for (int gi=0; !collision && gi<glyphCnt; gi++) {
            //uint64_t *row=rowBuff.data() + (gi*rowSize);

            auto glyph=currentContourLabel->glyphs[gi];
            IntRectangle rect{
                (int)glyph.trPosition.GetX()-viewport.x,
                (int)glyph.trPosition.GetY()-viewport.y,
                (int)glyph.trWidth,
                (int)glyph.trHeight
            };
            masks[gi].prepare(rect);
            collision |= CheckLabelCollision(labelCanvas, masks[gi], viewport.height);
          }
          if (!collision) {
            for (int gi=0; gi<glyphCnt; gi++) {
              MarkLabelPlace(labelCanvas, masks[gi], viewport.height);
            }
            contourLabelInstances.push_back(*currentContourLabel);
          }
          contourLabelIter++;
        }
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
        instance.priority = std::min(d.priority, instance.priority);
        if (d.type==LabelData::Type::Icon || d.type==LabelData::Type::Symbol){
          element.x = point.GetX() - d.iconWidth / 2;
          if (offset<0){
            element.y = point.GetY() - d.iconHeight / 2;
            offset = point.GetY() + d.iconHeight / 2;
          } else {
            element.y = offset;
            offset += d.iconHeight;
          }
        }else {
          // TODO: should we take style into account?
          element.label = textLayouter->Layout(projection, parameter,
                                               d.text, d.fontSize,
                                               objectWidth, /*enable wrapping*/ true);
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
          /*enable wrapping*/ false);

      // text should be rendered with 0x0 coordinate as left baseline
      // we want to move label little bit bottom, near to line center
      double textOffset=label->height * 0.25;
      //double textOffset=100.0;

      std::vector<Glyph<NativeGlyph>> glyphs = label->ToGlyphs();

      // TODO: do the magic to make sure that we don't render label upside-down
      double pLength=labelPath.GetLength();
      double offset=labelData.contourLabelOffset;
      while (offset+label->width < pLength){

        // skip string rendering when path is too much squiggly at this offset
        if (!labelPath.TestAngleVariance(offset,offset+label->width,M_PI/4)){
          // skip drawing current label and let offset point to the next instance
          offset+=label->width + labelData.contourLabelSpace;
          continue;
        }

        ContourLabelType cLabel;
        cLabel.priority = labelData.priority;
        cLabel.style = labelData.style;

        // direction of path at the label drawing starting point
        double initialAngle=std::abs(labelPath.AngleAtLengthDeg(offset));
        bool upwards=initialAngle>90 && initialAngle<270;

        for (const Glyph<NativeGlyph> &glyph:glyphs){
          double glyphOffset = upwards ?
                               offset - glyph.position.GetX() + label->width:
                               offset + glyph.position.GetX();
          osmscout::Vertex2D point=labelPath.PointAtLength(glyphOffset);

          double w = textLayouter->GlyphWidth(glyph.glyph);
          double h = textLayouter->GlyphHeight(glyph.glyph);

          // glyph angle in radians
          double angle=labelPath.AngleAtLength(upwards ? glyphOffset - w/2 : glyphOffset + w/2)*-1;

          // it is not real diagonal, but maximum distance from glyph
          // point that can be covered after treansformantions
          double diagonal=w+h+std::abs(textOffset);

          // fast check if current glyph can be visible
          if (!viewport.Intersects(IntRectangle{
            (int)(point.GetX()-diagonal),
            (int)(point.GetY()-diagonal),
            (int)(2*diagonal),
            (int)(2*diagonal)
          })){
            continue;
          }

          if (upwards) {
            angle-=M_PI;
          }
          double  sinA=std::sin(angle);
          double  cosA=std::cos(angle);

          Glyph<NativeGlyph> glyphCopy=glyph;
          glyphCopy.position=osmscout::Vertex2D(point.GetX() - textOffset * sinA,
                                                point.GetY() + textOffset * cosA);
          glyphCopy.angle=angle;

          auto tl = textLayouter->GlyphTopLeft(glyphCopy.glyph);

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
          // glyphCopy.tl.Set(x[0]+glyphCopy.position.GetX(), y[0]+glyphCopy.position.GetY());
          // glyphCopy.tr.Set(x[1]+glyphCopy.position.GetX(), y[1]+glyphCopy.position.GetY());
          // glyphCopy.br.Set(x[2]+glyphCopy.position.GetX(), y[2]+glyphCopy.position.GetY());
          // glyphCopy.bl.Set(x[3]+glyphCopy.position.GetX(), y[3]+glyphCopy.position.GetY());

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
    IntRectangle viewport;
  };

}

#endif
