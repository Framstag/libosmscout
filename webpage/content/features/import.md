---
date: "2016-05-29T19:50:12+02:00"
title: "Import"
description: "Detailed list of import features"
weight: 2

menu:
  main:
    Parent: "features"
    Weight: 2
---

## OSM import

<table class="sheet">
<thead>
<tr>
<th style="text-align: left; width: 35%">Feature</th>
<th style="text-align: left">Status</th>
<th style="text-align: left">Comments</th>
</tr>
</thead>
<tbody>

<tr>
<td style="text-align: left">Support nodes</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Support ways</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">Nodes as part of ways are typless. So nodes as part of ways do currently not know of their relation to the holding way.</td>
</tr>

<tr>
<td style="text-align: left">Support areas</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">See comments on ways</td>
</tr>

<tr>
<td style="text-align: left">Support relations</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">We especially support parsing and resolving of multipolygon relations. Ways of same type and with the same values get joined to reduce and simplify relation size.</td>
</tr>

<tr>
<td style="text-align: left">Support blacklisting of nodes, ways, areas by relations</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">Currently relations (besides multipolygon relations) do no drop their data members from the database. Thus a way might be drawn twice. Once as way and once as part of the relation.</td>
</tr>

</tbody>
</table>

## File format features

<table class="sheet">
<thead>
<tr>
<th style="text-align: left; width: 35%">Feature</th>
<th style="text-align: left">Status</th>
<th style="text-align: left">Comments</th>
</tr>
</thead>
<tbody>

<tr>
<td style="text-align: left">Platform independent format</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">We handle byte order, have some optimization for file size (more to do), but file format is not fixed yet.</td>
</tr>

<tr>
<td style="text-align: left">Improved file loading using memory maped files</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

</tbody>
</table>

## Preprocessing

<table class="sheet">
<thead>
<tr>
<th style="text-align: left; width: 35%">Feature</th>
<th style="text-align: left">Status</th>
<th style="text-align: left">Comments</th>
</tr>
</thead>
<tbody>

<tr>
<td style="text-align: left">Evaluating area tag</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">There are still some cases where ways are not detected correctly as way or area</td>
</tr>

<tr>
<td style="text-align: left">Evaluating name tag</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Evaluating ref tag</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Evaluating oneway tag</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Evaluating housenumber tag</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Evaluating width tag</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Evaluating layer tag</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Evaluating bridge and tunnel tag</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Evaluating lanes tag</td>

<td style="text-align: center" class="missing">Missing</td>

<td style="text-align: left">This however should be easy to implement. It should also be easy to extend the style sheet to draw separating lines based on the data.</td>
</tr>

<tr>
<td style="text-align: left">Evaluating junction=roundabout</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">junction=roundabout is evaluated and oneway=true is automatically implied.</td>
</tr>

<tr>
<td style="text-align: left">Support for resolving multipolygon relation</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Merging of adjacent ways with same tag values</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Generating reduced detail version of areas for faster drawing in low zoom</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Generating reduced detail version of ways for faster drawing in low zoom</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Adress lookup</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">We currently evaluate city nodes, city areas and administrative boundaries to build a hierachical index of named regions down to streets.</td>
</tr>

<tr>
<td style="text-align: left">Adress lookup</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">We currently evaluate city nodes, city areas and administrative boundaries to build a hierachical index of named regions down to streets.</td>
</tr>

<tr>
<td style="text-align: left">Support for restriction relation</td>

<td style="text-align: center" class="initial">Initial</td>

<td style="text-align: left">Restriction relations will be evaluated as part of the routing algorithm, we currently have no support for drawing restriction information.</td>
</tr>

<tr>
<td style="text-align: left">Handle support for water areas, oceans and similar.</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">We can render coastlines and water areas on any zoom level. There are though some visual artefacts on low zoom if coastline data is not complete</td>
</tr>

<tr>
<td style="text-align: left">Handle support for world wide country borders.</td>

<td style="text-align: center" class="missing">Missing</td>

<td style="text-align: left">We would need to choose a similar aproach as with coastline detection.</td>
</tr>

</tbody>
</table>

