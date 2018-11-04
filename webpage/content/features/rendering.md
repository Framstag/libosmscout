---
date: "2016-05-29T19:50:12+02:00"
title: "Map rendering"
description: "Detailed list of rendering features"
weight: 3

menu:
  main:
    Parent: "features"
    Weight: 3
---

## Backends

<table class="sheet">

<thead>
<tr>
<th style="text-align: left; width: 45%" class="bottom">Backend</th>
<th style="text-align: left; width: 5%" class="vertical bottom">Ways</th>
<th style="text-align: left; width: 5%" class="vertical bottom">Dashed ways</th>
<th style="text-align: left; width: 5%" class="vertical bottom">Areas</th>
<th style="text-align: left; width: 5%" class="vertical bottom">Area with image pattern</th>
<th style="text-align: left; width: 5%" class="vertical bottom">POI symbols</th>
<th style="text-align: left; width: 5%" class="vertical bottom">POI icons</th>
<th style="text-align: left; width: 5%" class="vertical bottom">Labels</th>
<th style="text-align: left; width: 5%" class="vertical bottom">Rith-to-left labels</th>
<th style="text-align: left; width: 5%" class="vertical bottom">Contour labels</th>
</tr>
</thead>
<tbody>

<tr>
<td style="text-align: left">Qt</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
</tr>

<tr>
<td style="text-align: left">Cairo</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓<sup>1</sup></td>
<td style="text-align: center" class="ok">✓</td>
</tr>

<tr>
<td style="text-align: left">AGG</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="missing">✗</td> 
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="missing">✗</td> 
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="missing">✗</td> 
<td style="text-align: center" class="ok">✓</td>
</tr>

<tr>
<td style="text-align: left">OpenGL</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="missing">✗</td> 
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="missing">✗</td> 
<td style="text-align: center" class="missing">✗</td> 
<td style="text-align: center" class="missing">✗</td> 
<td style="text-align: center" class="initial">✓<sup>4</sup></td>
<td style="text-align: center" class="missing">✗</td> 
<td style="text-align: center" class="missing">✗</td> 
</tr>

<tr>
<td style="text-align: left">DirectX</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="missing">✗</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td> 
<td style="text-align: center" class="initial">✓<sup>3</sup></td>
<td style="text-align: center" class="missing">✗</td> 
<td style="text-align: center" class="initial">✓<sup>3</sup></td>
</tr>

<tr>
<td style="text-align: left">MacOS / iOS</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="initial">✓<sup>2</sup></td>
<td style="text-align: center" class="initial">✓<sup>2</sup></td>
<td style="text-align: center" class="initial">✓<sup>2</sup></td>
</tr>

<tr>
<td style="text-align: left">SVG</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="missing">✗</td> 
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓</td>
<td style="text-align: center" class="ok">✓<sup>1</sup></td>
<td style="text-align: center" class="ok">✓</td>
</tr>
</tbody>
</table>

<div><sup>1)</sup> With Pango library</div>
<div><sup>2)</sup> Transition to new label layouter is not finished yet</div>
<div><sup>3)</sup> Problems with non-ascii characters</div>
<div><sup>4)</sup> Buggy on some platforms</div>

## Map drawing

<table class="sheet">
<thead>
<tr>
<th style="text-align: left; width: 45%">Feature</th>
<th style="text-align: left; width: 5%">Status</th>
<th style="text-align: left; width: 50%">Comments</th>
</tr>
</thead>
<tbody>

<tr>
<td style="text-align: left">Style sheet support</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Draw nodes</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Draw ways</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Draw multiple lines for way</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">Render multiple lines with different styles for one way</td>
</tr>

<tr>
<td style="text-align: left">Draw labels on top of way</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">E.g. render street names</td>
</tr>

<tr>
<td style="text-align: left">Draw plates on top of way</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">E.g. render motorway numbers</td>
</tr>

<tr>
<td style="text-align: left">Draw areas</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Handle layers</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Style sheet filter for bridge and tunnel</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Draw symbols</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">Simple API to draw geometrical objects like arrows, boxes, crosses</td>
</tr>

<tr>
<td style="text-align: left">Draw external images</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">We support loading and caching of PNGs for the cairo and Qt backend. We should support for formats.</td>
</tr>

<tr>
<td style="text-align: left">Define font</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">Current one font type support, style sheet allows to use different font sizes</td>
</tr>

<tr>
<td style="text-align: left">Render depending on zoom level</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">Style sheet allows filtering based on zoom level</td>
</tr>

<tr>
<td style="text-align: left">Advanced language support</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">UTF-8 is support, advanced text rendering used if supported by backend</td>
</tr>

<tr>
<td style="text-align: left">Label priority</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">More important labels are render befor less important ones</td>
</tr>

<tr>
<td style="text-align: left">Merge labels</td>

<td style="text-align: center" class="initial">Initial</td>

<td style="text-align: left">Improved handling of closed by labels with same value, e.g. two parallel lanes for the same highway</td>
</tr>

<tr>
<td style="text-align: left">Reposition labels</td>

<td style="text-align: center" class="missing">Missing</td>

<td style="text-align: left">Move labels slightly for improved appearance</td>
</tr>

<tr>
<td style="text-align: left">Draw turn restrictions</td>

<td style="text-align: center" class="missing">Missing</td>

<td style="text-align: left"></td>
</tr>

</tbody>
</table>

