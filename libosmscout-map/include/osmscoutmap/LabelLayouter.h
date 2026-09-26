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

#include <array>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include <osmscoutmap/MapImportExport.h>

#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/LabelPath.h>
#include <osmscoutmap/LabelLayouterHelper.h>

#include <osmscout/system/Math.h>

#include <iostream>

namespace osmscout {

#ifdef OSMSCOUT_DEBUG_LABEL_LAYOUTER
constexpr bool debugLabelLayouter = true;
#else
constexpr bool debugLabelLayouter = false;
#endif

  class PathLabelData
  {
  public:
    size_t            priority{0}; //!< Priority of the entry (from stylesheet)
    std::string       text;        //!< The label text (type==Text|PathText)
    double            height;
    PathTextStyleRef  style;
    double            contourLabelOffset;
    double            contourLabelSpace;
  };

  class LabelData
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
    size_t            priority{0}; //!< Priority of the entry (from stylesheet)
    size_t            position{0}; //!< Relative position of the label

    double            alpha{1.0};   //!< Alpha value of the label; 0.0 = fully transparent, 1.0 = solid
    double            fontSize{0};  //!< Font size to be used

    LabelStyleRef     style;    //!< Style for drawing
    std::string       text;     //!< The label text (type==Text|PathText)

    IconStyleRef      iconStyle; //!< Icon or symbol style
    double            iconWidth{0};
    double            iconHeight{0};

  public:
    LabelData() = default;
    ~LabelData() = default;
  };

  class OSMSCOUT_MAP_API ContourLabelPositioner CLASS_FINAL
  {
  public:
    struct Position
    {
      size_t labelCount; //!< Number of labels rendered
      double offset;     //!< Offset of the first label
      double labelSpace; //!< Space between individual labels
    };
  public:
    Position calculatePositions(const Projection& projection,
                                const MapParameter& parameter,
                                const PathLabelData &labelData,
                                double pathLength,
                                double labelWidth) const;
  };

  template<class NativeGlyph>
  class Glyph {
  public:
    NativeGlyph glyph;
    Vertex2D    position;        //!< glyph baseline position
    double      angle{0};        //!< clock-wise rotation in radians

    Vertex2D    trPosition{0,0}; //!< top-left position after rotation
    double      trWidth{0};      //!< width after rotation
    double      trHeight{0};     //!< height after rotation
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

    Label() = default;

    template<typename... Args>
    explicit Label(Args&&... args):
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

  struct LabelPriority
  {
    /** LabelPriority is used to determine the order of labels and to decide which label to show when there is a collision.
     *
     * - `priority` is defined by the stylesheet
     * - `basemap` flag is here to give precedence to labels from ordinary map databases before world overview (basemap)
     * - `ref` is used for providing stable output for two labels with the same priority (and basemap flag)
     */
    size_t                priority{std::numeric_limits<size_t>::max()}; //!< Priority from the stylesheet
    bool                  basemap{false}; //!< true when label is for object from basemap database
    ObjectFileRef         ref;

    LabelPriority() = default;

    LabelPriority(size_t priority, bool basemap, const ObjectFileRef& ref):
      priority(priority), basemap(basemap), ref(ref)
    {}

    bool operator<(const LabelPriority& other) const
    {
      return std::tie(priority, basemap, ref) < std::tie(other.priority, other.basemap, other.ref);
    }

    bool operator<=(const LabelPriority& other) const
    {
      return std::tie(priority, basemap, ref) <= std::tie(other.priority, other.basemap, other.ref);
    }

    bool operator!=(const LabelPriority& other) const
    {
      return std::tie(priority, basemap, ref) != std::tie(other.priority, other.basemap, other.ref);
    }

    friend std::ostream& operator<<(std::ostream& stream, const LabelPriority &prio);
  };

  inline std::ostream& operator<<(std::ostream& stream, const LabelPriority &prio)
  {
    stream << "LabelPriority(" << prio.priority << ", " << prio.basemap << ", " << prio.ref.GetName() << ")";
    return stream;
  }

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
    LabelPriority priority; //!< Priority of the entry (minimum of priority label elements)

    std::vector<Element>  elements;
  };

  template<class NativeGlyph>
  class ContourLabel
  {
  public:
#ifdef OSMSCOUT_DEBUG_LABEL_LAYOUTER
    std::string                     text;     //!< The original label (if debug)
    double                          offset;   //!< The offset of the label in relation the way start (if debug)
    Vertex2D                        start;    //!< Screen coordinates of the start of the path
#endif
    LabelPriority                   priority; //!< Priority of the label
    std::vector<Glyph<NativeGlyph>> glyphs;   //!< Vector of glyphs of the label text (see text)
    PathTextStyleRef                style;    //!< Style for drawing the text of the label
  };

  template <class NativeGlyph, class NativeLabel>
  static bool LabelInstanceSorter(const LabelInstance<NativeGlyph, NativeLabel> &a,
                                  const LabelInstance<NativeGlyph, NativeLabel> &b)
  {
    return a.priority < b.priority;
  }

  template <class NativeGlyph>
  static bool ContourLabelSorter(const ContourLabel<NativeGlyph> &a,
                                 const ContourLabel<NativeGlyph> &b)
  {

    if (a.priority != b.priority) {
      return a.priority < b.priority;
    }

    return a.glyphs[0].trPosition.GetX() < b.glyphs[0].trPosition.GetX();
  }

  /**
   * Key of a label measurement: the arguments a backend's Layout() call receives and the
   * parameters that backend's Layout() reads on top of them. Two labels with the same key
   * measure the same, so the second one can reuse the first measurement.
   *
   * The parameters of the line wrapping (`LabelLineMinCharCount`, `LabelLineMaxCharCount`,
   * `LabelLineFitToArea`, `LabelLineFitToWidth`) are no argument of Layout(), but the backends
   * that wrap a label ask MapPainter::GetProposedLabelWidth for the width they wrap with, and
   * that call reads them. A frame that sees them changed therefore measures again.
   */
  class LabelMeasurementKey
  {
  public:
    std::string text;
    double      fontSize{0.0};
    double      objectWidth{0.0};
    bool        enableWrapping{false};
    bool        contourLabel{false};
    size_t      labelLineMinCharCount{0};
    size_t      labelLineMaxCharCount{0};
    bool        labelLineFitToArea{false};
    double      labelLineFitToWidth{0.0};

    bool operator==(const LabelMeasurementKey& other) const
    {
      return fontSize==other.fontSize &&
             objectWidth==other.objectWidth &&
             enableWrapping==other.enableWrapping &&
             contourLabel==other.contourLabel &&
             labelLineMinCharCount==other.labelLineMinCharCount &&
             labelLineMaxCharCount==other.labelLineMaxCharCount &&
             labelLineFitToArea==other.labelLineFitToArea &&
             labelLineFitToWidth==other.labelLineFitToWidth &&
             text==other.text;
    }
  };

  struct LabelMeasurementKeyHash
  {
    size_t operator()(const LabelMeasurementKey& key) const
    {
      size_t hash=std::hash<std::string>()(key.text);

      hash=hash*31u+std::hash<double>()(key.fontSize);
      hash=hash*31u+std::hash<double>()(key.objectWidth);
      hash=hash*31u+std::hash<bool>()(key.enableWrapping);
      hash=hash*31u+std::hash<bool>()(key.contourLabel);
      hash=hash*31u+std::hash<size_t>()(key.labelLineMinCharCount);
      hash=hash*31u+std::hash<size_t>()(key.labelLineMaxCharCount);
      hash=hash*31u+std::hash<bool>()(key.labelLineFitToArea);
      hash=hash*31u+std::hash<double>()(key.labelLineFitToWidth);

      return hash;
    }
  };

  /**
   *
   * @tparam NativeGlyph
   * @tparam NativeLabel
   * @tparam TextLayouter - class providing low level text layouting
   *   required methods:
   *
   *    // glyph bounding box relative to its base point
   *    DoubleScreenRectangle GlyphBoundingBox(const NativeGlyph &) const
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
  class LabelLayouter
  {

  public:
    using ContourLabelType = ContourLabel<NativeGlyph>;
    using LabelType = Label<NativeGlyph, NativeLabel>;
    using LabelPtr = std::shared_ptr<LabelType>;
    using LabelInstanceType = LabelInstance<NativeGlyph, NativeLabel>;

  public:
    /**
     * Number of label measurements a layouter remembers by default. The bound exists so that a
     * long pan session does not accumulate the measurements of every label ever seen; it is
     * far above the label count of a typical view.
     */
    static constexpr size_t defaultMeasurementCount=4096;

    explicit LabelLayouter(TextLayouter* textLayouter,
                           size_t maxMeasurementCount=defaultMeasurementCount)
    : textLayouter(textLayouter),
      maxMeasurementCount(maxMeasurementCount)
    {}

    void SetViewport(const ScreenVectorRectangle& v)
    {
      visibleViewport = v;
      layoutViewportValid=true;
      SetLayoutOverlap(layoutOverlap);
    }

    void SetLayoutOverlap(uint32_t overlap)
    {
      layoutOverlap = overlap;
      layoutViewport.width = visibleViewport.width + (overlap * 2);
      layoutViewport.height = visibleViewport.height + (overlap * 2);
      layoutViewport.x = visibleViewport.x - overlap;
      layoutViewport.y = visibleViewport.y - overlap;
    }

    /**
     * Set the measurement environment the measurements of the following frame are made in:
     * the state a backend's measurements depend on that is not an argument of Layout() - the
     * resolved font, the resolution of the drawing target, its font settings. A backend calls
     * this once per frame, before it registers labels. While the environment is unchanged the
     * measurements of earlier frames stay valid; when it changes they are dropped, because the
     * backend would measure them differently now.
     *
     * A backend that never sets an environment has an empty one, which means that its
     * measurements depend on the arguments of Layout() alone.
     */
    void SetMeasurementEnvironment(const std::string& environment)
    {
      if (environment==measurementEnvironment) {
        return;
      }

      measurements.clear();
      measurementOrder.clear();
      measurementEnvironment=environment;
    }

    void Reset()
    {
      contourLabelInstances.clear();
      labelInstances.clear();

      // The viewport belongs to the frame that has just been drawn: the labels of the next frame
      // are registered before the drawing target of that frame reports its viewport, so they
      // cannot be decided against a viewport until it has been set again
      layoutViewportValid=false;

      // A layouter without a measurement cache holds the measurement of the frame that has just
      // been drawn; the frame is over, so it is dropped with the rest of the frame's state
      notRememberedMeasurement=LabelMeasurement();
    }

    // Something is an overlay, if its alpha is <0.8
    static bool IsOverlay(const LabelData &labelData)
    {
      return labelData.alpha < 0.8;
    }

    /**
     * Upper bound [pixels] of the distance the elements of a label can reach from the anchor
     * position of that label: the measured rectangle of an element stays inside the anchor plus
     * this distance, and the overlap canvases of the layout mark an element with the widest
     * padding of the frame around that rectangle. A label that is farther away from the layout
     * viewport than this bound can therefore be dropped before it is measured, without changing
     * which labels the drawing path draws and which labels the layout suppresses.
     *
     * The bound is summed over the elements, because the elements of a label are stacked at its
     * anchor, and it uses the conservative label extent bound of the label helper for text
     * elements.
     */
    double LabelReach(const Projection& projection,
                      const MapParameter& parameter,
                      const std::vector<LabelData>& data) const
    {
      double fontSizePixel=projection.ConvertWidthToPixel(parameter.GetFontSize());
      double reach=GetMaxLabelPaddingPixel(projection,
                                           parameter);

      for (const auto& d : data) {
        if (d.type==LabelData::Type::Text) {
          reach+=2.0*GetLabelExtentBound(d.text.size(),
                                         CountLabelWords(d.text),
                                         d.fontSize*fontSizePixel);
        }
        else {
          reach+=std::max(d.iconWidth,d.iconHeight);
        }
      }

      return reach;
    }

    /**
     * Returns true when no element of the label can intersect the layout viewport and when the
     * label cannot suppress a label inside it, i.e. when the label provably has no effect on the
     * frame and does not have to be measured, stored or laid out.
     */
    bool CannotReachViewport(const Projection& projection,
                             const MapParameter& parameter,
                             const Vertex2D& point,
                             const std::vector<LabelData>& data) const
    {
      // The frame's viewport is reported by the drawing target of the backend while the frame is
      // drawn, i.e. after the labels of the first steps of the frame were registered. A label
      // cannot be decided against the viewport of the previous frame, so it is kept until the
      // viewport of the current frame is known.
      if (!layoutViewportValid) {
        return false;
      }

      double reach=LabelReach(projection,
                              parameter,
                              data);

      ScreenVectorRectangle element(point.GetX()-reach,
                                    point.GetY()-reach,
                                    2.0*reach,
                                    2.0*reach);

      return !element.Intersects(layoutViewport);
    }

    bool CannotReachViewport(const Projection& projection,
                             const MapParameter& parameter,
                             const Vertex2D& point,
                             const LabelData& data) const
    {
      return CannotReachViewport(projection,
                                 parameter,
                                 point,
                                 std::vector<LabelData>{data});
    }

    /**
     * Debug check of the conservative bound the early decision uses: the rectangle of every
     * element a label built has to stay inside the anchor plus the reach of the label. If an
     * element could leave that box, the early decision could drop a label that the drawing path
     * of the label stage would draw, which would change the rendered output.
     */
    void AssertElementsInsideReach(const Projection& projection,
                                   const MapParameter& parameter,
                                   const Vertex2D& point,
                                   const std::vector<LabelData>& data,
                                   const LabelInstanceType& instance) const
    {
#ifdef NDEBUG
      // The check only exists in builds with assertions
      (void)projection;
      (void)parameter;
      (void)point;
      (void)data;
      (void)instance;
#else
      double reach=LabelReach(projection,
                              parameter,
                              data);

      for (const auto& element : instance.elements) {
        double width=element.labelData.type==LabelData::Type::Text ?
                       element.label->width :
                       element.labelData.iconWidth;
        double height=element.labelData.type==LabelData::Type::Text ?
                        element.label->height :
                        element.labelData.iconHeight;

        assert(element.x>=point.GetX()-reach);
        assert(element.x+width<=point.GetX()+reach);
        assert(element.y>=point.GetY()-reach);
        assert(element.y+height<=point.GetY()+reach);
      }
#endif
    }

    /**
     * Layout job initializes separate canvases for icons/symbols, labels and overlay labels.
     * Then takes all registered labels and contour labels and sort them by priority.
     * Note that labels includes standard labels, icons/symbols and overlay labels.
     * As final step process labels and contour labels (from highest priority) and check
     * its visual rectangle in corresponding canvas. When pixels are not occupied yet,
     * it is added and pixels on canvas mark.
     */
    struct LayoutJob {
      ScreenVectorRectangle layoutViewport;

      double iconPadding=0.0;
      double labelPadding=0.0;
      double shieldLabelPadding=0.0;
      double contourLabelPadding=0.0;
      double overlayLabelPadding=0.0;

      std::vector<ContourLabelType> allSortedContourLabels;
      std::vector<LabelInstanceType> allSortedLabels;

      ScreenMask                     iconCanvas;
      ScreenMask                     labelCanvas;
      ScreenMask                     overlayCanvas;

      std::vector<ScreenRectMask>    instanceMasks; //!< Reused masks of the label instance in flight
      std::vector<ScreenMask*>       instanceCanvases; //!< Reused canvas of each element of the instance
      std::vector<ScreenRectMask>    contourMasks; //!< Reused masks of the path label in flight

      LayoutJob() = default;

      LayoutJob(const LayoutJob&) = delete;
      LayoutJob(LayoutJob&&) = delete;
      ~LayoutJob() = default;
      LayoutJob& operator=(const LayoutJob&) = delete;
      LayoutJob& operator=(LayoutJob&&) = delete;

      /**
       * Prepare the job for a frame: adopt the layout viewport, recompute the paddings and reset
       * the canvases in place, so that a repeated frame of the same size does not allocate the
       * canvases again.
       */
      void Reset(const ScreenVectorRectangle &newLayoutViewport,
                 const Projection& projection,
                 const MapParameter& parameter)
      {
        layoutViewport=newLayoutViewport;

        iconPadding=projection.ConvertWidthToPixel(parameter.GetIconPadding());
        labelPadding=projection.ConvertWidthToPixel(parameter.GetLabelPadding());
        shieldLabelPadding=projection.ConvertWidthToPixel(parameter.GetPlateLabelPadding());
        contourLabelPadding=projection.ConvertWidthToPixel(parameter.GetContourLabelPadding());
        overlayLabelPadding=projection.ConvertWidthToPixel(parameter.GetOverlayLabelPadding());

        iconCanvas.Reset(layoutViewport.width,layoutViewport.height);
        labelCanvas.Reset(layoutViewport.width,layoutViewport.height);
        overlayCanvas.Reset(layoutViewport.width,layoutViewport.height);
      }

      /**
       * Take the registered labels of the frame and prepare the output stores of the frame: the
       * registered labels move into the job, the output stores are cleared but keep their
       * capacity, because the resolution is expected to add the resolved labels to them.
       */
      void PrepareFrame(std::vector<LabelInstanceType> &newLabelInstances,
                        std::vector<ContourLabelType> &newContourLabelInstances)
      {
        std::swap(allSortedLabels, newLabelInstances);
        std::swap(allSortedContourLabels, newContourLabelInstances);

        newLabelInstances.clear();
        newContourLabelInstances.clear();
      }

      void SortLabels()
      {
        // sort labels by priority and position (to be deterministic)
        std::stable_sort(allSortedLabels.begin(),
                         allSortedLabels.end(),
                         LabelInstanceSorter<NativeGlyph, NativeLabel>);
        std::stable_sort(allSortedContourLabels.begin(),
                         allSortedContourLabels.end(),
                         ContourLabelSorter<NativeGlyph>);
      }

      double GetLabelPadding(const LabelData &labelData) const
      {
        if (labelData.type==LabelData::Icon || labelData.type==LabelData::Symbol) {
          return iconPadding;
        }

        if (IsOverlay(labelData)) {
          return overlayLabelPadding;
        }

        if (dynamic_cast<const ShieldStyle*>(labelData.style.get())!=nullptr){
          return shieldLabelPadding;
        }

        return labelPadding;
      }

      ScreenMask* GetCanvas(LabelData data) {
        if (data.type==LabelData::Icon || data.type==LabelData::Symbol){
          return &iconCanvas;
        }

        if (IsOverlay(data)) {
          return &overlayCanvas;
        }

        return &labelCanvas;
      }

      void ProcessLabelInstance(const LabelInstanceType &currentLabel,
                                std::vector<LabelInstanceType> &labelInstances)

      {
        size_t elementCount = currentLabel.elements.size();       // Number of elements in label

        // Reused scratch storage: the mask of every element of the instance and the canvas each
        // of them collides with. A mask reuses its bitmask, so a repeated frame does not
        // allocate it again.
        instanceMasks.resize(elementCount);
        instanceCanvases.assign(elementCount, nullptr);

        // The resolved instance is built in place in the output store: the visible elements are
        // appended to it and the instance is dropped again when none of them is visible
        labelInstances.emplace_back();

        LabelInstanceType &instance=labelInstances.back();

        instance.priority=currentLabel.priority;

        for (size_t eli=0; eli < elementCount; eli++) {
          const typename LabelInstance<NativeGlyph, NativeLabel>::Element &element = currentLabel.elements[eli];
          ScreenRectMask                                                  &mask=instanceMasks[eli];
          ScreenMask                                                      *canvas=GetCanvas(element.labelData);
          double                                                          padding=GetLabelPadding(element.labelData);

          ScreenPixelRectangle rectangle{(int)(element.x - layoutViewport.x - padding),
                                         (int)(element.y - layoutViewport.y - padding),
                                         0, 0 };

          if (element.labelData.type==LabelData::Icon || element.labelData.type==LabelData::Symbol){
            if (element.labelData.iconStyle->IsOverlay()) {
              rectangle.width = 0;
              rectangle.height = 0;
            }
            else {
              rectangle.width = element.labelData.iconWidth + 2*padding;
              rectangle.height = element.labelData.iconHeight + 2*padding;
            }

            if constexpr (debugLabelLayouter) {
              if (element.labelData.type == LabelData::Icon) {
                std::cout << "Test icon " << element.labelData.iconStyle->GetIconName() <<
                          " prio " << currentLabel.priority;
              } else {
                std::cout << "Test symbol " << element.labelData.iconStyle->GetSymbol()->GetName() <<
                          " prio " << currentLabel.priority;
              }
            }
          }
          else {
            if constexpr (debugLabelLayouter) {
              std::cout << "Test " << (IsOverlay(element.labelData) ? "overlay " : "")
                        << "label prio " << currentLabel.priority << ": "
                        << element.labelData.text;
            }

            rectangle.width = element.label->width + 2*padding;
            rectangle.height = element.label->height + 2*padding;
          }

          mask.Reset(layoutViewport.width,
                     rectangle);

          bool collision = canvas->HasCollision(mask);

          if (!collision) {
            instance.elements.push_back(element);
            instanceCanvases[eli]=canvas;
          }

          if constexpr (debugLabelLayouter) {
            std::cout << " -> " << (collision ? "skipped" : "added") << std::endl;
            // p->DrawRectangle(rectangle.x, rectangle.y,
            //                  rectangle.width, rectangle.height,
            //                  collision ? Color(0.8, 0, 0, 0.8): Color(0, 0.8, 0, 0.8));
          }
        }

        if (instance.elements.empty()) {
          labelInstances.pop_back();

          return;
        }

        // mark all labels at once (elements of single label may have no padding)

        for (size_t eli=0; eli < elementCount; eli++) {
          if (instanceCanvases[eli] != nullptr) {
            instanceCanvases[eli]->AddMask(instanceMasks[eli]);
          }
        }
      }

      void ProcessLabelContourLabel(const ContourLabelType &currentContourLabel,
                                    std::vector<ContourLabelType> &contourLabelInstances)
      {
        int glyphCnt=currentContourLabel.glyphs.size();

        if constexpr (debugLabelLayouter) {
          std::cout << "Test contour label prio " << currentContourLabel.priority << ": " << currentContourLabel.text;
        }

        contourMasks.resize(glyphCnt);

        bool collision=false;
        for (int gi=0; gi<glyphCnt; gi++) {
          auto glyph=currentContourLabel.glyphs[gi];
          ScreenPixelRectangle rect{
            (int)(glyph.trPosition.GetX() - layoutViewport.x - contourLabelPadding),
            (int)(glyph.trPosition.GetY() - layoutViewport.y - contourLabelPadding),
            (int)(glyph.trWidth + 2*contourLabelPadding),
            (int)(glyph.trHeight + 2*contourLabelPadding)
          };

          contourMasks[gi].Reset(layoutViewport.width,
                                 rect);

          if (labelCanvas.HasCollision(contourMasks[gi])) {
            collision=true;
            break;
          }
        }

        if (!collision) {
          for (int gi=0; gi<glyphCnt; gi++) {
            labelCanvas.AddMask(contourMasks[gi]);
          }

          contourLabelInstances.push_back(currentContourLabel);
        }

        if constexpr (debugLabelLayouter) {
          std::cout << " -> " << (collision ? "skipped" : "added") << std::endl;
        }
      };

      void ProcessLabels(std::vector<LabelInstanceType> &labelInstances,
                         std::vector<ContourLabelType> &contourLabelInstances)
      {
        labelInstances.reserve(allSortedLabels.size());
        contourLabelInstances.reserve(allSortedContourLabels.size());

        // Get first entries

        auto labelIter = allSortedLabels.begin();
        auto contourLabelIter = allSortedContourLabels.begin();

        // While both lists are not completely processed...
        //   Process first all contour labels of a priority and then all normal labels of the same priority

        while (labelIter != allSortedLabels.end() &&
           contourLabelIter != allSortedContourLabels.end()) {

          if (contourLabelIter->priority<=labelIter->priority) {
            ProcessLabelContourLabel(*contourLabelIter, contourLabelInstances);
            contourLabelIter++;
          }
          else {
            ProcessLabelInstance(*labelIter, labelInstances);
            labelIter++;
          }

        }

        // Process all the rest... (there should only be one of the two lists left)

        while (contourLabelIter != allSortedContourLabels.end()) {
          ProcessLabelContourLabel(*contourLabelIter, contourLabelInstances);
          contourLabelIter++;
        }

        while (labelIter != allSortedLabels.end()) {
          ProcessLabelInstance(*labelIter, labelInstances);
          labelIter++;
        }
      }
    };

    void Layout(const Projection& projection,
                const MapParameter& parameter)
    {
      // compute collisions, hide some labels
      layoutJob.Reset(layoutViewport,
                      projection,
                      parameter);
      layoutJob.PrepareFrame(labelInstances, contourLabelInstances);
      layoutJob.SortLabels();
      layoutJob.ProcessLabels(labelInstances, contourLabelInstances);
    }

    template<class Painter>
    void DrawTextLabels(const Projection& /*projection*/,
                        const MapParameter& /*parameter*/,
                        Painter */*p*/) const
    {

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
     *                    double centerX, double centerY,
     *                    double width, double height) override;
     *
     *      void DrawLabel(const Projection& projection,
     *                     const MapParameter& parameter,
     *                     const DoubleScreenRectangle& labelRectangle,
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
      std::vector<const typename LabelInstanceType::Element*> overlayElements;

      for (const LabelInstanceType &inst : Labels()){
        for (const typename LabelInstanceType::Element &el : inst.elements) {
          ScreenVectorRectangle elementRectangle;
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
                          Vertex2D(el.x + el.labelData.iconWidth/2,
                                   el.y + el.labelData.iconHeight/2),
                          1.0);

          } else if (el.labelData.type==LabelData::Icon){
            p->DrawIcon(el.labelData.iconStyle.get(),
                        Vertex2D(el.x + el.labelData.iconWidth/2,
                                 el.y + el.labelData.iconHeight/2),
                        el.labelData.iconWidth,
                        el.labelData.iconHeight);

          } else {
            // postpone text elements
            if (IsOverlay(el.labelData)){
              overlayElements.push_back(&el);
            }else {
              textElements.push_back(&el);
            }
          }
        }
      }

      // draw postponed text elements
      for (const typename LabelInstanceType::Element *el : textElements) {
        p->DrawLabel(projection, parameter,
                     ScreenVectorRectangle(el->x, el->y, el->label->width, el->label->height),
                     el->labelData, el->label->label);
      }

      for (const typename LabelInstanceType::Element *el : overlayElements) {
        p->DrawLabel(projection, parameter,
                     ScreenVectorRectangle(el->x, el->y, el->label->width, el->label->height),
                     el->labelData, el->label->label);
      }

      for (const ContourLabelType& label : ContourLabels()){
        p->DrawGlyphs(projection,
                      parameter,
                      label.style,
                      label.glyphs);
      }
    }

    // should be made private
    void ProcessLabel(const Projection& projection,
                      const MapParameter& parameter,
                      const Vertex2D& point,
                      LabelInstanceType& instance,
                      double& offset,
                      const LabelData& data,
                      double objectWidth)
    {
      typename LabelInstance<NativeGlyph, NativeLabel>::Element element;

      element.labelData=data;

      if (data.type==LabelData::Type::Icon || data.type==LabelData::Type::Symbol){
        instance.priority = std::min(
          LabelPriority(data.priority, instance.priority.basemap, instance.priority.ref),
          instance.priority);
        element.x = point.GetX() - data.iconWidth / 2;
        if (std::isnan(offset)){
          element.y = point.GetY() - data.iconHeight / 2;
          offset = point.GetY() + data.iconHeight / 2;
        }
        else {
          element.y = offset;
          offset += data.iconHeight;
        }
      }
      else {
        instance.priority = std::min(
          LabelPriority(data.priority, instance.priority.basemap, instance.priority.ref),
          instance.priority);
        // TODO: should we take style into account?
        // Qt allows to split text layout and style setup
        LabelMeasurement & measurement=MeasureLabel(projection,
                                                    parameter,
                                                    data.text,
                                                    data.fontSize,
                                                    objectWidth,
                                                    /*enable wrapping*/ true,
                                                    /*contour label*/ false);

        element.label = measurement.label;
        element.x = point.GetX() - element.label->width / 2;
        if (std::isnan(offset)){
          element.y = point.GetY() - element.label->height / 2;
          offset = point.GetY() + element.label->height / 2;
        }
        else {
          element.y = offset;
          offset += element.label->height;
        }
      }

      instance.elements.push_back(element);
    }

    void RegisterLabel(const Projection& projection,
                       const MapParameter& parameter,
                       bool basemap,
                       const ObjectFileRef& ref,
                       const Vertex2D& point,
                       const LabelData& data,
                       double objectWidth = 10.0)
    {
      if (CannotReachViewport(projection,
                              parameter,
                              point,
                              data)) {
        return;
      }

      LabelInstanceType instance;

      // A label instance carries the object reference and the basemap flag as part of its
      // priority, which is also how the element list overload below registers a label
      instance.priority=LabelPriority(std::numeric_limits<size_t>::max(), basemap, ref);

      // NaN until the first element is placed. The offset then becomes a screen coordinate, which
      // is negative for a point above the drawn area: with -1 as the marker, the elements after
      // the first one were centred on the point again instead of being stacked below it.
      double offset=std::numeric_limits<double>::quiet_NaN();
      ProcessLabel(projection,
                   parameter,
                   point,
                   instance,
                   offset,
                   data,
                   objectWidth);

      AssertElementsInsideReach(projection,
                                parameter,
                                point,
                                std::vector<LabelData>{data},
                                instance);

      labelInstances.push_back(instance);
    }

    void RegisterLabel(const Projection& projection,
                       const MapParameter& parameter,
                       bool basemap,
                       const ObjectFileRef& ref,
                       const Vertex2D& point,
                       const std::vector<LabelData>& data,
                       double objectWidth = 10.0)
    {
      if (CannotReachViewport(projection,
                              parameter,
                              point,
                              data)) {
        return;
      }

      LabelInstanceType instance;

      instance.priority=LabelPriority(std::numeric_limits<size_t>::max(), basemap, ref);

      // NaN until the first element is placed. The offset then becomes a screen coordinate, which
      // is negative for a point above the drawn area: with -1 as the marker, the elements after
      // the first one were centred on the point again instead of being stacked below it.
      double offset=std::numeric_limits<double>::quiet_NaN();
      for (const auto& d : data) {
        ProcessLabel(projection,
                     parameter,
                     point,
                     instance,
                     offset,
                     d,
                     objectWidth);
      }

      AssertElementsInsideReach(projection,
                                parameter,
                                point,
                                data,
                                instance);

      labelInstances.push_back(instance);
    }

    void RegisterContourLabel(const Projection& projection,
                              const MapParameter& parameter,
                              bool basemap,
                              const ObjectFileRef& ref,
                              const PathLabelData &labelData,
                              const LabelPath &labelPath)
    {
      LabelMeasurement & measurement=MeasureLabel(projection,
                                                  parameter,
                                                  labelData.text,
                                                  labelData.height,
                                                  /* object width */ 0.0,
                                                  /*enable wrapping*/ false,
                                                  /*contour label*/ true);

      LabelPtr label=measurement.label;

      // text should be rendered with 0x0 coordinate as left baseline
      // we want to move label a bit to the bottom, near to line center
      double                           textBaselineOffset = label->height * 0.25;

      const std::vector<Glyph<NativeGlyph>> &glyphs = GetLabelGlyphs(measurement);
      double                           pathLength=labelPath.GetLength();
      ContourLabelPositioner           positioner;
      ContourLabelPositioner::Position position=positioner.calculatePositions(projection,
                                                                              parameter,
                                                                              labelData,
                                                                              pathLength,
                                                                              label->width);

      double offset=position.offset;
      size_t currentCount=1;
      while (currentCount<=position.labelCount){
        double nextOffset=offset+label->width+position.labelSpace;

        currentCount++;

        // skip string rendering when path is too much squiggly at this offset
        if (!labelPath.TestAngleVariance(offset,offset+label->width,M_PI_4)){
          // skip drawing current label and let offset point to the next instance
          offset=nextOffset;
          continue;
        }

        ContourLabelType cLabel;

        cLabel.priority = LabelPriority(labelData.priority, basemap, ref);
        cLabel.style = labelData.style;

        if constexpr (debugLabelLayouter) {
          cLabel.text = labelData.text;
          cLabel.offset = offset;
          cLabel.start = labelPath.PointAtLength(0);
        }

        // do the magic to make sure that we don't render label upside-down

        // direction of path at the label drawing starting point
        double initialAngle=std::abs(labelPath.AngleAtLengthDeg(offset));
        bool upwards=initialAngle>90 && initialAngle<270;


        for (const Glyph<NativeGlyph> &glyph:glyphs){
          double glyphOffset = upwards ?
                               offset - glyph.position.GetX() + label->width:
                               offset + glyph.position.GetX();
          osmscout::Vertex2D point=labelPath.PointAtLength(glyphOffset);

          ScreenVectorRectangle textBoundingBox = textLayouter->GlyphBoundingBox(glyph.glyph);
          double w = textBoundingBox.width;
          double h = textBoundingBox.height;
          osmscout::Vertex2D tl(textBoundingBox.x, textBoundingBox.y);

          // glyph angle in radians
          double angle=labelPath.AngleAtLength(upwards ? glyphOffset - w/2 : glyphOffset + w/2)*-1;

          // it is not real diagonal, but maximum distance from glyph
          // point that can be covered after transformations
          double diagonal=w+h+std::abs(textBaselineOffset);

          // fast check if current glyph can be visible
          if (!layoutViewport.Intersects(ScreenVectorRectangle{
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
          glyphCopy.trPosition=Vertex2D(minX+glyphCopy.position.GetX(),
                                        minY+glyphCopy.position.GetY());
          glyphCopy.trWidth  = maxX - minX;
          glyphCopy.trHeight = maxY - minY;

          cLabel.glyphs.push_back(glyphCopy);
        }
        if (!cLabel.glyphs.empty()) { // is some glyph visible?
          contourLabelInstances.push_back(cLabel);
        }

        offset=nextOffset;
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

    /**
     * Return the number of label measurements that are remembered at the moment.
     */
    size_t GetMeasurementCount() const
    {
      return measurements.size();
    }

    /**
     * Return the number of label measurements that are remembered at most.
     */
    size_t GetMaxMeasurementCount() const
    {
      return maxMeasurementCount;
    }

    /**
     * Return the number of remembered measurements whose glyph data has been derived.
     */
    size_t GetGlyphCount() const
    {
      size_t count=0;

      for (const auto& entry : measurements) {
        if (entry.second.glyphsDerived) {
          count++;
        }
      }

      return count;
    }

  private:
    /**
     * A remembered label measurement: the layouted label and the glyph data derived from it.
     *
     * The glyph data is owned here, together with the label it was derived from. A label whose
     * measurement is dropped therefore drops its glyph data as well, and no glyph data can
     * outlive the label it was derived from.
     */
    struct LabelMeasurement
    {
      LabelPtr                                 label;
      std::vector<Glyph<NativeGlyph>>          glyphs;
      bool                                     glyphsDerived{false};
      std::list<LabelMeasurementKey>::iterator order; //!< Position of the measurement in measurementOrder
    };

    using MeasurementOrder = std::list<LabelMeasurementKey>;

    using MeasurementIndex = std::unordered_map<LabelMeasurementKey,LabelMeasurement,LabelMeasurementKeyHash>;

    TextLayouter     *textLayouter;
    size_t           maxMeasurementCount;

    std::string      measurementEnvironment;
    MeasurementIndex measurements;
    MeasurementOrder measurementOrder;
    LayoutJob        layoutJob;                                                         //!< Reused state of the frame's overlap resolution

    /**
     * Measurement of a layouter that does not remember measurements (see maxMeasurementCount);
     * it holds the measurement of the label that was measured last.
     */
    LabelMeasurement notRememberedMeasurement;

    /**
     * Measure a label, reusing the measurement of an earlier frame while the measurement
     * inputs and the measurement environment are unchanged.
     *
     * The returned reference stays valid until the measurement is dropped, which happens when
     * the environment changes or when the measurement is the oldest one of a full cache. It is
     * therefore only valid for the label stage's current step, not for the whole frame.
     */
    LabelMeasurement& MeasureLabel(const Projection& projection,
                                   const MapParameter& parameter,
                                   const std::string& text,
                                   double fontSize,
                                   double objectWidth,
                                   bool enableWrapping,
                                   bool contourLabel)
    {
      LabelMeasurementKey key;

      key.text=text;
      key.fontSize=fontSize;
      key.objectWidth=objectWidth;
      key.enableWrapping=enableWrapping;
      key.contourLabel=contourLabel;
      key.labelLineMinCharCount=parameter.GetLabelLineMinCharCount();
      key.labelLineMaxCharCount=parameter.GetLabelLineMaxCharCount();
      key.labelLineFitToArea=parameter.GetLabelLineFitToArea();
      key.labelLineFitToWidth=parameter.GetLabelLineFitToWidth();

      // A bound of 0 disables the reuse of measurements
      if (maxMeasurementCount>0) {
        auto entry=measurements.find(key);

        if (entry!=measurements.end()) {
          // The measurement has just been used, so it is the most recent one and the measurement
          // at the front of the order is the one that has not been used for the longest time
          measurementOrder.splice(measurementOrder.end(),
                                  measurementOrder,
                                  entry->second.order);

          return entry->second;
        }
      }

      LabelMeasurement measurement;

      measurement.label=textLayouter->Layout(projection,
                                             parameter,
                                             text,
                                             fontSize,
                                             objectWidth,
                                             enableWrapping,
                                             contourLabel);

      if (maxMeasurementCount==0) {
        // The measurement is used by the caller and dropped afterwards, so it is not remembered
        // and no glyph data is keyed on its label
        notRememberedMeasurement=std::move(measurement);

        return notRememberedMeasurement;
      }

      while (measurements.size()>=maxMeasurementCount &&
             !measurementOrder.empty()) {
        auto oldest=measurements.find(measurementOrder.front());

        if (oldest!=measurements.end()) {
          measurements.erase(oldest);
        }

        measurementOrder.pop_front();
      }

      measurementOrder.push_back(key);

      auto inserted=measurements.emplace(std::move(key),
                                         std::move(measurement));

      inserted.first->second.order=std::prev(measurementOrder.end());

      return inserted.first->second;
    }

    /**
     * Return the glyph data of a measured label, deriving it once per remembered measurement.
     */
    const std::vector<Glyph<NativeGlyph>>& GetLabelGlyphs(LabelMeasurement& measurement)
    {
      if (!measurement.glyphsDerived) {
        measurement.glyphs=measurement.label->ToGlyphs();
        measurement.glyphsDerived=true;
      }

      return measurement.glyphs;
    }

  private:
    std::vector<ContourLabelType> contourLabelInstances;
    std::vector<LabelInstanceType> labelInstances;
    ScreenVectorRectangle visibleViewport{0,0,0,0};
    ScreenVectorRectangle layoutViewport{0,0,0,0};
    bool                 layoutViewportValid=false; //!< true when the layout viewport belongs to the frame being prepared
    uint32_t layoutOverlap=0; // overlap [pixels] used for label layouting
  };

}

#endif
