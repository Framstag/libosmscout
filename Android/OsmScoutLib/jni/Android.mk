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

LOCAL_PATH := $(call my-dir)

##########################################################
# Static library of the OsmScout library for ARM

include $(CLEAR_VARS)

LOCAL_MODULE := osmscout-arm

LOCAL_C_INCLUDES := ../../../libosmscout/include \
                    ../../../libosmscout-map/include \
                    $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/include \
                    $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/libs/armeabi/include

LOCAL_CFLAGS := -std=gnu++0x

LOCAL_SRC_FILES := ../../../libosmscout/src/osmscout/AreaAreaIndex.cpp \
                   ../../../libosmscout/src/osmscout/AreaNodeIndex.cpp \
                   ../../../libosmscout/src/osmscout/AreaWayIndex.cpp \
                   ../../../libosmscout/src/osmscout/CityStreetIndex.cpp \
                   ../../../libosmscout/src/osmscout/Database.cpp \
                   ../../../libosmscout/src/osmscout/GroundTile.cpp \
                   ../../../libosmscout/src/osmscout/Location.cpp \
                   ../../../libosmscout/src/osmscout/Node.cpp \
                   ../../../libosmscout/src/osmscout/OptimizeLowZoom.cpp \
                   ../../../libosmscout/src/osmscout/Relation.cpp \
                   ../../../libosmscout/src/osmscout/SegmentAttributes.cpp \
                   ../../../libosmscout/src/osmscout/TypeConfig.cpp \
                   ../../../libosmscout/src/osmscout/TypeConfigLoader.cpp \
                   ../../../libosmscout/src/osmscout/Types.cpp \
                   ../../../libosmscout/src/osmscout/WaterIndex.cpp \
                   ../../../libosmscout/src/osmscout/Way.cpp \
                   ../../../libosmscout/src/osmscout/ost/Parser.cpp \
                   ../../../libosmscout/src/osmscout/ost/Scanner.cpp \
                   ../../../libosmscout/src/osmscout/util/Color.cpp \
                   ../../../libosmscout/src/osmscout/util/File.cpp \
                   ../../../libosmscout/src/osmscout/util/FileScanner.cpp \
                   ../../../libosmscout/src/osmscout/util/FileWriter.cpp \
                   ../../../libosmscout/src/osmscout/util/Geometry.cpp \
                   ../../../libosmscout/src/osmscout/util/Projection.cpp \
                   ../../../libosmscout/src/osmscout/util/StopClock.cpp \
                   ../../../libosmscout/src/osmscout/util/String.cpp \
                   ../../../libosmscout/src/osmscout/util/Transformation.cpp \
                   ../../../libosmscout-map/src/osmscout/MapPainter.cpp \
                   ../../../libosmscout-map/src/osmscout/StyleConfig.cpp \
                   ../../../libosmscout-map/src/osmscout/StyleConfigLoader.cpp \
                   ../../../libosmscout-map/src/osmscout/oss/Parser.cpp \
                   ../../../libosmscout-map/src/osmscout/oss/Scanner.cpp

LOCAL_LDLIBS := -L$(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/libs/armeabi/ \
                -lstdc++ -lgnustl_shared -llog

include $(BUILD_STATIC_LIBRARY)

##########################################################
# Shared library for the JNI of the osmscout library
#
include $(CLEAR_VARS)

LOCAL_MODULE := osmscout-jni

LOCAL_C_INCLUDES := ./include \
                    ../../../libosmscout/include \
                    ../../../libosmscout-map/include \
                    $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/include \
                    $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/libs/armeabi/include
					
LOCAL_CFLAGS := -std=gnu++0x

LOCAL_LDLIBS := -L$(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/libs/armeabi/ \
                -L$(ANDROID_NDK_PATH)/platforms/android-5/arch-arm/usr/lib \
                -lstdc++ -lgnustl_shared -llog

LOCAL_STATIC_LIBRARIES := osmscout-arm

LOCAL_SRC_FILES := src/jniDatabase.cpp \
                   src/jniMapData.cpp \
                   src/jniMapPainterCanvas.cpp \
                   src/jniMercatorProjection.cpp \
                   src/jniObjectTypeSets.cpp \
                   src/jniOnLoad.cpp \
                   src/jniStyleConfig.cpp \
                   src/jniTypeConfig.cpp

include $(BUILD_SHARED_LIBRARY)

