---
date: "2016-05-29T19:50:12+02:00"
title: "Routing"
description: "Detailed list of routing features"
weight: 4

menu:
  main:
    Parent: "features"
    Weight: 4
---

## Routing

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
<td style="text-align: left">Route by shortest way</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Route by fasted way</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Support routing profiles</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">Routing profiles allow to define custom costs for individual way segments and areas. Provided implementations allow calculation of shortest and fastest route.</td>
</tr>

<tr>
<td style="text-align: left">Routing profile supports custom max speed per way type</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">You can define your own max speed values for each way type.</td>
</tr>

<tr>
<td style="text-align: left">Routing profile supports setting max speed of vehicle</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">You can define your own max speed for the vehicle you use.</td>
</tr>

<tr>
<td style="text-align: left">Take account of oneways</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Take account of roundabouts</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">junction=roundabout implies oneway=yes. Closed ways as build by roundabouts are correctly handled.</td>
</tr>

<tr>
<td style="text-align: left">Take account of turn restrictions</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">We support positive and negative turn restrictions with one or multiple via nodes.</td>
</tr>

<tr>
<td style="text-align: left">Take account of traffic light waiting time</td>

<td style="text-align: center" class="missing">Missing</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Take account of stop street waiting time</td>

<td style="text-align: center" class="missing">Missing</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Take account of general left turn waiting time</td>

<td style="text-align: center" class="missing">Missing</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Take account of speed limits on certain streets</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">We evaluate numeric values and profile constants for the maxspeed attribute</td>
</tr>

<tr>
<td style="text-align: left">Take account of street quality speed restrictions</td>

<td style="text-align: center" class="missing">Missing</td>

<td style="text-align: left">Traffic calming of various kinds.</td>
</tr>

<tr>
<td style="text-align: left">Support of foot routing</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Support of bicycle routing</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Support of car routing</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Support of motorbike routing</td>

<td style="text-align: center" class="missing">Missing</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Support of horse routing</td>

<td style="text-align: center" class="missing">Missing</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Support of wheelchair routing</td>

<td style="text-align: center" class="missing">Missing</td>

<td style="text-align: left"></td>
</tr>

<tr>
<td style="text-align: left">Time based routing</td>

<td style="text-align: center" class="missing">Missing</td>

<td style="text-align: left">We do not yet take into account time based access restrictions.</td>
</tr>

<tr>
<td style="text-align: left">Display route im map</td>

<td style="text-align: center" class="ok">OK</td>

<td style="text-align: left">A route is just an special (internal) type. All style sheet options are allowed</td>
</tr>

<tr>
<td style="text-align: left">Calculate routes to multiple targets at the same time</td>

<td style="text-align: center" class="missing">Missing</td>

<td style="text-align: left"></td>
</tr>

</tbody>
</table>

