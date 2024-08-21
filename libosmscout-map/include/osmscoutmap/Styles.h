#ifndef OSMSCOUT_STYLES_H
#define OSMSCOUT_STYLES_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <list>
#include <set>
#include <vector>

#include <osmscoutmap/MapImportExport.h>

#include <osmscout/Pixel.h>

#include <osmscout/projection/Projection.h>

#include <osmscout/util/Color.h>
#include <osmscout/util/LaneTurn.h>
#include <osmscout/util/ScreenBox.h>

#include <osmscoutmap/StyleDescription.h>

namespace osmscout {

  /**
   * Offset for rendered line, relative to way.
   */
  enum class OffsetRel: int {
    base,                       //!< way center
    leftOutline,                //!< left side of the way
    rightOutline,               //!< right side of the way
    laneDivider,                //!< when way has multiple lanes, line is rendered as its divider

    /** when way has explicit turns, following offsets may be used to decorate them specially */
    laneForwardLeft,
    laneForwardThroughLeft,
    laneForwardThrough,
    laneForwardThroughRight,
    laneForwardRight,
    laneBackwardLeft,
    laneBackwardThroughLeft,
    laneBackwardThrough,
    laneBackwardThroughRight,
    laneBackwardRight,

    sidecar                     //!< special offset for routes, line are stacked next to way, same colors are "collapsed"
  };

  extern bool IsLaneOffset(OffsetRel rel);

  extern OffsetRel ParseForwardTurnStringToOffset(LaneTurn turn);
  extern OffsetRel ParseBackwardTurnStringToOffset(LaneTurn turn);

  /**
   * \ingroup Stylesheet
   *
   * Style options for a line.
   */
  class OSMSCOUT_MAP_API LineStyle CLASS_FINAL : public Style
  {
  public:
    enum CapStyle {
      capButt,
      capRound,
      capSquare
    };

    enum Attribute {
      attrLineColor,
      attrGapColor,
      attrPreferColorFeature,
      attrDisplayWidth,
      attrWidth,
      attrDisplayOffset,
      attrOffset,
      attrJoinCap,
      attrEndCap,
      attrDashes,
      attrPriority,
      attrZIndex,
      attrOffsetRel
    };

  private:
    std::string         slot;
    Color               lineColor;
    Color               gapColor;
    bool                preferColorFeature;
    double              displayWidth;
    double              width;
    double              displayOffset;
    double              offset;
    CapStyle            joinCap;
    CapStyle            endCap;
    std::vector<double> dash;
    int                 priority;
    int                 zIndex;
    OffsetRel           offsetRel;

  public:
    LineStyle();

    void SetColorValue(int attribute, const Color& value) override;
    void SetDoubleValue(int attribute, double value) override;
    void SetDoubleArrayValue(int attribute, const std::vector<double>& value) override;
    void SetIntValue(int attribute, int value) override;
    void SetBoolValue(int attribute, bool value) override;

    LineStyle& SetSlot(const std::string& slot);

    LineStyle& SetLineColor(const Color& color);
    LineStyle& SetGapColor(const Color& color);
    LineStyle& SetPreferColorFeature(bool value);
    LineStyle& SetDisplayWidth(double value);
    LineStyle& SetWidth(double value);
    LineStyle& SetDisplayOffset(double value);
    LineStyle& SetOffset(double value);
    LineStyle& SetJoinCap(CapStyle joinCap);
    LineStyle& SetEndCap(CapStyle endCap);
    LineStyle& SetDashes(const std::vector<double>& dashes);
    LineStyle& SetPriority(int priority);
    LineStyle& SetZIndex(int zIndex);
    LineStyle& SetOffsetRel(OffsetRel offsetRel);

    bool IsVisible() const
    {
      return (displayWidth>0.0 ||
              width>0.0) &&
              lineColor.IsVisible();
    }

    const std::string& GetSlot() const
    {
      return slot;
    }

    const Color& GetLineColor() const
    {
      return lineColor;
    }

    const Color& GetGapColor() const
    {
      return gapColor;
    }

    bool GetPreferColorFeature() const
    {
      return preferColorFeature;
    }

    double GetDisplayWidth() const
    {
      return displayWidth;
    }

    double GetWidth() const
    {
      return width;
    }

    double GetDisplayOffset() const
    {
      return displayOffset;
    }

    double GetOffset() const
    {
      return offset;
    }

    CapStyle GetJoinCap() const
    {
      return joinCap;
    }

    CapStyle GetEndCap() const
    {
      return endCap;
    }

    bool HasDashes() const
    {
      return !dash.empty();
    }

    const std::vector<double>& GetDash() const
    {
      return dash;
    }

    int GetPriority() const
    {
      return priority;
    }

    int GetZIndex() const
    {
      return zIndex;
    }

    OffsetRel GetOffsetRel() const
    {
      return offsetRel;
    }

    static StyleDescriptorRef GetDescriptor();

    void CopyAttributes(const LineStyle& other,
                        const std::set<Attribute>& attributes);

    bool operator==(const LineStyle& other) const;
    bool operator!=(const LineStyle& other) const;
    bool operator<(const LineStyle& other) const;
  };

  class OSMSCOUT_MAP_API CapStyleEnumAttributeDescriptor CLASS_FINAL : public StyleEnumAttributeDescriptor
  {
  public:
    CapStyleEnumAttributeDescriptor(const std::string& name,
                                    int attribute)
      : StyleEnumAttributeDescriptor(name,
                                     attribute)
    {
      AddEnumValue("butt",LineStyle::capButt);
      AddEnumValue("round",LineStyle::capRound);
      AddEnumValue("square",LineStyle::capSquare);
    }
  };

  class OSMSCOUT_MAP_API OffsetRelAttributeDescriptor CLASS_FINAL : public StyleEnumAttributeDescriptor
  {
  public:
    OffsetRelAttributeDescriptor(const std::string& name,
                                 int attribute)
      : StyleEnumAttributeDescriptor(name,
                                     attribute)
    {
      AddEnumValue2("base",OffsetRel::base);
      AddEnumValue2("leftOutline",OffsetRel::leftOutline);
      AddEnumValue2("rightOutline",OffsetRel::rightOutline);
      AddEnumValue2("laneDivider",OffsetRel::laneDivider);

      AddEnumValue2("laneForwardLeft",OffsetRel::laneForwardLeft);
      AddEnumValue2("laneForwardThroughLeft",OffsetRel::laneForwardThroughLeft);
      AddEnumValue2("laneForwardThrough",OffsetRel::laneForwardThrough);
      AddEnumValue2("laneForwardThroughRight",OffsetRel::laneForwardThroughRight);
      AddEnumValue2("laneForwardRight",OffsetRel::laneForwardRight);
      AddEnumValue2("laneBackwardLeft",OffsetRel::laneBackwardLeft);
      AddEnumValue2("laneBackwardThroughLeft",OffsetRel::laneBackwardThroughLeft);
      AddEnumValue2("laneBackwardThrough",OffsetRel::laneBackwardThrough);
      AddEnumValue2("laneBackwardThroughRight",OffsetRel::laneBackwardThroughRight);
      AddEnumValue2("laneBackwardRight",OffsetRel::laneBackwardRight);

      AddEnumValue2("sidecar", OffsetRel::sidecar);
    }

    void AddEnumValue2(const std::string& name,
                       OffsetRel value)
    {
      AddEnumValue(name, static_cast<int>(value));
    }
  };

  using LineStyleRef = std::shared_ptr<LineStyle>;

  /**
   * \ingroup Stylesheet
   *
   * Style options for filling an area.
   */
  class OSMSCOUT_MAP_API FillStyle CLASS_FINAL : public Style
  {
  public:
    enum Attribute {
      attrFillColor,
      attrPattern,
      attrPatternMinMag
    };

  private:
    Color               fillColor;
    std::string         pattern;
    mutable size_t      patternId;
    Magnification       patternMinMag;

  public:
    FillStyle();

    void SetStringValue(int attribute, const std::string& value) override;
    void SetColorValue(int attribute, const Color& value) override;
    void SetMagnificationValue(int attribute, const Magnification& value) override;

    FillStyle& SetFillColor(const Color& color);
    FillStyle& SetPattern(const std::string& pattern);
    void SetPatternId(size_t id) const;
    FillStyle& SetPatternMinMag(const Magnification& mag);

    bool IsVisible() const
    {
      return (fillColor.IsVisible() ||
              !pattern.empty());
    }

    const Color& GetFillColor() const
    {
      return fillColor;
    }

    bool HasPattern() const
    {
      return !pattern.empty();
    }

    std::string GetPatternName() const
    {
      return pattern;
    }

    size_t GetPatternId() const
    {
      return patternId;
    }

    const Magnification& GetPatternMinMag() const
    {
      return patternMinMag;
    }

    static StyleDescriptorRef GetDescriptor();

    void CopyAttributes(const FillStyle& other,
                        const std::set<Attribute>& attributes);

    bool operator==(const FillStyle& other) const;
    bool operator!=(const FillStyle& other) const;
    bool operator<(const FillStyle& other) const;
  };

  using FillStyleRef = std::shared_ptr<FillStyle>;

  /**
   * \ingroup Stylesheet
   *
   * Style options for borders around an area.
   */
  class OSMSCOUT_MAP_API BorderStyle CLASS_FINAL : public Style
  {
  public:
    enum Attribute {
      attrColor,
      attrGapColor,
      attrWidth,
      attrDashes,
      attrDisplayOffset,
      attrOffset,
      attrPriority
    };

  private:
    std::string         slot;
    Color               color;
    Color               gapColor;
    double              width;
    std::vector<double> dash;
    double              displayOffset;
    double              offset;
    int                 priority;

  public:
    BorderStyle();

    void SetColorValue(int attribute, const Color& value) override;
    void SetDoubleValue(int attribute, double value) override;
    void SetDoubleArrayValue(int attribute, const std::vector<double>& value) override;
    void SetIntValue(int attribute, int value) override;

    BorderStyle& SetSlot(const std::string& slot);

    BorderStyle& SetColor(const Color& color);
    BorderStyle& SetGapColor(const Color& color);
    BorderStyle& SetWidth(double value);
    BorderStyle& SetDashes(const std::vector<double>& dashes);
    BorderStyle& SetDisplayOffset(double value);
    BorderStyle& SetOffset(double value);
    BorderStyle& SetPriority(int priority);

    bool IsVisible() const
    {
      return width>0 && color.IsVisible();
    }

    const std::string& GetSlot() const
    {
      return slot;
    }

    const Color& GetColor() const
    {
      return color;
    }

    const Color& GetGapColor() const
    {
      return gapColor;
    }

    double GetWidth() const
    {
      return width;
    }

    bool HasDashes() const
    {
      return !dash.empty();
    }

    const std::vector<double>& GetDash() const
    {
      return dash;
    }

    double GetDisplayOffset() const
    {
      return displayOffset;
    }

    double GetOffset() const
    {
      return offset;
    }

    int GetPriority() const
    {
      return priority;
    }

    static StyleDescriptorRef GetDescriptor();

    void CopyAttributes(const BorderStyle& other,
                        const std::set<Attribute>& attributes);

    bool operator==(const BorderStyle& other) const;
    bool operator!=(const BorderStyle& other) const;
    bool operator<(const BorderStyle& other) const;
  };

  using BorderStyleRef = std::shared_ptr<BorderStyle>;

  /**
   * \ingroup Stylesheet
   *
   * Abstract base class for all (point) labels. All point labels have priority
   * and a alpha value.
   */
  class OSMSCOUT_MAP_API LabelStyle : public Style
  {
  private:
    size_t priority;
    double size;

  public:
    LabelStyle();

    virtual bool IsVisible() const = 0;
    virtual double GetAlpha() const = 0;

    LabelStyle& SetPriority(size_t priority);

    virtual LabelStyle& SetSize(double size);

    size_t GetPriority() const
    {
      return priority;
    }

    double GetSize() const
    {
      return size;
    }
  };

  using LabelStyleRef = std::shared_ptr<LabelStyle>;

  /**
   * \ingroup Stylesheet
   *
   * A textual label.
   */
  class OSMSCOUT_MAP_API TextStyle CLASS_FINAL : public LabelStyle
  {
  public:
    enum Style {
      normal,
      emphasize
    };

    enum Attribute {
      attrPriority,
      attrSize,
      attrLabel,
      attrPosition,
      attrTextColor,
      attrEmphasizeColor,
      attrStyle,
      attrScaleAndFadeMag,
      attrAutoSize
    };

  private:
    std::string      slot;
    LabelProviderRef label;           //!< The label - a reference to a feature and its label index
    size_t           position;        //!< Relative vertical position of the label
    Color            textColor;       //!< Color of text
    Color            emphasizeColor;  //!< Color of text emphasize
    Style            style;           //!< Style of the text
    Magnification    scaleAndFadeMag; //!< Automatic pseudo-autoSize scaling for nodes
    bool             autoSize;        //!< Calculate the size of the label base don the height of the area

  public:
    TextStyle();

    void SetBoolValue(int attribute, bool value) override;
    void SetColorValue(int attribute, const Color& value) override;
    void SetMagnificationValue(int attribute, const Magnification& value) override;
    void SetDoubleValue(int attribute, double value) override;
    void SetIntValue(int attribute, int value) override;
    void SetUIntValue(int attribute, size_t value) override;
    void SetLabelValue(int attribute, const LabelProviderRef& value) override;

    TextStyle& SetSlot(const std::string& slot);

    TextStyle& SetSize(double size) override;
    TextStyle& SetLabel(const LabelProviderRef& label);
    TextStyle& SetPosition(size_t position);
    TextStyle& SetTextColor(const Color& color);
    TextStyle& SetEmphasizeColor(const Color& color);
    TextStyle& SetStyle(Style style);
    TextStyle& SetScaleAndFadeMag(const Magnification& mag);
    TextStyle& SetAutoSize(bool autoSize);

    bool IsVisible() const override
    {
      return label &&
             GetSize()>0.0 &&
             GetTextColor().IsVisible();
    }

    double GetAlpha() const override
    {
      return textColor.GetA();
    }

    const std::string& GetSlot() const
    {
      return slot;
    }

    const LabelProviderRef& GetLabel() const
    {
      return label;
    }

    size_t GetPosition() const
    {
      return position;
    }

    const Color& GetTextColor() const
    {
      return textColor;
    }

    const Color& GetEmphasizeColor() const
    {
      return emphasizeColor;
    }

    const Style& GetStyle() const
    {
      return style;
    }

    Magnification GetScaleAndFadeMag() const
    {
      return scaleAndFadeMag;
    }

    bool GetAutoSize() const
    {
      return autoSize;
    }

    static StyleDescriptorRef GetDescriptor();

    void CopyAttributes(const TextStyle& other,
                        const std::set<Attribute>& attributes);

    bool operator==(const TextStyle& other) const;
    bool operator!=(const TextStyle& other) const;
    bool operator<(const TextStyle& other) const;
  };

  class OSMSCOUT_MAP_API TextStyleEnumAttributeDescriptor CLASS_FINAL : public StyleEnumAttributeDescriptor
  {
  public:
    TextStyleEnumAttributeDescriptor(const std::string& name,
                                     int attribute)
      : StyleEnumAttributeDescriptor(name,
                                     attribute)
    {
      AddEnumValue("normal",TextStyle::normal);
      AddEnumValue("emphasize",TextStyle::emphasize);
    }
  };

  using TextStyleRef = std::shared_ptr<TextStyle>;

  /**
   * \ingroup Stylesheet
   *
   * A shield or plate label (text placed on a plate).
   */
  class OSMSCOUT_MAP_API ShieldStyle : public LabelStyle
  {
  public:
    enum Attribute {
      attrPriority,
      attrSize,
      attrLabel,
      attrTextColor,
      attrBgColor,
      attrBorderColor
    };

  private:
    LabelProviderRef label;          //!< The label - a reference to a feature and its label index
    Color            textColor;      //!< Color of the text
    Color            bgColor;        //!< Background of the text
    Color            borderColor;    //!< Color of the border

  public:
    ShieldStyle();

    ShieldStyle& SetLabel(const LabelProviderRef& label);
    ShieldStyle& SetSize(double size) override;
    ShieldStyle& SetTextColor(const Color& color);
    ShieldStyle& SetBgColor(const Color& color);
    ShieldStyle& SetBorderColor(const Color& color);

    bool IsVisible() const override
    {
      return label &&
              GetSize()>0.0 &&
              GetTextColor().IsVisible();
    }

    double GetAlpha() const override
    {
      return textColor.GetA();
    }

    const LabelProviderRef& GetLabel() const
    {
      return label;
    }

    const Color& GetTextColor() const
    {
      return textColor;
    }

    const Color& GetBgColor() const
    {
      return bgColor;
    }

    const Color& GetBorderColor() const
    {
      return borderColor;
    }

    void CopyAttributes(const ShieldStyle& other,
                        const std::set<Attribute>& attributes);
  };

  using ShieldStyleRef = std::shared_ptr<ShieldStyle>;

  /**
   * \ingroup Stylesheet
   *
   * A style defining repetive drawing of a shield label along a path. It consists
   * mainly of the attributes of the shield itself (it internally holds a shield
   * label for this) and some more attributes defining the way of repetition.
   */
  class OSMSCOUT_MAP_API PathShieldStyle : public Style
  {
  public:
    enum Attribute {
      attrPriority,
      attrSize,
      attrLabel,
      attrTextColor,
      attrBgColor,
      attrBorderColor,
      attrShieldSpace
    };

  private:
    ShieldStyleRef shieldStyle;
    double         shieldSpace;

  public:
    PathShieldStyle();
    // Explicit copy constructor because of shieldStyle attribute
    PathShieldStyle(const PathShieldStyle& style);
    PathShieldStyle(const PathShieldStyle&& style);

    PathShieldStyle& operator=(const PathShieldStyle& style) = delete;
    PathShieldStyle& operator=(const PathShieldStyle&& style) = delete;

    void SetLabelValue(int attribute, const LabelProviderRef& value) override;
    void SetColorValue(int attribute, const Color& value) override;
    void SetDoubleValue(int attribute, double value) override;
    void SetUIntValue(int attribute, size_t value) override;

    PathShieldStyle& SetLabel(const LabelProviderRef& label);
    PathShieldStyle& SetPriority(size_t priority);
    PathShieldStyle& SetSize(double size);
    PathShieldStyle& SetTextColor(const Color& color);
    PathShieldStyle& SetBgColor(const Color& color);
    PathShieldStyle& SetBorderColor(const Color& color);
    PathShieldStyle& SetShieldSpace(double shieldSpace);

    bool IsVisible() const
    {
      return shieldStyle->IsVisible();
    }

    double GetAlpha() const
    {
      return shieldStyle->GetAlpha();
    }

    size_t GetPriority() const
    {
      return shieldStyle->GetPriority();
    }

    double GetSize() const
    {
      return shieldStyle->GetSize();
    }

    const LabelProviderRef& GetLabel() const
    {
      return shieldStyle->GetLabel();
    }

    const Color& GetTextColor() const
    {
      return shieldStyle->GetTextColor();
    }

    const Color& GetBgColor() const
    {
      return shieldStyle->GetBgColor();
    }

    const Color& GetBorderColor() const
    {
      return shieldStyle->GetBorderColor();
    }

    double GetShieldSpace() const
    {
      return shieldSpace;
    }

    const ShieldStyleRef& GetShieldStyle() const
    {
      return shieldStyle;
    }

    static StyleDescriptorRef GetDescriptor();

    void CopyAttributes(const PathShieldStyle& other,
                        const std::set<Attribute>& attributes);
  };

  using PathShieldStyleRef = std::shared_ptr<PathShieldStyle>;

  /**
   * \ingroup Stylesheet
   *
   * A style for drawing text onto a path, the text following the
   * contour of the path.
   */
  class OSMSCOUT_MAP_API PathTextStyle CLASS_FINAL : public Style
  {
  public:
    enum Attribute {
      attrLabel,
      attrSize,
      attrTextColor,
      attrDisplayOffset,
      attrOffset,
      attrPriority
    };

  private:
    LabelProviderRef label;
    double           size;
    Color            textColor;
    double           displayOffset;
    double           offset;
    size_t           priority;

  public:
    PathTextStyle();

    void SetColorValue(int attribute, const Color& value) override;
    void SetDoubleValue(int attribute, double value) override;
    void SetLabelValue(int attribute, const LabelProviderRef& value) override;
    void SetUIntValue(int attribute, size_t value) override;

    PathTextStyle& SetLabel(const LabelProviderRef& label);
    PathTextStyle& SetSize(double size);
    PathTextStyle& SetTextColor(const Color& color);
    PathTextStyle& SetDisplayOffset(double value);
    PathTextStyle& SetOffset(double value);
    PathTextStyle& SetPriority(size_t value);

    bool IsVisible() const
    {
      return label &&
             GetSize()>0.0 &&
             textColor.IsVisible();
    }

    const LabelProviderRef& GetLabel() const
    {
      return label;
    }

    double GetSize() const
    {
      return size;
    }

    const Color& GetTextColor() const
    {
      return textColor;
    }

    double GetDisplayOffset() const
    {
      return displayOffset;
    }

    double GetOffset() const
    {
      return offset;
    }

    size_t GetPriority() const
    {
      return priority;
    }

    static StyleDescriptorRef GetDescriptor();

    void CopyAttributes(const PathTextStyle& other,
                        const std::set<Attribute>& attributes);
  };

  using PathTextStyleRef = std::shared_ptr<PathTextStyle>;

  /**
   * \ingroup Stylesheet
   *
   * The icon style allow the rendering of external images or internal symbols.
   */
  class OSMSCOUT_MAP_API IconStyle CLASS_FINAL : public Style
  {
  public:
    enum Attribute {
      attrSymbol,
      attrIconName,
      attrPosition,
      attrPriority,
      attrOverlay
    };

  private:
    SymbolRef    symbol;
    std::string  iconName; //!< name of the icon as given in style
    size_t       iconId;   //!< Id for external resource binding
    unsigned int width;    //!< width of icon in pixels
    unsigned int height;   //!< height of icon in pixels
    size_t       position; //!< Relative vertical position of the label
    size_t       priority; //!< Rendering priority
    bool         overlay;  //!< Render in any case, do not clip if it overlaps with something else. Default: false

  public:
    IconStyle();

    void SetBoolValue(int attribute, bool value) override;
    void SetStringValue(int attribute, const std::string& value) override;
    void SetSymbolValue(int attribute, const SymbolRef& value) override;
    void SetUIntValue(int attribute, size_t value) override;

    IconStyle& SetSymbol(const SymbolRef& symbol);
    IconStyle& SetIconName(const std::string& iconName);
    IconStyle& SetIconId(size_t id);
    IconStyle& SetWidth(unsigned int w);
    IconStyle& SetHeight(unsigned int h);
    IconStyle& SetPosition(size_t position);
    IconStyle& SetPriority(size_t priority);
    IconStyle& SetOverlay(bool overlay);

    size_t GetPriority() const
    {
      return priority;
    }

    bool IsVisible() const
    {
      return !iconName.empty() ||
              symbol;
    }

    const SymbolRef& GetSymbol() const
    {
      return symbol;
    }

    std::string GetIconName() const
    {
      return iconName;
    }

    size_t GetIconId() const
    {
      return iconId;
    }

    unsigned int GetWidth() const
    {
      return width;
    }

    unsigned int GetHeight() const
    {
      return height;
    }

    size_t GetPosition() const
    {
      return position;
    }

    bool IsOverlay() const
    {
      return overlay;
    }

    static StyleDescriptorRef GetDescriptor();

    void CopyAttributes(const IconStyle& other,
                        const std::set<Attribute>& attributes);
  };

  using IconStyleRef = std::shared_ptr<IconStyle>;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API DrawPrimitive
  {

  private:
    FillStyleRef   fillStyle;
    BorderStyleRef borderStyle;

  public:
    DrawPrimitive(const FillStyleRef& fillStyle,
                  const BorderStyleRef& borderStyle);
    virtual ~DrawPrimitive() = default;

    const FillStyleRef& GetFillStyle() const
    {
      return fillStyle;
    }

    const BorderStyleRef& GetBorderStyle() const
    {
      return borderStyle;
    }

    virtual ScreenBox GetBoundingBox() const = 0;
  };

  using DrawPrimitiveRef = std::shared_ptr<DrawPrimitive>;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API PolygonPrimitive CLASS_FINAL : public DrawPrimitive
  {
  private:
    std::list<Vertex2D> coords;

  public:
    PolygonPrimitive(const FillStyleRef& fillStyle,
                     const BorderStyleRef& borderStyle);

    void AddCoord(const Vertex2D& coord);

    const std::list<Vertex2D>& GetCoords() const
    {
      return coords;
    }

    ScreenBox GetBoundingBox() const override;
  };

  using PolygonPrimitiveRef = std::shared_ptr<PolygonPrimitive>;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API RectanglePrimitive CLASS_FINAL : public DrawPrimitive
  {
  private:
    Vertex2D topLeft;
    double   width;
    double   height;

  public:
    RectanglePrimitive(const Vertex2D& topLeft,
                       double width,
                       double height,
                       const FillStyleRef& fillStyle,
                       const BorderStyleRef& borderStyle);

    const Vertex2D& GetTopLeft() const
    {
      return topLeft;
    }

    const double& GetWidth() const
    {
      return width;
    }

    const double& GetHeight() const
    {
      return height;
    }

    ScreenBox GetBoundingBox() const override;
  };

  using RectanglePrimitiveRef = std::shared_ptr<RectanglePrimitive>;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API CirclePrimitive CLASS_FINAL : public DrawPrimitive
  {
  private:
    Vertex2D center;
    double   radius;

  public:
    CirclePrimitive(const Vertex2D& center,
                    double radius,
                    const FillStyleRef& fillStyle,
                    const BorderStyleRef& borderStyle);

    const Vertex2D& GetCenter() const
    {
      return center;
    }

    const double& GetRadius() const
    {
      return radius;
    }

    ScreenBox GetBoundingBox() const override;
  };

  using CirclePrimitiveRef = std::shared_ptr<CirclePrimitive>;

  /**
   * \ingroup Stylesheet
   *
   * Definition of a symbol. A symbol consists of a list of DrawPrimitives
   * with with assigned rendering styles.
   */
  class OSMSCOUT_MAP_API Symbol CLASS_FINAL
  {
  public:
    enum class ProjectionMode {
      MAP,
      GROUND
    };

  private:
    std::string                 name;               //!< The name of the symbol for reference
    ProjectionMode              projectionMode;     //!< Symbol is either in ground or map coordinates
    std::list<DrawPrimitiveRef> primitives;         //!< List of drawing priitive instances that make up the symbol shape
    ScreenBox                   mapBoundingBox;     //!< bounding box in map canvas coordinates [mm]
    ScreenBox                   groundBoundingBox;  //!< bounding box in ground coordinates [m]
    double                      maxBorderWidth=0;   //!< maximum border width [mm]

  public:
    explicit Symbol(const std::string& name,
                    ProjectionMode projectionMode);

    void AddPrimitive(const DrawPrimitiveRef& primitive);

    std::string GetName() const
    {
      return name;
    }

    Symbol::ProjectionMode GetProjectionMode() const
    {
      return projectionMode;
    }

    const std::list<DrawPrimitiveRef>& GetPrimitives() const
    {
      return primitives;
    }

    /**
     * bounding box in pixels for given projection
     */
    ScreenBox GetBoundingBox(const Projection &projection) const
    {
      if (projectionMode==ProjectionMode::GROUND) {
        return {Vertex2D(projection.GetMeterInPixel() * groundBoundingBox.GetMinX(),
                         projection.GetMeterInPixel() * groundBoundingBox.GetMinY()),
                Vertex2D(projection.GetMeterInPixel() * groundBoundingBox.GetMaxX(),
                         projection.GetMeterInPixel() * groundBoundingBox.GetMaxY())};
      }
      else {
        return {Vertex2D(projection.ConvertWidthToPixel(mapBoundingBox.GetMinX()),
                         projection.ConvertWidthToPixel(mapBoundingBox.GetMinY())),
                Vertex2D(projection.ConvertWidthToPixel(mapBoundingBox.GetMaxX()),
                         projection.ConvertWidthToPixel(mapBoundingBox.GetMaxY()))};
      }
    }

    /**
     * Maximum border width. As border is not accounted to bounding box and symbol dimension,
     * it is good to use this value as symbol margin to make sure that symbol is to cropped.
     *
     * @param projection
     * @return width in pixels
     */
    double GetMaxBorderWidth(const Projection &projection) const
    {
      return projection.ConvertWidthToPixel(maxBorderWidth);
    }

    /**
     * width in pixels for given projection
     */
    double GetWidth(const Projection &projection) const
    {
      return std::max(projection.ConvertWidthToPixel(mapBoundingBox.GetWidth()),
                      projection.GetMeterInPixel() * groundBoundingBox.GetWidth());
    }

    /**
     * height in pixels for given projection
     */
    double GetHeight(const Projection &projection) const
    {
      return std::max(projection.ConvertWidthToPixel(mapBoundingBox.GetHeight()),
                      projection.GetMeterInPixel() * groundBoundingBox.GetHeight());
    }

  };

  /**
   * \ingroup Stylesheet
   *
   * Style for repetive drawing of symbols on top of a path.
   */
  class OSMSCOUT_MAP_API PathSymbolStyle CLASS_FINAL : public Style
  {
  public:
    enum class RenderMode : int {
      fixed,
      scale
    };

    enum Attribute {
      attrSymbol,
      attrRenderMode,
      attrScale,
      attrSymbolSpace,
      attrDisplayOffset,
      attrOffset,
      attrOffsetRel
    };

  private:
    std::string slot;
    SymbolRef   symbol;
    RenderMode  renderMode=RenderMode::fixed;
    double      scale=1.0;
    double      symbolSpace=15.0;
    double      displayOffset=0.0;
    double      offset=0.0;
    OffsetRel   offsetRel{OffsetRel::base};

  public:
    PathSymbolStyle() = default;

    void SetDoubleValue(int attribute, double value) override;
    void SetSymbolValue(int attribute, const SymbolRef& value) override;
    void SetIntValue(int attribute, int value) override;

    PathSymbolStyle& SetSlot(const std::string& slot);

    PathSymbolStyle& SetSymbol(const SymbolRef& symbol);
    PathSymbolStyle& SetRenderMode(RenderMode renderMode);
    PathSymbolStyle& SetScale(double scale);
    PathSymbolStyle& SetSymbolSpace(double space);
    PathSymbolStyle& SetDisplayOffset(double value);
    PathSymbolStyle& SetOffset(double value);
    PathSymbolStyle& SetOffsetRel(OffsetRel offsetRel);

    bool IsVisible() const
    {
      return (bool)symbol;
    }

    const std::string& GetSlot() const
    {
      return slot;
    }

    const SymbolRef& GetSymbol() const
    {
      return symbol;
    }

    RenderMode GetRenderMode() const
    {
      return renderMode;
    }

    double GetScale() const
    {
      return scale;
    }

    double GetSymbolSpace() const
    {
      return symbolSpace;
    }

    bool HasDisplayOffset() const
    {
      return displayOffset!=0.0;
    }

    double GetDisplayOffset() const
    {
      return displayOffset;
    }

    bool HasOffset() const
    {
      return offset!=0.0;
    }

    double GetOffset() const
    {
      return offset;
    }

    OffsetRel GetOffsetRel() const
    {
      return offsetRel;
    }

    static StyleDescriptorRef GetDescriptor();

    void CopyAttributes(const PathSymbolStyle& other,
                        const std::set<Attribute>& attributes);
  };

  using PathSymbolStyleRef = std::shared_ptr<PathSymbolStyle>;

  class OSMSCOUT_MAP_API RenderModeEnumAttributeDescriptor CLASS_FINAL : public StyleEnumAttributeDescriptor
  {
  public:
    RenderModeEnumAttributeDescriptor(const std::string& name,
                                      int attribute)
      : StyleEnumAttributeDescriptor(name,
                                     attribute)
    {
      AddEnumValue("fixed",(int)PathSymbolStyle::RenderMode::fixed);
      AddEnumValue("scale",(int)PathSymbolStyle::RenderMode::scale);
    }
  };

}

#endif
