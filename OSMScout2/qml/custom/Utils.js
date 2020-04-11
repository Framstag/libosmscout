/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2020  Lukas Karas

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

.pragma library

function formatDegree(degree){
    var minutes = (degree - Math.floor(degree)) * 60;
    var seconds = (minutes - Math.floor(minutes )) * 60;
    return Math.floor(degree) + "°"
        + (minutes<10?"0":"") + Math.floor(minutes) + "'"
        + (seconds<10?"0":"") + seconds.toFixed(2) + "\"";
}

function formatCoord(lat, lon){
    return formatDegree( Math.abs(lat) ) + (lat>0? "N":"S") + " " + formatDegree( Math.abs(lon) ) + (lon>0? "E":"W");
}

function locationStr(location){
    if (location==null){
        return "";
    }
    return (location.label=="" || location.type=="coordinate") ?
                formatCoord(location.lat, location.lon) :
                location.label;
}
