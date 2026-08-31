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
#include <array>

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
  class LabelInstanceSorter CLASS_FINAL
  {
  public:
    explicit LabelInstanceSorter(const std::set<ObjectFileRef> &visibleRefs)
      : visibleRefs(visibleRefs)
    {
    }

    bool operator()(const LabelInstance<NativeGlyph, NativeLabel> &a,
                    const LabelInstance<NativeGlyph, NativeLabel> &b) const
    {
      // Labels that were already visible in the previous layout round claim
      // their space first. This makes the visibility of labels stable against
      // growth of the candidate set (e.g. when the render bounding box of the
      // previous asynchronous rendering differs from the current one). Within
      // the two groups the ordering is the (priority,basemap,ref) tuple.
      const bool aWasVisible=visibleRefs.count(a.priority.ref)>0;
      const bool bWasVisible=visibleRefs.count(b.priority.ref)>0;

      if (aWasVisible!=bWasVisible) {
        return aWasVisible;
      }

      return a.priority < b.priority;
    }

  private:
    const std::set<ObjectFileRef> &visibleRefs;
  };

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
    explicit LabelLayouter(TextLayouter *textLayouter):
        textLayouter(textLayouter)
    {};
    /**
     * Temporary diagnostics for label stability investigation: enabled by
     * the environment variable OSMSCOUT_DEBUG_LABEL_HYSTERESIS. For
     * every layout round that loses a label that was visible in the
     * previous round, the reason is reported: not registered anymore
     * (candidate/data churn) or hidden by collision despite previously
     * visible (resolution churn).
     */
    static bool IsHysteresisDebug()
    {
      static const bool enabled=std::getenv("OSMSCOUT_DEBUG_LABEL_HYSTERESIS")!=nullptr;

      return enabled;
    }

    void SetViewport(const ScreenVectorRectangle& v)
    {
      visibleViewport = v;
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
     * Clears all registered labels and layout results of the current
     * draw. The previous-round visibility state is intentionally kept:
     * it provides stable label visibility when the candidate set
     * changes between draw calls (rendering of shifted bounding boxes,
     * asynchronous tile loading). The state is refreshed by every
     * Layout() call.
     */
    void Reset()
    {
      contourLabelInstances.clear();
      labelInstances.clear();
    }

    // Something is an overlay, if its alpha is <0.8
    static bool IsOverlay(const LabelData &labelData)
    {
      return labelData.alpha < 0.8;
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

      double iconPadding;
      double labelPadding;
      double shieldLabelPadding;
      double contourLabelPadding;
      double overlayLabelPadding;

      std::vector<ContourLabelType> allSortedContourLabels;
      std::vector<LabelInstanceType> allSortedLabels;

      ScreenMask iconCanvas;
      ScreenMask labelCanvas;
      ScreenMask overlayCanvas;

      const std::set<ObjectFileRef> &visibleRefs;
      const std::set<ObjectFileRef> &visibleContourRefs;

      LayoutJob(const ScreenVectorRectangle &layoutViewport,
                const Projection& projection,
                const MapParameter& parameter,
                const std::set<ObjectFileRef> &visibleRefs,
                const std::set<ObjectFileRef> &visibleContourRefs):
              layoutViewport(layoutViewport),
              visibleRefs(visibleRefs),
              visibleContourRefs(visibleContourRefs),
              iconPadding(projection.ConvertWidthToPixel(parameter.GetIconPadding())),
              labelPadding(projection.ConvertWidthToPixel(parameter.GetLabelPadding())),
              shieldLabelPadding(projection.ConvertWidthToPixel(parameter.GetPlateLabelPadding())),
              contourLabelPadding(projection.ConvertWidthToPixel(parameter.GetContourLabelPadding())),
              overlayLabelPadding(projection.ConvertWidthToPixel(parameter.GetOverlayLabelPadding())),
              iconCanvas(layoutViewport.width,layoutViewport.height),
              labelCanvas(layoutViewport.width,layoutViewport.height),
              overlayCanvas(layoutViewport.width,layoutViewport.height)
      {
      }

      LayoutJob(const LayoutJob&) = delete;
      LayoutJob(LayoutJob&&) = delete;
      ~LayoutJob() = default;
      LayoutJob& operator=(const LayoutJob&) = delete;
      LayoutJob& operator=(LayoutJob&&) = delete;

      void Swap(std::vector<LabelInstanceType> &labelInstances,
                std::vector<ContourLabelType> &contourLabelInstances)
      {
        std::swap(allSortedLabels, labelInstances);
        std::swap(allSortedContourLabels, contourLabelInstances);
      }

      void SortLabels()
      {
        // sort labels by priority and position (to be deterministic)
        std::stable_sort(allSortedLabels.begin(),
                         allSortedLabels.end(),
                         LabelInstanceSorter<NativeGlyph, NativeLabel>(visibleRefs));
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

      static constexpr int hysteresisTolerancePx=2;

      void ProcessLabelInstance(const LabelInstanceType &currentLabel,
                                std::vector<LabelInstanceType> &labelInstances,
                                bool hysteresisTolerance)

      {
        size_t elementCount = currentLabel.elements.size();       // Number of elements in label
        std::vector<ScreenRectMask> masks(elementCount);          // Vector of masks of each individual object
        std::vector<ScreenMask*> canvases(elementCount, nullptr); // Corresponding canvas for each label or null (if collision)

        // List of elements to be rendered (no collision)
        std::vector<typename LabelInstance<NativeGlyph, NativeLabel>::Element> visibleElements;

        for (size_t eli=0; eli < elementCount; eli++) {
          const typename LabelInstance<NativeGlyph, NativeLabel>::Element& element = currentLabel.elements[eli];
          ScreenRectMask &mask=masks[eli];
          ScreenMask     *canvas=GetCanvas(element.labelData);
          double         padding=GetLabelPadding(element.labelData);

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

          mask=ScreenRectMask(layoutViewport.width,
                              rectangle);

          bool collision = canvas->HasCollision(mask);

          if (collision && hysteresisTolerance && hysteresisTolerancePx>0) {
            // Previously visible labels must not flip their visibility due to
            // the +-1px jitter of the rasterized mask rectangles (integer
            // truncation of fractional positions) while the map slides.
            // Ignore collisions that disappear when the rectangle is shrunk
            // by a small tolerance. Real overlaps survive the shrink and are
            // resolved by priority as usual.
            ScreenPixelRectangle shrunkRectangle{rectangle.x+hysteresisTolerancePx,
                                                 rectangle.y+hysteresisTolerancePx,
                                                 rectangle.width-2*hysteresisTolerancePx,
                                                 rectangle.height-2*hysteresisTolerancePx};

            if (shrunkRectangle.width>0 && shrunkRectangle.height>0) {
              ScreenRectMask shrunkMask(layoutViewport.width,
                                        shrunkRectangle);

              collision=canvas->HasCollision(shrunkMask);
            }
          }

          if (!collision) {
            visibleElements.push_back(element);
            canvases[eli]=canvas;
          }

          if constexpr (debugLabelLayouter) {
            std::cout << " -> " << (collision ? "skipped" : "added") << std::endl;
            // p->DrawRectangle(rectangle.x, rectangle.y,
            //                  rectangle.width, rectangle.height,
            //                  collision ? Color(0.8, 0, 0, 0.8): Color(0, 0.8, 0, 0.8));
          }
        }

        if (!visibleElements.empty()) {
          LabelInstanceType instanceCopy{currentLabel.priority, visibleElements};
          labelInstances.push_back(instanceCopy);

          // mark all labels at once (elements of single label may have no padding)

          for (size_t eli=0; eli < elementCount; eli++) {
            if (canvases[eli] != nullptr) {
              canvases[eli]->AddMask(masks[eli]);
            }
          }
        }
      }

      void ProcessLabelContourLabel(const ContourLabelType &currentContourLabel,
                                    std::vector<ContourLabelType> &contourLabelInstances,
                                    bool hysteresisTolerance)
      {
        int glyphCnt=currentContourLabel.glyphs.size();

        if constexpr (debugLabelLayouter) {
          std::cout << "Test contour label prio " << currentContourLabel.priority << ": " << currentContourLabel.text;
        }

        std::vector<ScreenRectMask> masks(glyphCnt);

        bool collision=false;
        for (int gi=0; gi<glyphCnt; gi++) {
          auto glyph=currentContourLabel.glyphs[gi];
          ScreenPixelRectangle rect{
            (int)(glyph.trPosition.GetX() - layoutViewport.x - contourLabelPadding),
            (int)(glyph.trPosition.GetY() - layoutViewport.y - contourLabelPadding),
            (int)(glyph.trWidth + 2*contourLabelPadding),
            (int)(glyph.trHeight + 2*contourLabelPadding)
          };

          masks[gi]=ScreenRectMask(layoutViewport.width,
                                   rect);

          if (labelCanvas.HasCollision(masks[gi])) {
            if (hysteresisTolerance && hysteresisTolerancePx>0) {
              // single-pixel rasterization jitter of previously visible
              // path labels must not toggle their visibility
              ScreenPixelRectangle shrunkRectangle{rect.x+hysteresisTolerancePx,
                                                   rect.y+hysteresisTolerancePx,
                                                   rect.width-2*hysteresisTolerancePx,
                                                   rect.height-2*hysteresisTolerancePx};

              if (shrunkRectangle.width>0 && shrunkRectangle.height>0) {
                ScreenRectMask shrunkMask(layoutViewport.width,
                                          shrunkRectangle);

                if (!labelCanvas.HasCollision(shrunkMask)) {
                  continue; // jitter only, tolerate
                }
              }
            }

            collision=true;
            break;
          }
        }

        if (!collision) {
          for (int gi=0; gi<glyphCnt; gi++) {
            labelCanvas.AddMask(masks[gi]);
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

        // Phase 0: labels that were visible in the previous round claim
        // their space first, with hysteresis tolerance. They must not be
        // displaced by contour label claims or by new candidates in order
        // to keep the visible set stable while the map slides.
        while (labelIter != allSortedLabels.end() &&
               visibleRefs.count(labelIter->priority.ref)>0) {
          ProcessLabelInstance(*labelIter,
                               labelInstances,
                               /*hysteresisTolerance*/ true);
          labelIter++;
        }

        // Phase 1: contour labels of ways that were visible in the previous
        // round (with hysteresis tolerance): street names must not flip
        // because of the rasterization jitter of their glyphs.
        while (contourLabelIter != allSortedContourLabels.end() &&
               visibleContourRefs.count(contourLabelIter->priority.ref)>0) {
          ProcessLabelContourLabel(*contourLabelIter,
                                   contourLabelInstances,
                                   /*hysteresisTolerance*/ true);
          contourLabelIter++;
        }

        // Phase 2: the remaining contour labels and the remaining regular
        // labels (new candidates), merged by priority...

        while (labelIter != allSortedLabels.end() &&
           contourLabelIter != allSortedContourLabels.end()) {

          if (contourLabelIter->priority<=labelIter->priority) {
            ProcessLabelContourLabel(*contourLabelIter, contourLabelInstances, false);
            contourLabelIter++;
          }
          else {
            ProcessLabelInstance(*labelIter, labelInstances, false);
            labelIter++;
          }

        }

        // Process all the rest... (there should only be one of the two lists left)

        while (contourLabelIter != allSortedContourLabels.end()) {
          ProcessLabelContourLabel(*contourLabelIter, contourLabelInstances, false);
          contourLabelIter++;
        }

        while (labelIter != allSortedLabels.end()) {
          ProcessLabelInstance(*labelIter, labelInstances, false);
          labelIter++;
        }
      }
    };

    void Layout(const Projection& projection,
                const MapParameter& parameter)
    {
      // compute collisions, hide some labels
      LayoutJob job(layoutViewport, projection, parameter, lastVisibleRefs, lastVisibleContourRefs);
      job.Swap(labelInstances, contourLabelInstances);
      job.SortLabels();
      job.ProcessLabels(labelInstances, contourLabelInstances);

      // refresh the previous-round visibility state: it defines which
      // labels claim their space first in the next round. This must run
      // for every layout, independent of the diagnostics.
      std::set<ObjectFileRef> registeredRefs;
      const size_t allSortedContourLabelCount=job.allSortedContourLabels.size();

      for (const LabelInstanceType &instance : job.allSortedLabels) {
        registeredRefs.insert(instance.priority.ref);
      }

      for (const LabelInstanceType &instance : labelInstances) {
        registeredRefs.insert(instance.priority.ref);
      }

      if (IsHysteresisDebug()) {
        std::vector<ObjectFileRef> hiddenByCollision;
        std::vector<ObjectFileRef> notRegistered;
        size_t keptVisible=0;

        for (const ObjectFileRef &ref : lastVisibleRefs) {
          if (registeredRefs.count(ref)>0) {
            bool isVisible=std::any_of(labelInstances.begin(),labelInstances.end(),[ref](const LabelInstanceType &instance) {
              return instance.priority.ref==ref;
            });

            if (isVisible) {
              keptVisible++;
            }
            else {
              hiddenByCollision.push_back(ref);
            }
          }
          else {
            notRegistered.push_back(ref);
          }
        }

        std::set<ObjectFileRef> placedContourRefs;

        for (const ContourLabelType &cLabel : contourLabelInstances) {
          placedContourRefs.insert(cLabel.priority.ref);
        }

        size_t lostContours=0;

        for (const ObjectFileRef &ref : lastVisibleContourRefs) {
          if (placedContourRefs.count(ref)==0) {
            lostContours++;
          }
        }

        if (!hiddenByCollision.empty() || !notRegistered.empty() ||
            lostContours>0) {
          std::cout << "[LabelHysteresis] viewport=" << layoutViewport.width << "x"
                    << layoutViewport.height << "+" << layoutViewport.x << "+"
                    << layoutViewport.y << " registered=" << registeredRefs.size()
                    << " visible=" << labelInstances.size()
                    << " prevVisible=" << lastVisibleRefs.size()
                    << " kept=" << keptVisible
                    << " hiddenByCollision=" << hiddenByCollision.size()
                    << " notRegistered=" << notRegistered.size()
                    << " contoursRegistered=" << allSortedContourLabelCount
                    << " prevVisibleContours=" << lastVisibleContourRefs.size()
                    << " lostContours=" << lostContours
                    << std::endl;

          for (const ObjectFileRef &ref : hiddenByCollision) {
            std::cout << "[LabelHysteresis]   hidden-by-collision: "
                      << ref.GetName();
            std::cout << std::endl;
          }

          for (const ObjectFileRef &ref : notRegistered) {
            std::cout << "[LabelHysteresis]   not-registered (candidate/data churn): "
                      << ref.GetName() << std::endl;
          }
        }
      }

      lastVisibleRefs.clear();

      for (const LabelInstanceType &instance : labelInstances) {
        lastVisibleRefs.insert(instance.priority.ref);
      }

      lastVisibleContourRefs.clear();

      for (const ContourLabelType &cLabel : contourLabelInstances) {
        lastVisibleContourRefs.insert(cLabel.priority.ref);
      }
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
        if (offset<0){
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
        element.label = textLayouter->Layout(projection, parameter,
                                             data.text, data.fontSize,
                                             objectWidth,
                                             /*enable wrapping*/ true,
                                             /*contour label*/ false);
        element.x = point.GetX() - element.label->width / 2;
        if (offset<0){
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
      LabelInstanceType instance;

      instance.ref=ref;
      instance.basemap=basemap;

      double offset=-1;
      ProcessLabel(projection,
                   parameter,
                   point,
                   instance,
                   offset,
                   data,
                   objectWidth);

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
      LabelInstanceType instance;

      instance.priority=LabelPriority(std::numeric_limits<size_t>::max(), basemap, ref);

      double offset=-1;
      for (const auto& d : data) {
        ProcessLabel(projection,
                     parameter,
                     point,
                     instance,
                     offset,
                     d,
                     objectWidth);
      }

      labelInstances.push_back(instance);
    }

    void RegisterContourLabel(const Projection& projection,
                              const MapParameter& parameter,
                              bool basemap,
                              const ObjectFileRef& ref,
                              const PathLabelData &labelData,
                              const LabelPath &labelPath)
    {
      // TODO: cache label for string and font parameters
      LabelPtr label=textLayouter->Layout(projection,
                                          parameter,
                                          labelData.text,
                                          labelData.height,
                                          /* object width */ 0.0,
                                          /*enable wrapping*/ false,
                                          /*contour label*/ true);

      // text should be rendered with 0x0 coordinate as left baseline
      // we want to move label a bit to the bottom, near to line center
      double                           textBaselineOffset = label->height * 0.25;

      std::vector<Glyph<NativeGlyph>>  glyphs = label->ToGlyphs();
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

  private:
    TextLayouter *textLayouter;
    std::vector<ContourLabelType> contourLabelInstances;
    std::vector<LabelInstanceType> labelInstances;
    std::set<ObjectFileRef> lastVisibleRefs; //!< refs of the labels visible in the previous round
    std::set<ObjectFileRef> lastVisibleContourRefs; //!< refs of the ways with visible path labels in the previous round
    ScreenVectorRectangle visibleViewport{0,0,0,0};
    ScreenVectorRectangle layoutViewport{0,0,0,0};
    uint32_t layoutOverlap=0; // overlap [pixels] used for label layouting
  };

}

#endif
