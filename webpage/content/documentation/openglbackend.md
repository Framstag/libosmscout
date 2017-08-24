---
date: "2017-08-04T19:48:00+02:00"
title: "OpenGL backend"
description: "Description of the inner workings of the OpenGL backend"
weight: 13

menu:
  main:
    Parent: "documentation"
    Weight: 13
---

The following chapters give an overview over the inner workings
of the OpenGL backend. It gives you some information about how 
the backend works and how you can integrate it into your app.


# How to use the backend

The MapPainterOpenGL class is the part of the backend that offers the interface to the data processing and the drawing calls.
In order to incorporate the backend in an application one would need to follow these steps:

1. Create an OpenGl context with some library. OSMScoutOpenGL demo uses GLFW, but one can use other libraries as well
(for example SDL, Qt).
2. ProcessData() function processes the map data and converts it to
the format required by the OpenGL pipeline.
3. SwapData() function loads the processed data into the buffers, so it can be rendered
by OpenGL.
4. DrawMap() function draws the map data to the OpenGL context.
See OSMScoutOpenGL demo for reference.

## Dependencies
The OpenGL backend needs the following libraries in order to function correctly:

* glew
* FreeType
* glm
OSMScoutOpenGL demo:
* GLFW

# Rendering process

## Areas and ground
The MapData contains the areas as sequence of coordinates. It gives the outline of a given area. Because OpenGL operates
with triangles (and cannot render more complex shapes), the backend has to triangulate the area first.
The backend uses poly2tri library for triangulation. The ground rendering works just like area rendering.
Link: [poly2tri] (https://github.com/greenm01/poly2tri)

## Ways
A way is given as a sequence of coordinates. The OpenGL backend renders them as a sequence of quads, which is
consists of two triangles.

You can read about the math behind the way rendering here. The backend does not work the exact same as described here,
 but similar.
 
* [Thick lines using geometry shader] (https://forum.libcinder.org/topic/smooth-thick-lines-using-geometry-shader)
* [Drawing lines in OpenGL] (https://mattdesl.svbtle.com/drawing-lines-is-hard)

## Symbols
Symbols are rendered the same as areas.

## Text
The OpenGL backend uses FreeType library for loading fonts. The backend creates a so called texture atlas,
which is one bitmap that contains all characters the backend has found during the processing of labels.
When it encounters a "new" character, it loads the character glyph with FreeType, generates a bitmap,
and add the bitmap to the texture atlas. After that, the backend renders a text label character by character,
as textured quads.
Link: [FreeType] (https://www.freetype.org/)

## Icons
Icons are fixed sized (14x14 pixel) images. The backend renders them as a 10x10 px quad with texture. It also uses
a texture atlas.

## After processing
After the backend processes the given data and converts it to suitable format, the SwapData function loads the processed,
and triangulated geographic data in to the buffers as required by OpenGL. After that, these vertices being processed by
shader programs. Shaders are small programs that runs on the graphics processor, and they do the transformations
(like Mercator projection, and world-to-screen projection), and fragment manipulation (anti-aliasing, coloring).
You can read more about the OpenGL rendering pipeline here:

* [OpenGL rendering pipeline] (https://www.khronos.org/opengl/wiki/Rendering_Pipeline_Overview)
And about the shaders here:
* [Shaders] (https://www.khronos.org/opengl/wiki/Shader)
