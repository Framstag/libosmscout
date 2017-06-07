---
date: "2016-08-10T19:48:00+02:00"
title: "Render backend requirements"
description: "Requires to full for render backends"
weight: 11

menu:
  main:
    Parent: "documentation"
    Weight: 11
---

The following gives you a list of functionalities a render backend must
be able to implement (normally this results in feature requirements for the
underlying OS/ drawing engine).

The sum of all features is best summerized by

> "You need a modern 2D vector graphics library"

Note that while this is a list of all features required to be implemented you
can still to implement a backend does does not implement all of this. You must
then remove all style sheet options that trigger such functionality or must live
with your own fallbacks.

## Coordinates

* All object coordinates must suport floating point (double) granularity
(vector drawing). Tis is the same for all other spacings, lengths, widths and
similar. vector drawing also requires some anti alias handling, too.

## Colors

* We need to be able to assign any RGB color to primitives like lines and areas 
and texts.
* We also need to support alpha channel for any such objects.

## Lines

* We need to be able to draw lines of any thickness. Lines may consist on any
number of coordinates.
* Draw lines with gaps. You must be able to define the length of the repeating
gaps.

## Areas

* We need to be able to draw any closed area (concave and convex) using
polygons of any number of coordinates.
* We need to be able to punch holes into a rendered polygon by defining any number
of polygons as holes.
* The defnition of areas and holes must not enforce any orientation of the
polygon.

## Text

* We need to be able to render text using at least one font, supporting any
UTF character.
* We must support any font size.
* UTF-8 encoding must be supported (or transformable to the target format).
* We do need to get the height of a given font in pixel.
* We need to receive dimensions of text in pixel.
* We need to be able to fit texts to any polygon curve by either transforming
the text or at least rotating and repositioning of individual glyphs.

## SVG and image support

* For supporting overlay icons, symbols and images it is helpful, if the 
library can load such formats and be able to render them at a given point.

# Using OpenGL or other 3D engines as backend

To make use of complex lines and areas you need to tesselate these into
(sequences of) triangles. Also font handling requires support code.

Thus you either need a bunch of helper code to get tesselation working or use
a third party helper library that does this for you. Note that even a line
is not just a list of rectangles consisting of two triangles (think of line
cap!).

Also there exist a number of OpenGL variants and versions and some extensions.
To handle all these correctly, suggests using a wrapper library that handles all
this for you, too.

Also to make win from the OpenGL performance you should not render the same map
again and again but try to add tiles of data to the OpenGL scene once they
appear and remove them once they disapear from the view point.

Using Vulcan situation will be even more complex, since it looks like, the code
to visual effect ration in Vulcan is even worse than in OpenGL.

# Possible alternative backends

This is a list of possible alternative gui libraries, frameworks etc...
that could be base for new, alternative backends. There was not yet an in
deep analysis, if these libraries fit all criteria, nor implies this
list, that there is an actual plan.

* [Magnum](http://mosra.cz/blog/magnum-doc/)
* [Visualization Library](http://www.visualizationlibrary.org/documentation/pag_learning.html)
* [nanovg](https://github.com/memononen/nanovg)
* [Skia](https://skia.org/)
* [Blend2D](http://blend2d.com/)
