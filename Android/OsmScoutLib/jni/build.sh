###################################################################
#
#  This source is part of the libosmscout library
#  Copyright (C) 2010  Tim Teulings
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
#

###################################################################
# Check for the "Config.h" header file
SRC_FILE=include/config/Config.h
DST_FILE=../../../libosmscout/include/osmscout/private/Config.h

diff --brief $SRC_FILE $DST_FILE

if [ $? -ne 0 ];
then
   echo "Copying Config.h..."
   cp $SRC_FILE $DST_FILE
fi

###################################################################
# Check for the "CoreFeatures.h" header file
SRC_FILE=include/config/CoreFeatures.h
DST_FILE=../../../libosmscout/include/osmscout/CoreFeatures.h

diff --brief $SRC_FILE $DST_FILE

if [ $? -ne 0 ];
then
   echo "Copying CoreFeatures.h..."
   cp $SRC_FILE $DST_FILE
fi

###################################################################
# Check for the "MapFeatures.h" header file
SRC_FILE=include/config/MapFeatures.h
DST_FILE=../../../libosmscout-map/include/osmscout/MapFeatures.h

diff --brief $SRC_FILE $DST_FILE

if [ $? -ne 0 ];
then
   echo "Copying MapFeatures.h..."
   cp $SRC_FILE $DST_FILE
fi

###################################################################
# Run the NDK build tool
ndk-build

###################################################################
# Copy the GNU STL shared library to the project lib directory
SRC_FILE=$ANDROID_NDK_PATH/sources/cxx-stl/gnu-libstdc++/libs/armeabi/libgnustl_shared.so
DST_FILE=../libs/armeabi/libgnustl_shared.so

echo "Copying the GNU STL shared library..."
cp $SRC_FILE $DST_FILE


