---
date: "2016-06-30T19:50:42+02:00"
title: "Type definition"
description: "Typedefinition using *.ost files"
weight: 10

menu:
  main:
    Parent: "documentation"
    Weight: 10
---

The OST file defines how data is parsed from the OSM export files (exported
from central OSM database, imported into the libosmsocut database using the
Import tool).

Type definition files are also named `OST` files
(from lib<b>o</b>smscout <b>t</b>ypedefinition). They should end in `*.ost`.

If you are interested in a more detailed syntax description, please take a
look at the OST.atg file, which contains (besides the actual AST building
code) the EBNF.

While reading the documentation you should at the same time look into the
default style supplied in the git repository (`stylesheets/map.ost`).

## Concepts

### Definition of types

The OST file defines types based on some boolean expression, which looks for the
existance of certain tags and look at their values.

Example: A building is something that has a tag `building` and where the tag
value is not a boolean "false" value. In concrete the value of the building
tag must not be either `false`, `no` or `0`.

### Order of evaluation

Types are evaluated from top to bottom. That means, that for an object 
the type conditions (if matching the object type) are evaluated from top to
bottom until a condition matches. In this case the object is marked as having
the given type and evaluation is stopped. If there is no match, the type is
`ignore`.

### Ignoring types

Normally you do not want to import everything that is in the OSM export file,
but just a subset.

You might think of just not defining that time. But a better approach is to 
*do* define the type but mark is as `IGNORE`. The reason for this is, that
objects that match this type might have other tags that may match to some other 
type. If you do not define the main type the object might thus mismatched with
some other type.

### Object type

Libosmscout know nodes, ways and areas. Normally a types is not represented
as all of this types, but just a sub set (for example a road is likely most
of the time a way, never a node and - dependeng on the actual type - sometimes
an area.

Via the OST file you can define which representation a type has.

### Features

Libosmsocut allows the definition of features. Features signal that one or
more tags exists. For example, for the tag `width` there exists a `Width`
feature. Features can have values. The value normally is calculated from one
or more tags.

Using the OST file, you can assign multiple features to a type. In applications
using libosmscout you can later on ask which features a object based on its
type has, which features actually exist on the object and request the value 
of these features.

Technically access to the features of an object are handled by instances of
the class `osmscout::FeatureValueBuffer`.

## Structure

The overall structure of an OSS file is as follows:

```
  MAX SPEEDS
  ...
  GRADES
  ...
  TYPES
  ...
  END
```

In the following sections we describe the content of each part of the OST file.

## Section `MAX SPEEDS`

The max speed section allows you to define max speed constants together with
their value.

OSM allows you to not only assign actual values to the max_speed tag but also
constants like `DE:rural`. The max speed section allows you to give mappings
to actual numerical values for these constants. The import will then
exchange the constants with the matching values during import.

## Section `GRADES`

OSM has two tags for defning the quality of a way: `grade` and `surface`.

## Section `TYPES`

The type section consists of a number of type definitions that match 
tag and tag values onto types.

The overall syntax is:

* Keyword `TYPE` foolowed by the name of the type
* The optional keyword `IGNORE` to makr a type as to be ignored.
* The `=` sign
* A list of types (one or more of `NODE`, `WAY`, `AREA` or `RELATION`) followed
by a boolean condition within `(` ... `)`. This means, if the object is of the
given type it must match the given expression. This allows to pass different
conditions for different objects types.
* The type condition section is optionally followed by a list of features
surrounded by`{` and `}`.
* Followed by a optional list of type options
* Followed again by an optional list of group names introduced by the keyword
`GROUP`.

Some examples:

A building is an area with a `building` tag, where the value of this tag is
neither `no`, `false`, or `0`. Building have a `Name` feature and a `NameAlt`
feature and are addressable.

```
TYPE building
  = AREA (EXISTS "building" AND !("building" IN ["no","false","0"]))
    {Name, NameAlt}
    ADDRESS
```

A "million city" is a node or area with a tag `place` with value `city`,
where a `polutation` value does exist, that has a value greater than 1.000.000.
Such city has a `Name` and a `NameAlt` feature. IT also builds an administrative
region and thus is be evaluated by the location index import step.

```
TYPE place_millioncity
  = NODE AREA ("place"=="city" AND EXISTS "population" AND "population">1000000)
    {Name, NameAlt}
    ADMIN_REGION
```

A administrative boundary is either a way or area with a tag `bondary` with
the value `administrative` or a releation with a additional tag `type`
wit value `boundary`. It has `Name`, `NameAlt` and `AdminLevel` features.
The relation should be handled as multipolygon relation, even if no such tag
exists. Since administrative boundaries can occur on land and on sea they are
no hint, that the region they cover is sea.

```
  TYPE boundary_administrative
    = WAY AREA ("boundary"=="administrative") OR
      RELATION ("type"=="boundary" AND "boundary"=="administrative")
      {Name, NameAlt, AdminLevel}
      MULTIPOLYGON IGNORESEALAND
```

### Conditionals

Besides the usual operators `==`, `!=`, `<`, `<=`, `>`, `>=` where the left
hand side always is a string containg the name of a tag, there are also the
following operators

* `EXISTS <tag name>`
* `<tag name> IN [ <value list> ]`

### Options

Here is a list of existing options together with their meaning:

PATH[[FOOT] [BICYLCE] [CAR]]
:     This way or area is a traversable path and possibly routable for the given vehicles.

      See: http://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access-Restrictions

      If something is a path, the following features are automatically assigned:
      
      * Width
      * Grade
      * Bridge
      * Tunnel
      * Roundabout 
      
      If something is routable, the following features are automatically assigned:
      
      * Access
      * MaxSpeed
      
OPTIMIZE_LOW_ZOOM
: Optimize this area or way for idplaying in low zoom by
  reducing visible complexity
      
PIN_WAY
: This is a way, even if the path is closed

MULTIPOLYGON
: Type should be handled as multipolygon even if type is not set to multipolygon.

ADMIN_REGION
: The given area or node describes an administrate region
  that should be part of the region tree for the city/street
  index
  
POI
:     The given area, way or node is a POI and should be indexed
      by its name in relation to the enclosing region. If something is a POI and has the
      feature "Name"
  
      It automatically get the following features assigned:
  
      * Location
      * Address
    
ADDRESS
:     Objects should be indexed as address
      It automatically get the following features assigned:
  
      * Location
      * Address
  
LOCATION
: The given way or area should be indexed as location.

MERGE_AREAS
: Areas of this type that "touch" each other and the same attribute values
  will get merged.
  
IGNORESEALAND
: Ignore this type for calculation of land masses (because objects of this type
  can occur on sea, too, and thus have no distinguishing character).

