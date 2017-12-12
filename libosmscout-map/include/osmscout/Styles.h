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

#include <osmscout/private/MapImportExport.h>

#include <osmscout/Pixel.h>
#include <osmscout/util/Color.h>

#include <osmscout/StyleDescription.h>

namespace osmscout {

  /**
   * \ingroup Stylesheet
   *
   * Style options for a line.
   */
  class OSMSCOUT_MAP_API LineStyle : public Style
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
      attrDisplayWidth,
      attrWidth,
      attrDisplayOffset,
      attrOffset,
      attrJoinCap,
      attrEndCap,
      attrDashes,
      attrPriority,
      attrZIndex
    };

  private:
    std::string         slot;
    Color               lineColor;
    Color               gapColor;
    double              displayWidth;
    double              width;
    double              displayOffset;
    double              offset;
    CapStyle            joinCap;
    CapStyle            endCap;
    std::vector<double> dash;
    int                 priority;
    int                 zIndex;

  public:
    LineStyle();
    LineStyle(const LineStyle& style);

    void SetColorValue(int attribute, const Color& value) override;
    void SetDoubleValue(int attribute, double value) override;
    void SetDoubleArrayValue(int attribute, const std::vector<double>& value) override;
    void SetIntValue(int attribute, int value) override;

    LineStyle& SetSlot(const std::string& slot);

    LineStyle& SetLineColor(const Color& color);
    LineStyle& SetGapColor(const Color& color);
    LineStyle& SetDisplayWidth(double value);
    LineStyle& SetWidth(double value);
    LineStyle& SetDisplayOffset(double value);
    LineStyle& SetOffset(double value);
    LineStyle& SetJoinCap(CapStyle joinCap);
    LineStyle& SetEndCap(CapStyle endCap);
    LineStyle& SetDashes(const std::vector<double>& dashes);
    LineStyle& SetPriority(int priority);
    LineStyle& SetZIndex(int zIndex);

    inline bool IsVisible() const
    {
      return (displayWidth>0.0 ||
              width>0.0) &&
              lineColor.IsVisible();
    }

    inline const std::string& GetSlot() const
    {
      return slot;
    }

    inline const Color& GetLineColor() const
    {
      return lineColor;
    }

    inline const Color& GetGapColor() const
    {
      return gapColor;
    }

    inline double GetDisplayWidth() const
    {
      return displayWidth;
    }

    inline double GetWidth() const
    {
      return width;
    }

    inline double GetDisplayOffset() const
    {
      return displayOffset;
    }

    inline double GetOffset() const
    {
      return offset;
    }

    inline CapStyle GetJoinCap() const
    {
      return joinCap;
    }

    inline CapStyle GetEndCap() const
    {
      return endCap;
    }

    inline bool HasDashes() const
    {
      return !dash.empty();
    }

    inline const std::vector<double>& GetDash() const
    {
      return dash;
    }

    inline int GetPriority() const
    {
      return priority;
    }

    inline int GetZIndex() const
    {
      return zIndex;
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


  typedef std::shared_ptr<LineStyle>                       LineStyleRef;

  /**
   * \ingroup Stylesheet
   *
   * Style options for filling an area.
   */
  class OSMSCOUT_MAP_API FillStyle : public Style
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
    FillStyle(const FillStyle& style);

    void SetStringValue(int attribute, const std::string& value) override;
    void SetColorValue(int attribute, const Color& value) override;
    void SetMagnificationValue(int attribute, const Magnification& value) override;

    FillStyle& SetFillColor(const Color& color);
    FillStyle& SetPattern(const std::string& pattern);
    void SetPatternId(size_t id) const;
    FillStyle& SetPatternMinMag(const Magnification& mag);

    inline bool IsVisible() const
    {
      return (fillColor.IsVisible() ||
              !pattern.empty());
    }

    inline const Color& GetFillColor() const
    {
      return fillColor;
    }

    inline bool HasPattern() const
    {
      return !pattern.empty();
    }

    inline std::string GetPatternName() const
    {
      return pattern;
    }

    inline size_t GetPatternId() const
    {
      return patternId;
    }

    inline const Magnification& GetPatternMinMag() const
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

  typedef std::shared_ptr<FillStyle>                       FillStyleRef;

  /**
   * \ingroup Stylesheet
   *
   * Style options for borders around an area.
   */
  class OSMSCOUT_MAP_API BorderStyle : public Style
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
    BorderStyle(const BorderStyle& style);

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

    inline bool IsVisible() const
    {
      return width>0 && color.IsVisible();
    }

    inline const std::string& GetSlot() const
    {
      return slot;
    }

    inline const Color& GetColor() const
    {
      return color;
    }

    inline const Color& GetGapColor() const
    {
      return gapColor;
    }

    inline double GetWidth() const
    {
      return width;
    }

    inline bool HasDashes() const
    {
      return !dash.empty();
    }

    inline const std::vector<double>& GetDash() const
    {
      return dash;
    }

    inline double GetDisplayOffset() const
    {
      return displayOffset;
    }

    inline double GetOffset() const
    {
      return offset;
    }

    inline int GetPriority() const
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

  typedef std::shared_ptr<BorderStyle>                         BorderStyleRef;

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
    LabelStyle(const LabelStyle& style);

    ~LabelStyle() override;

    virtual bool IsVisible() const = 0;
    virtual double GetAlpha() const = 0;

    LabelStyle& SetPriority(size_t priority);

    virtual LabelStyle& SetSize(double size);

    inline size_t GetPriority() const
    {
      return priority;
    }

    inline double GetSize() const
    {
      return size;
    }
  };

  typedef std::shared_ptr<LabelStyle> LabelStyleRef;

  /**
   * \ingroup Stylesheet
   *
   * A textual label.
   */
  class OSMSCOUT_MAP_API TextStyle : public LabelStyle
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
      attrStyle,
      attrScaleAndFadeMag,
      attrAutoSize
    };

  private:
    std::string      slot;
    LabelProviderRef label;           //!< The label - a reference to a feature and its label index
    size_t           position;        //!< Relative vertical position of the label
    Color            textColor;       //!< Color of text
    Style            style;           //!< Style of the text
    Magnification    scaleAndFadeMag; //!< Automatic pseudo-autoSize scaling for nodes
    bool             autoSize;        //!< Calculate the size of the label base don the height of the area

  public:
    TextStyle();
    TextStyle(const TextStyle& style);

    void SetBoolValue(int attribute, bool value) override;
    void SetColorValue(int attribute, const Color& value) override;
    void SetMagnificationValue(int attribute, const Magnification& value) override;
    void SetDoubleValue(int attribute, double value) override;
    void SetIntValue(int attribute, int value) override;
    void SetUIntValue(int attribute, size_t value) override;
    void SetLabelValue(int attribute, const LabelProviderRef& value) override;

    TextStyle& SetSlot(const std::string& slot);

    TextStyle& SetPriority(uint8_t priority);
    TextStyle& SetSize(double size) override;
    TextStyle& SetLabel(const LabelProviderRef& label);
    TextStyle& SetPosition(size_t position);
    TextStyle& SetTextColor(const Color& color);
    TextStyle& SetStyle(Style style);
    TextStyle& SetScaleAndFadeMag(const Magnification& mag);
    TextStyle& SetAutoSize(bool autoSize);

    inline bool IsVisible() const override
    {
      return label &&
             GetTextColor().IsVisible();
    }

    inline double GetAlpha() const override
    {
      return textColor.GetA();
    }

    inline const std::string& GetSlot() const
    {
      return slot;
    }

    inline const LabelProviderRef& GetLabel() const
    {
      return label;
    }

    inline size_t GetPosition() const
    {
      return position;
    }

    inline const Color& GetTextColor() const
    {
      return textColor;
    }

    inline const Style& GetStyle() const
    {
      return style;
    }

    inline Magnification GetScaleAndFadeMag() const
    {
      return scaleAndFadeMag;
    }

    inline bool GetAutoSize() const
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

  typedef std::shared_ptr<TextStyle>                       TextStyleRef;

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
    ShieldStyle(const ShieldStyle& style);

    ShieldStyle& SetLabel(const LabelProviderRef& label);
    ShieldStyle& SetPriority(uint8_t priority);
    ShieldStyle& SetSize(double size) override;
    ShieldStyle& SetTextColor(const Color& color);
    ShieldStyle& SetBgColor(const Color& color);
    ShieldStyle& SetBorderColor(const Color& color);

    inline bool IsVisible() const override
    {
      return label &&
             GetTextColor().IsVisible();
    }

    inline double GetAlpha() const override
    {
      return textColor.GetA();
    }

    inline const LabelProviderRef& GetLabel() const
    {
      return label;
    }

    inline const Color& GetTextColor() const
    {
      return textColor;
    }

    inline const Color& GetBgColor() const
    {
      return bgColor;
    }

    inline const Color& GetBorderColor() const
    {
      return borderColor;
    }

    void CopyAttributes(const ShieldStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef std::shared_ptr<ShieldStyle>                         ShieldStyleRef;

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
    PathShieldStyle(const PathShieldStyle& style);

    void SetLabelValue(int attribute, const LabelProviderRef& value) override;
    void SetColorValue(int attribute, const Color& value) override;
    void SetDoubleValue(int attribute, double value) override;
    void SetUIntValue(int attribute, size_t value) override;

    PathShieldStyle& SetLabel(const LabelProviderRef& label);
    PathShieldStyle& SetPriority(uint8_t priority);
    PathShieldStyle& SetSize(double size);
    PathShieldStyle& SetTextColor(const Color& color);
    PathShieldStyle& SetBgColor(const Color& color);
    PathShieldStyle& SetBorderColor(const Color& color);
    PathShieldStyle& SetShieldSpace(double shieldSpace);

    inline bool IsVisible() const
    {
      return shieldStyle->IsVisible();
    }

    inline double GetAlpha() const
    {
      return shieldStyle->GetAlpha();
    }

    inline uint8_t GetPriority() const
    {
      return shieldStyle->GetPriority();
    }

    inline double GetSize() const
    {
      return shieldStyle->GetSize();
    }

    inline const LabelProviderRef& GetLabel() const
    {
      return shieldStyle->GetLabel();
    }

    inline const Color& GetTextColor() const
    {
      return shieldStyle->GetTextColor();
    }

    inline const Color& GetBgColor() const
    {
      return shieldStyle->GetBgColor();
    }

    inline const Color& GetBorderColor() const
    {
      return shieldStyle->GetBorderColor();
    }

    inline double GetShieldSpace() const
    {
      return shieldSpace;
    }

    inline const ShieldStyleRef& GetShieldStyle() const
    {
      return shieldStyle;
    }

    static StyleDescriptorRef GetDescriptor();

    void CopyAttributes(const PathShieldStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef std::shared_ptr<PathShieldStyle>                             PathShieldStyleRef;

  /**
   * \ingroup Stylesheet
   *
   * A style for drawing text onto a path, the text following the
   * contour of the path.
   */
  class OSMSCOUT_MAP_API PathTextStyle : public Style
  {
  public:
    enum Attribute {
      attrLabel,
      attrSize,
      attrTextColor,
      attrDisplayOffset,
      attrOffset
    };

  private:
    LabelProviderRef label;
    double           size;
    Color            textColor;
    double           displayOffset;
    double           offset;

  public:
    PathTextStyle();
    PathTextStyle(const PathTextStyle& style);

    void SetColorValue(int attribute, const Color& value) override;
    void SetDoubleValue(int attribute, double value) override;
    void SetLabelValue(int attribute, const LabelProviderRef& value) override;

    PathTextStyle& SetLabel(const LabelProviderRef& label);
    PathTextStyle& SetSize(double size);
    PathTextStyle& SetTextColor(const Color& color);
    PathTextStyle& SetDisplayOffset(double value);
    PathTextStyle& SetOffset(double value);

    inline bool IsVisible() const
    {
      return label &&
             textColor.IsVisible();
    }

    inline const LabelProviderRef& GetLabel() const
    {
      return label;
    }

    inline double GetSize() const
    {
      return size;
    }

    inline const Color& GetTextColor() const
    {
      return textColor;
    }

    inline double GetDisplayOffset() const
    {
      return displayOffset;
    }

    inline double GetOffset() const
    {
      return offset;
    }

    static StyleDescriptorRef GetDescriptor();

    void CopyAttributes(const PathTextStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef std::shared_ptr<PathTextStyle>                           PathTextStyleRef;

  /**
   * \ingroup Stylesheet
   *
   * The icon style allow the rendering of external images or internal symbols.
   */
  class OSMSCOUT_MAP_API IconStyle : public Style
  {
  public:
    enum Attribute {
      attrSymbol,
      attrIconName,
      attrPosition
    };

  private:
    SymbolRef   symbol;
    std::string iconName; //!< name of the icon as given in style
    size_t      iconId;   //!< Id for external resource binding
    size_t      position; //!< Relative vertical position of the label

  public:
    IconStyle();
    IconStyle(const IconStyle& style);

    void SetStringValue(int attribute, const std::string& value) override;
    void SetSymbolValue(int attribute, const SymbolRef& value) override;
    void SetUIntValue(int attribute, size_t value) override;

    IconStyle& SetSymbol(const SymbolRef& symbol);
    IconStyle& SetIconName(const std::string& iconName);
    IconStyle& SetIconId(size_t id);
    IconStyle& SetPosition(size_t position);

    inline bool IsVisible() const
    {
      return !iconName.empty() ||
              symbol;
    }

    inline const SymbolRef& GetSymbol() const
    {
      return symbol;
    }

    inline std::string GetIconName() const
    {
      return iconName;
    }

    inline size_t GetIconId() const
    {
      return iconId;
    }

    inline size_t GetPosition() const
    {
      return position;
    }

    static StyleDescriptorRef GetDescriptor();

    void CopyAttributes(const IconStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef std::shared_ptr<IconStyle>                       IconStyleRef;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API DrawPrimitive
  {
  public:
    enum class ProjectionMode {
      MAP,
      GROUND
    };

  private:
    ProjectionMode projectionMode;
    FillStyleRef   fillStyle;
    BorderStyleRef borderStyle;

  public:
    DrawPrimitive(ProjectionMode projectionMode,
                  const FillStyleRef& fillStyle,
                  const BorderStyleRef& borderStyle);
    virtual ~DrawPrimitive();

    inline ProjectionMode GetProjectionMode() const
    {
      return projectionMode;
    }

    inline const FillStyleRef& GetFillStyle() const
    {
      return fillStyle;
    }

    inline const BorderStyleRef& GetBorderStyle() const
    {
      return borderStyle;
    }

    virtual void GetBoundingBox(double& minX,
                                double& minY,
                                double& maxX,
                                double& maxY) const = 0;
  };

  typedef std::shared_ptr<DrawPrimitive> DrawPrimitiveRef;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API PolygonPrimitive : public DrawPrimitive
  {
  private:
    std::list<Vertex2D> coords;

  public:
    PolygonPrimitive(ProjectionMode projectionMode,
                     const FillStyleRef& fillStyle,
                     const BorderStyleRef& borderStyle);

    void AddCoord(const Vertex2D& coord);

    inline const std::list<Vertex2D>& GetCoords() const
    {
      return coords;
    }

    void GetBoundingBox(double& minX,
                        double& minY,
                        double& maxX,
                        double& maxY) const override;
  };

  typedef std::shared_ptr<PolygonPrimitive> PolygonPrimitiveRef;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API RectanglePrimitive : public DrawPrimitive
  {
  private:
    Vertex2D topLeft;
    double   width;
    double   height;

  public:
    RectanglePrimitive(ProjectionMode projectionMode,
                       const Vertex2D& topLeft,
                       double width,
                       double height,
                       const FillStyleRef& fillStyle,
                       const BorderStyleRef& borderStyle);

    inline const Vertex2D& GetTopLeft() const
    {
      return topLeft;
    }

    inline const double& GetWidth() const
    {
      return width;
    }

    inline const double& GetHeight() const
    {
      return height;
    }

    void GetBoundingBox(double& minX,
                        double& minY,
                        double& maxX,
                        double& maxY) const override;
  };

  typedef std::shared_ptr<RectanglePrimitive> RectanglePrimitiveRef;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API CirclePrimitive : public DrawPrimitive
  {
  private:
    Vertex2D center;
    double   radius;

  public:
    CirclePrimitive(ProjectionMode projectionMode,
                    const Vertex2D& center,
                    double radius,
                    const FillStyleRef& fillStyle,
                    const BorderStyleRef& borderStyle);

    inline const Vertex2D& GetCenter() const
    {
      return center;
    }

    inline const double& GetRadius() const
    {
      return radius;
    }

    void GetBoundingBox(double& minX,
                        double& minY,
                        double& maxX,
                        double& maxY) const override;
  };

  typedef std::shared_ptr<CirclePrimitive> CirclePrimitiveRef;

  /**
   * \ingroup Stylesheet
   *
   * Definition of a symbol. A symbol consists of a list of DrawPrimitives
   * with with assigned rendering styes.
   */
  class OSMSCOUT_MAP_API Symbol
  {
  private:
    std::string                 name;
    std::list<DrawPrimitiveRef> primitives;
    double                      minX;
    double                      minY;
    double                      maxX;
    double                      maxY;

  public:
    explicit Symbol(const std::string& name);

    void AddPrimitive(const DrawPrimitiveRef& primitive);

    inline std::string GetName() const
    {
      return name;
    }

    inline const std::list<DrawPrimitiveRef>& GetPrimitives() const
    {
      return primitives;
    }

    inline void GetBoundingBox(double& minX,
                               double& minY,
                               double& maxX,
                               double& maxY) const
    {
      minX=this->minX;
      minY=this->minY;

      maxX=this->maxX;
      maxY=this->maxY;
    }

    inline double GetWidth() const
    {
      return maxX-minX;
    }

    inline double GetHeight() const
    {
      return maxY-minY;
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Style for repetive drawing of symbols on top of a path.
   */
  class OSMSCOUT_MAP_API PathSymbolStyle : public Style
  {
  public:

    enum Attribute {
      attrSymbol,
      attrSymbolSpace,
      attrDisplayOffset,
      attrOffset
    };

  private:
    SymbolRef symbol;
    double    symbolSpace;
    double    displayOffset;
    double    offset;

  public:
    PathSymbolStyle();
    PathSymbolStyle(const PathSymbolStyle& style);

    void SetDoubleValue(int attribute, double value) override;
    void SetSymbolValue(int attribute, const SymbolRef& value) override;

    PathSymbolStyle& SetSymbol(const SymbolRef& symbol);
    PathSymbolStyle& SetSymbolSpace(double space);
    PathSymbolStyle& SetDisplayOffset(double value);
    PathSymbolStyle& SetOffset(double value);

    inline bool IsVisible() const
    {
      return (bool)symbol;
    }

    inline const SymbolRef& GetSymbol() const
    {
      return symbol;
    }

    inline double GetSymbolSpace() const
    {
      return symbolSpace;
    }

    inline double GetDisplayOffset() const
    {
      return displayOffset;
    }

    inline double GetOffset() const
    {
      return offset;
    }

    static StyleDescriptorRef GetDescriptor();

    void CopyAttributes(const PathSymbolStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef std::shared_ptr<PathSymbolStyle>                             PathSymbolStyleRef;
}

#endif
