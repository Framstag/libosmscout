/*
 This source is part of the libosmscout library
 Copyright (C) 2014  Tim Teulings, Vladimir Vyskocil
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include "osmscout/Navigation.h"

namespace osmscout {
    
    double distanceToSegment(const GeoCoord &p, const GeoCoord &p1, const GeoCoord &p2, double &r){
        double px = p.GetLon();
        double py = p.GetLat();
        double p1x = p1.GetLon();
        double p1y = p1.GetLat();
        double p2x = p2.GetLon();
        double p2y = p2.GetLat();
        
        if(p1x == p2x && p1y == p2y){
            return NAN;
        }
        
        //
        // find the distance from the point (cx,cy) to the line
        // determined by the points (ax,ay) and (bx,by)
        //
        // distanceSegment = distance from the point to the line segment
        // distanceLine = distance from the point to the line (assuming
        //                                        infinite extent in both directions
        //
        
        /*
         
         How do I find the distance from a point to a line?
         
         
         Let the point be C (Cx,Cy) and the line be AB (Ax,Ay) to (Bx,By).
         Let P be the point of perpendicular projection of C on AB.  The parameter
         r, which indicates P's position along AB, is computed by the dot product
         of AC and AB divided by the square of the length of AB:
         
         AC dot AB
         (1) r = ---------
         ||AB||^2
         
         r has the following meaning:
         
         r=0      P = A
         r=1      P = B
         r<0      P is on the backward extension of AB
         r>1      P is on the forward extension of AB
         0<r<1    P is interior to AB
         
         The length of a line segment AB is computed by:
         
         L = sqrt( (Bx-Ax)^2 + (By-Ay)^2 )
         
         and the dot product of two vectors U dot V is computed:
         
         D = (Ux * Vx) + (Uy * Vy)
         
         So (1) expands to:
         
         (Cx-Ax)(Bx-Ax) + (Cy-Ay)(By-Ay)
         r = -------------------------------
         L^2
         
         The point P can then be found:
         
         Px = Ax + r(Bx-Ax)
         Py = Ay + r(By-Ay)
         
         And the distance from A to P = r*L.
         
         Use another parameter s to indicate the location along PC, with the
         following meaning:
         s<0      C is left of AB
         s>0      C is right of AB
         s=0      C is on AB
         
         Compute s as follows:
         
         (Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)
         s = -----------------------------
         L^2
         
         
         Then the distance from C to P = |s|*L.
         
         */
        
        
        double rn = (px-p1x)*(p2x-p1x) + (py-p1y)*(p2y-p1y);
        double rd = (p2x-p1x)*(p2x-p1x) + (p2y-p1y)*(p2y-p1y);
        r = rn / rd;
        double ppx = p1x + r*(p2x-p1x);
        double ppy = p1y + r*(p2y-p1y);
        double s =  ((p1y-py)*(p2x-p1x)-(p1x-px)*(p2y-p1y)) / rd;
        
        //
        // (xx,yy) is the point on the lineSegment closest to (px,py)
        //
        double xx = ppx;
        double yy = ppy;
        
        if ((r >= 0) && (r <= 1))
        {
            return fabs(s)*sqrt(rd);
        }
        else
        {
            double dist1 = (px-p1x)*(px-p1x) + (py-p1y)*(py-p1y);
            double dist2 = (px-p2x)*(px-p2x) + (py-p2y)*(py-p2y);
            if (dist1 < dist2)
            {
                xx = p1x;
                yy = p1y;
                return sqrt(dist1);
            }
            else
            {
                xx = p2x;
                yy = p2y;
                return sqrt(dist2);
            }
        }
    }
}


