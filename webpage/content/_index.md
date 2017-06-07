+++
date = "2017-04-22T20:19:32+01:00"
title = "About"
weight = 1
+++

libosmscout offers applications simple, high-level interfaces for __offline__
__location and POI lokup__, __rendering__ and __routing__ functionalities based
on OpenStreetMap (OSM) data.

It offers you much of the functionality you need, to implement a offline-capable
routing application (or similar) based on OSM data.

This means:

* It offers a tool to process OSM data (*.osm and *.osm.bf files), transform them
and dump them into a custom database - which is the sole datasource for all
offline operations
* You can control in detail which data is actually stored into the database,
thus making it contain only the data you actually need
* It implements a fast vector map renderer based on the content from above
database
* It offers you a powerful style sheet to control the look of the resulting map
* It offers a profile-based router, allowing you to route for various and
multiple vehicles
* It generates a detailed route description for the calculated route
* It allows you to lookup geo coordinates based on a address description
* It allows you to reverse lookup location descriptions base on geo coordinates
* All is done using layers APIs. You can use high level APIs to get the task done,
but can also use lower level APIs to have more control

The goals of libosmscout are:

* Enable OpenStreetMap and its rich data set
* Implement all feature required for a navigation app or application that
  want to use some subset of such features.
* Work on low-end hardware like it is commonly used for handhelds, phones and
  similar.
* Offer compact data storage making it possible to place data for countries
  and up to continents on SD cards of common size.

