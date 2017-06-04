---
date: "2017-06-04T15:00:00+02:00"
title: "General Concepts"
description: "Description of some basic concepts of libosmscout"
weight: 7

menu:
  main:
    Parent: "documentation"
    Weight: 7
---

This article tries to explain some general concepts of libosmscout.

## Import

Libosmscout has its own internal representation of OSM data. This is necssary
to offer fast access of the data and reduze the size of the data on disk.
Standard export formats like .osm or *.osm.pbf files are used as data dump
from the OSM SQL database for further processing and are designed  for fast,
index random data access.

For importing the data the `Import` tool is used. The import tool generates
a custom libosmsocut database from the raw data passed.

An import normally is restrictied to a certain area of the world. Normally
you find imports for countries or counties or bigger cities.

Take a look at the [import tutorial]({{< ref "tutorials/Importing.md" >}}) for
a description of how to do an import.

## Database

The import tool creates a custom database from the import data. The database
consists of a number of files. Some files hold data, other files
act as index into data and still other files have index and actual data combined.

You can find a description of the individual database files
[here]({{< ref "documentation/database.md" >}}).

Libomssocut assumes (but does not enforce this) that all maps are stored under
the same directory. Each database has its own sub directory storing the
individual files.

In code each database file is accessed by an individual access class at the
lowest level. These classes normally end in `Index` or `DataFile`.
The `Database` class allows easy to all database files.

## Services

To further hide the details or complexity o f the low level access code a number
of Services have been defined that offer a more suitable API.

Currently the following services exist:

* `MapService`
* `LocationService`
* `RoutingService`
* `POIService`

## BasemapDatabase

The basemap database is a special database that holds world wide information
to be used as an background to the actual data from individual database
instances.

The basemap currently holds the following information:

* World-wide coastlines

## Rendering backend

For actual drawing of maps you need a rendering backend. Since libosmscout is
layerd and modular the actual rendering is decoupled from database access.
Services (in this case `MapService') are designed to be used by different
backends. No special API is enforced. 

The following backends currently exist:

* libagg backend
* cairo backend (with and without pango support)
* Qt backend
* iOS and Mac OS backend
* DirectX backend (proof of concept)
* SVG backend (this backend is in a proof of concept state and is not activly
  maintained)
* OpenGL backend (current in proof of concept but will be rewritten as part of
  the GSoC 2017).
  
You can find more information regarding requirements for a backend 
[here]({{< ref "documentation/renderbackend.md" >}}). A description of the actual
render process can be found [here]({{< ref "documentation/renderprocess.md" >}}).

## Type configration file (*.ost)

The type configuration file maps certain tags onto types. Each object
imported from the import fil(s) will hav a certain type.

Types are later on used to assign further information to the object and
are used as a filter criteria for the style sheet.

You can find more information regarding type configuration
[here]({{< ref "documentation/typedef.md" >}})

## Style sheets (*.oss)

To allow configurable rendering and things like night mode libosmscout
uses style sheets for configuraion of the actual rendering look.

You can find more information regarding style sheets
[here]({{< ref "documentation/stylesheet.md" >}})


