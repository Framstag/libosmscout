package com.framstag.osmscout.objecttypes

import com.framstag.osmscout.io.FileScanner

@ExperimentalUnsignedTypes
class TypeInfo(val name: String) {
    companion object {

        fun read(scanner : FileScanner): TypeInfo {
            val name: String

            name = scanner.readString()

            val typeInfo = TypeInfo(name)

            typeInfo.canBeNode = scanner.readBool()
            typeInfo.canBeWay = scanner.readBool()
            typeInfo.canBeArea = scanner.readBool()
            typeInfo.canBeRelation = scanner.readBool()
            typeInfo.isPath = scanner.readBool()
            typeInfo.canRouteFoot = scanner.readBool()
            typeInfo.canRouteBicycle = scanner.readBool()
            typeInfo.canRouteCar = scanner.readBool()
            typeInfo.indexAsAddress = scanner.readBool()
            typeInfo.indexAsLocation = scanner.readBool()
            typeInfo.indexAsRegion = scanner.readBool()
            typeInfo.indexAsPOI = scanner.readBool()
            typeInfo.optimizeLowZoom = scanner.readBool()
            typeInfo.multipolygon = scanner.readBool()
            typeInfo.pinWay = scanner.readBool()
            typeInfo.mergeAreas = scanner.readBool()
            typeInfo.ignoreSeaLand = scanner.readBool()
            typeInfo.ignore = scanner.readBool()
            typeInfo.lanes = scanner.readUInt8()
            typeInfo.onewayLanes = scanner.readUInt8()

            return typeInfo
        }
    }

    var index = 0

    var canBeNode = false
    var canBeWay = false
    var canBeArea = false
    var canBeRelation = false
    var isPath = false
    var canRouteFoot = false
    var canRouteBicycle = false
    var canRouteCar = false
    var indexAsAddress = false
    var indexAsLocation = false
    var indexAsRegion = false
    var indexAsPOI = false
    var optimizeLowZoom = false
    var multipolygon = false
    var pinWay = false
    var mergeAreas = false
    var ignore = false
    var ignoreSeaLand = false
    var lanes : UByte = 1u
    var onewayLanes : UByte = 1u
}