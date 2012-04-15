/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

package osm.scout;

public class AdminRegion {
	
  public ObjectRef        reference; //! Reference to the object defining the region
  //long                  offset;    //! Offset into the region datafile
  public String           name;      //! name of the region
  //Vector<String>        path;      //! Admin region names higher up in the hierarchy

  public AdminRegion(String name, int refType, long id) {
		
    this.name=name;
    reference=new ObjectRef(refType, id);		
  }

  public String toString() {

    return name;
  }

}

