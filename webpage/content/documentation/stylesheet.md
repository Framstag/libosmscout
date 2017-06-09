---
date: "2016-06-30T19:50:42+02:00"
title: "Style sheet syntax"
description: "Description of the style sheet syntax in *.oss files"
weight: 9

menu:
  main:
    Parent: "documentation"
    Weight: 9
---


The OSS file defines how data is visualized on the map. It is using a
cascading style sheet syntax (CSS) like as it is known from HTML.

Style sheet files are also named `OSS` files
(from lib<b>o</b>smscout <b>s</b>tyle <b>s</b>heet). They should end in `*.oss`.

If you are interested in a more detailed syntax description, please take a
look at the OSS.atg file, which contains (besides the actual AST building
code) the EBNF.

While reading the documentation you should at the same time look into the
default style supplied in the git repository (`stylesheets/standard.oss`).

Also take a look at the example at the end of this document.

## Structure

The overall structure of an OSS file is as follows:

```
  FLAGS
  ...
  ODER WAYS
  ...
  CONST
  ...
  SYMBOL
  ...
  STYLE
  ...
  END
```

In the following sections we describe the content of each part of the OSS file.

## Section `FLAG`

The flag sections allows you to define a number of flags you can later
on make use of as part of an conditional evaluation using an
variant of the classic "IF...ELSE IF...ELSE" syntax:

```
IF [!] <variable> {
 ...
}
ELIF <variable> {
 ...
}
ELSE {
 ...
}
```
 
Such statements can be used later on in the CONST and in the STYLE section.

The value of the flag in the FLAG section defines the default value.
You can manipulate flags before style loading programatically,
allowing different style variant in your application bases on the same config
file. Examples:

* Show/hide certain features based on configuration
* Support day/night modusby choosing different const values.

## Section `ORDER WAYS`

This section defines in which order ways are rendered. It defines a
number groups which contains ways types. Ways types in the same group
have the same rendering priority. Groups further down the style sheet
are rendered below ways further up in the GROUP list. Rendering order
is only relevant if ways of different types cross each other. Note
that ways are first ordered by layer, then within a layer by rendering
priority. So a bridge will always be rendered on top a "bridged" way,
a tunnel always below a "tunneled" ways.

## Section `CONST`

This (currently) allows you to define named constants for magnification levels,
numerical values and colors.

magnification values and numeric values just get a "name" as alias. Their values
can be used anywhere a magnification or a numerical value can be used.

Named colors can be used anywhere you can use "raw" rgb(a) values using the
"@"<colorname> syntax. Note it is possible to enhance the CONST section
to support constants for other types, too. Just make a feature request!

Example:
```
  // Magnification aliases
  MAG stepsMag                     = veryClose;
  MAG labelPathsMag                = veryClose;

  // Label priority aliases
  UINT labelPrioContinent          = 1;
  UINT labelPrioIsland             = 1;

  // Building color aliases
  COLOR buildingColor              = #d9d9d9;
  COLOR buildingBorderColor        = darken(@buildingColor, 0.3);
  COLOR buildingLabelColor         = darken(@buildingColor, 0.5);
```

## Section `SYMBOL`

The SYMBOL sections allows you to define simple vectorized symbols
consisting of one or more simple primitives like rectangles and circles.

It is medium complicated for a backend to support symbols and likely
simpler than support of external images or SVGs. Use SYMBOLs for things
like oneway arrows, simple signs or similar.

Currently the following primives are supported:

* `RECTANGLE <x> , <y> <width> x <height> { area style definitions }`
* `CIRCLE    <x> , <y> <radius> { area style definitions }`
* `POLYGON   <x> , <y> ...more coordinates... { area style definitions }`

"area style definitions" are normal style definitions for areas as
used within the STYLE section.

## Section `STYLE`

The style section is a CSS-like aproach to define a number of filters
for objects (nodes, areas, ways) put into the renderer, where each
filter can manipulate the rendering attributes of the object.

By default an object is not rendered at all. You need to set at least
some attributes to get an object rendered.

The fundamental aspect of this sections are filters, objects and
the style definition itself.


## Style filter

Filter are defined in "[" ... "]" brackets. The following filter
criteria exist:
   
`GROUP <groupname> "," <groupname>...`
:  matches all types that are in all the given groups (AND semantic)

`FEATURE <featurename> "," <featurename>...`
:  matches all types that have ALL the given features (AND semantic) and in
turn all object that have the feature set during runtime (so objects which have
a type that has this feature but the feature is not actually present for 
agiven nstance of this type do not match).

`PATH`
: matches all types wihich are annotated as `PATH` in the OST file.

`TYPE <typename> "," <typename>...`
: this filter is valid only for objects that match one of the given types
  (OR semantic).
  
  
`MAG <minLevel> - <maxLevel>`
: this filter is only valid for the given magnification interval. `MinLevel` or
  `Maxlevel` is optional with the obvious default.
  
`ONEWAY`
: this filter only matches for oneways.

`SIZE <widthInMeter>m <minWidthMM>mm:<minWidthPx>px "<" <maxWidthMM>mm:<maxWidthPx>px`
: this filter restrict objects by dimension of things on map in releation
  to some size in real world. The minimum interval or
  the maximum interval is optional (with 0 and infivity as
  defaults). For each border the size in mm or the size in pixel
  is optional, too. The criteria transforms the size in meter
  to a size on map. This size on the map must be in the given
  interval.

If a filter criteria is not given, the filter critera matches
everything (all types, all magnifications,...). Criteria can only
be used once in a filter and criteria must be written in the order
above.

A filter operates on a directly following style object, It can
also be followed by a block ("`{`"..."`}`") which then can recursively
contain additional filters and or style definitions.

## Style attributes

A style definition consists of a starting "`{`", a list of attributes
with values ("`<attributeName> : <value> ;`") and a closing
"`}`".

## Attribute types

`String`
: A textual value surounded by `"`. Example: `"wood"`.

`Int`
: Numerical value

`Bool`
: Boolean value, only allowed constants are `true` and `false`.

`Color`
: Either a 6 or 8 digit hexedacimal value starting with `#` or a color 
  constant starting with `@`. Example: `#ff0000` (the color red).
  
`Dash`
: A sequence of one or more positive numerical values (>0) separated by a colon.
  Size is in millimeter. Example: `3,1`

`ScreenSize`
: An (dependending on the actual attribute) a negative or positive numeric value
  followed by `mm` (millimeter). Defines a on screen size in millimeter.
  Example: `5mm`
  
`GroundSize`
: An (dependending on the actual attribute) a negative or positive numeric value
  followed by `mm` (meter). Defines a on screen size in meter.
  Example: `5m`
  
`Magnification`
: A numerical magnification value (>=0) or one of the magnification constants.
  
`Cap`
: One of the constants `butt`, `round` or `square` describing the type of line
  termination.

`TextStyle`
: One of the constants `normal` or `emphasize` describing the type of text
  rendering.
  
`FeatureAttr`
: Name of a feature followed by a point followed by the name of the feature
attribute that holds the to be evaluated value. Example: `Name.name`
  
## Slots

Certain styles can get applied multiple times for an object (Node, Way, Area).
In this case the style name is postpended by an `#` and a slot name.
  
## Attributes of the style objects

### LineStyle - Drawing lines

Currently allowed instances:

* `WAY#<slot>`

The line style has the following attributes:

Name         |Type         |Description
-------------|-------------|-----------
color        |Color        |Color of the line. if not set, color is transparent (a lines is not drawn)
dash         |Dash         |Size of the dashes. If not set lines is solid
gapColor     |Color        |Color drawn in the "gap", if not set gapColor is transparent
displayWidth |ScreenSize   |width of the line in millimeter ("size on map")
width        |GroundSize   |width of the line in meters ("real word dimension"). Note that if the object itself has a "width" feature, this value will be replaced with the actual value!
displayOffset|ScreenSize   |Offset of drawn line in relation to the actual path.
offset       |FroundSize   |Offset of the drawn line in relation to the actual path.
joinCap      |Cap          |Cap in case where lines join.
endCap       |Cap          |Cap in the case where the lines ends without joining another line.
priority     |Int          |Drawing priority in relation to other slots/pathes of the same object. Smaller values are drawn first.

If `displayWidth` and `width` are both set, the resulting pixel values will be added.

### BorderStyle - Border of areas

### LineStyle - Drawing lines

Currently allowed instances:

* `AREA.BORDER#<slot>`

The border style has the following attributes:

Name         |Type         |Description
-------------|-------------|-----------
color        |Color        |Color of the line. if not set, color is transparent (a lines is not drawn)
dash         |Dash         |Size of the dashes. If not set lines is solid
gapColor     |Color        |Color drawn in the "gap", if not set gapColor is transparent
displayWidth |ScreenSize   |width of the line in millimeter ("size on map")
width        |GroundSize   |width of the line in meters ("real word dimension"). Note that if the object itself has a "width" feature, this value will be replaced with the actual value!
displayOffset|ScreenSize   |Offset of drawn line in relation to the actual path.
offset       |FroundSize   |Offset of the drawn line in relation to the actual path.
priority     |Int          |Drawing priority in relation to other slots/pathes of the same object. Smaller values are drawn first.

If `displayWidth` and `width` are both set, the resulting pixel values will be added.


### FillStyle - Filling areas

Currently allowed instances:

* `AREA`

The area fill style has the following attributes:

Name         |Type         |Description
-------------|-------------|-----------
color        |Color        |The fill color. By default transparent so area will not be filled.
pattern      |String       |(File)name of an image that is repeately drawn on top of the area. By default no pattern will be drawn.
patternMinMag|Magnification|Minimum magnification for the pattern to be drawn (deprecated, should be better defined by a filter). By default this is magWorld, so the pattern will always be drawn

### TextStyle - Drawing labels for nodes or areas

Currently allowed instances:

* `NODE.TEXT#<slot>`
* `AREA.TEXT#<slot>`

The text style for point labels has the following attributes:

Name         |Type         |Description
-------------|-------------|-----------
label        |FeatureAttr  |The name of the feature attribute to be rendered.
style        |TextStyle    |Depending on the value (normal or emphasize) the label is either drawn normal or (depending on the backend the visualisation may be different) somehow emphasized.
color        |Color        |The color of the text
size         |Int          |The size of the text relative to the standard text size. 2.0 for example generates a text twice as height as normal.
scaleMag     |Magnification|Starting with the given magnification in the label is drawn bigger but on the same time with increasing transparency with increasing magnification, genrating an overlay-like effect.
priority     |Int          |numeric value defining a relative priority between labels. Labels with a lower value will be drawn in favour of labels with a higher priority value. Note that labels with a certain alpha value will be ignored (so giant scaleMag labels will not "kill" all other labels beneeth).
autoSize     |Bool         |The size of the label is automatically scaled to fit the height of the area itself. Thus bigger areas will get bigger labels, label with not be higher than the actual area.

### PathTextStyle - Draw labels onto ways and area borders

Currently allowed instances:

* `WAY.TEXT`
* `AREA.BORDERTEXT`

The text style for contour labels has the following attributes:

Name         |Type         |Description
-------------|-------------|-----------
label        |FeatureAttr  |The name of the feature attribute to be rendered.
color        |Color        |The color of the text
size         |Int          |The size of the text relative to the standard text size. 2.0 for example genrates a text twice as height as normal.
displayOffset|ScreenSize   |Offset of drawn text in relation to the actual path.
offset       |FroundSize   |Offset of the drawn text in relation to the actual path.

### IconStyle - Drawing icons for nodes and areas

The icon style has the following attributes:

Currently allowed instances:

* `NODE.ICON`
* `AREA.ICON`

Name         |Type         |Description
-------------|-------------|-----------
symbol       |String       |The name of the symbol to draw
name         |String       |The name of the icon to draw. An icon name is normally mapped onto an external image. The supported image formats depend on the actual backend.

### ShieldStyle - Drawing plates for ways

Currently allowed instances:

* `WAY.SHIELD`

The shield style has the following attributes:

Name           |Type         |Description
---------------|-------------|-----------
label          |FeatureAttr  |The name of the feature attribute to be rendered.
color          |Color        |The color of the text
backgroundColor|Color        |The color of the shield itself
borderColor    |Color        |The color of the border.
size           |Int          |The size of the text relative to the standard text size. 2.0 for example genrates a text twice as height as normal.
priority       |Int          |numeric value defining a relative priority between labels. Labels with a lower value will be drawn in favour of labels with a higher priority value. Note that labels with a certain alpha value will be ignored (so giant scaleMag labels will not "kill" all other labels beneeth).
shieldSpace    |ScreenSize   |Space between each shield on a path.

### PathSymbolStyle - Drawing symbols onto paths

Currently allowed instances:

* `WAY.SYMBOL`
* `AREA.BORDERSYMBOL`

Name         |Type         |Description
-------------|-------------|-----------
symbol       |String       |The name of the symbol to draw
symbolSpace  |ScreenSize   |Space between each symbol on a path.
displayOffset|ScreenSize   |Offset of drawn symbol in relation to the actual path.
offset       |FroundSize   |Offset of the drawn symbol in relation to the actual path.

## Label and icon placement

Icons and labels will we drawn vertically. Relative label positioning will be handled
by position numbers. A higher position number will get rendered further down the list,
a lower one furth to the top.

Given the following labels and their position (displaying an alternate name for the mountain):

* 3: 8848m
* 1: Mount Everest
* 2: Everest

The labels will be rendered (centered):

```
Mount Everest
   Everest
    8848m
```

Gaps in the position sequence are possible. The default is 0. The order of
labels with the same position is undefined.

## Available functions

Currently the following functions exist:

Name           |Type         |Description
---------------|-------------|-----------
lighten        |Color, Double|Returns a lighter version of the given color
darken         |Color, Double|Returns a darker version of the given color

## Example

```
 [MAG continent-] {
   [TYPE highway_motorway] {
     [SIZE 20m 0.45mm:3px<] {
       WAY#outline {
         color: darken(@motorwayColor,0.4);
         width: 20m;
         displayWidth: 0.5mm;
         priority: -1;
         joinCap: butt;
       }
       
       WAY {
         color: @motorwayColor;
         width: 20m;
       }
     }
     [SIZE 20m <0.45mm:3px] WAY {
       color: lighten(@motorwayColor,0.3);
       displayWidth: 0.45mm;
     }
   }
 }
```

The `[MAG continent-]` filter activates the given rules for the magnification zoom
level range `continent` (zoom level 4) and higher zooms (note, that the interval
is open!)

The filter `[TYPE highway_motorway]` assures, that the given rules are only
applied for objects of type `highway_motorways` (which is the type for
motorways).

The `[SIZE 20m 0.45mm:3px<]` (note the "`<`" instead of "`>`" and also its
position) is triggered, if 20m on the ground (the estimated default width of an
highway) are at least in the display either 0.45mm or 3 pixel. In this case we
do a complex visualisation of the motorway consisting of an outline and an
"normal" line. In the other case the motorway is just drawn as a line without
an outline.

In complex mode the motorway is drawn with normal color and a even
darker outline, in the simple mode its drawn lighter than normal (to make the
map appear less crowdy).

The priority for the outline makes sure that it is drawn before the WAY line.
The trick for the outline is, that it is drawn as a thicker line (displayWidth)
with a slightly thinner line for the way itself drawn on top. So while it looks
like the outline is implemented by drawing very thin lines to the left and right
of the way, in reality it is not done this way.
