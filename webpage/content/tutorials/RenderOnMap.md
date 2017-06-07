---
date: "2016-09-25T17:21:58+02:00"
title:  "Render on top of the map"
description: "How to render on top of the map"
weight: 5

menu:
  main:
    Parent: "tutorials"
    Weight: 5
---

## Goal of the tutorial

This tutorial describes how you can render on top of the map. How to

* Show special ways like the current route
* Add current position mark
* Popups and detail information
* Scale
* Something with animations
* Highlight selected POIs

## Solution 1: Inject custom objects 

If you just want to inject additional map primitives (nodes, ways, areas)
that are not part of the original OSM data returned by the Database and want to
use the existing style hseet engine, you can inject your own objects.

If you want them to be drawn differently, just define your own type, assign this
type to the objects and assign custom rendering via the style sheet.

To create your own type, just create a `TypeInfo` instance, make sure you mark
the type as internal and add it to the `TypeConfig`, here is an slightly
modified example from `TypeConfig.cpp` itself, registering a custom type used
for rendering the current route:

```c++
TypeInfoRef route=std::make_shared<TypeInfo>("_route");

route->SetInternal().CanBeWay(true);

typeConfig->RegisterType(route);
```

Make sure, that the types are registered to the TypeConfig for the importer
and for the rendering engine in the same order (they get a running number
internally).

Later on you can create (in this case) a way, assigning the route type to it:

```c++
TypeConfigRef typeConfig=database->GetTypeConfig();
Way           way;

if (!typeConfig) {
  return false;
}

TypeInfoRef routeType=typeConfig->GetTypeInfo("_route");

assert(routeType!=typeConfig->typeInfoIgnore);

way.SetType(routeType);
way.SetLayerToMax();
```

Make sure that you fill way.nodes afterwards with valid data to build the actual
path. You need to call `SetLayerToMax()` to make sure that this way is drawn
on top of all other ways.

In the next step you can now inject this way into the `MapData`:

```c++
data.poiWays.push_back(way);
```
Finally make sure that you have some style definition attached:

```
[MAG world-] {
  [TYPE _route] WAY {color: @routeColor; displayWidth: 1.5mm; priority: 100; }
}
```

## Solution 2: Just draw it

If you need more power the solution is to use the rendering engine of the backend
itself. If you are e.g. using the Qt backend, you pass a `QPainter*` ot the
painter and your `MapPainterQt` instance draws the map into passed painter.
After the map has been rendered you can use the Qt graphics routines like
`painter.setPen()` and `painter.fillRect()` to draw on top of the map.

This makes sense for drawing complex things like

* scales
* Popups
* Current position mark
