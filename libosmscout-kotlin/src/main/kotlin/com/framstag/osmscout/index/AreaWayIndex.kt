package com.framstag.osmscout.index

import com.framstag.osmscout.objecttypes.TypeConfig
import com.framstag.osmscout.geometry.GeoBox
import com.framstag.osmscout.io.FileOffset
import com.framstag.osmscout.io.FileScanner
import com.framstag.osmscout.objecttypes.TypeInfo
import com.framstag.osmscout.objecttypes.TypeInfoSet
import java.io.File
import java.util.*
import kotlin.math.pow

/**
 * Returns ways in a given area
 */
@ExperimentalUnsignedTypes
class AreaWayIndex {
    data class TypeData constructor(
        val type: TypeInfo,
        val bitmapOffset: FileOffset,
        val indexLevel: UInt = 0u,
        val dataOffsetBytes: UByte = 0u,
        val cellXStart: UInt = 0u,
        val cellXEnd: UInt = 0u,
        val cellYStart: UInt = 0u,
        val cellYEnd: UInt = 0u,
        val cellXCount: UInt = 0u,
        val cellYCount: UInt = 0u,
        val cellWidth: Double = 0.0,
        val cellHeight: Double = 0.0,
        val minLon: Double = 0.0,
        val maxLon: Double = 0.0,
        val minLat: Double = 0.0,
        val maxLat: Double = 0.0
    )

    private var scanner: FileScanner? = null
    private val typeData = Vector<TypeData>()
    private val wayTypeData = Vector<TypeData>()

    fun open(
        typeConfig: TypeConfig,
        file: File
    ) {
        scanner = FileScanner(file.inputStream())

        scanner?.let { scanner ->
            val indexEntries = scanner.readUInt32()

            typeData.ensureCapacity(indexEntries.toInt())

            for (t in 0u..indexEntries - 1u) {
                val typeId = scanner.readTypeId(typeConfig.getWayTypeIdBytes())

                val type = typeConfig.getWayTypeInfo(typeId)
                val bitmapOffset = scanner.readFileOffset()

                val typeData: TypeData

                if (bitmapOffset > 0u) {
                    val dataOffsetBytes = scanner.readUInt8()

                    val indexLevel = scanner.readUInt32()

                    val cellXStart = scanner.readUInt32Number()
                    val cellXEnd = scanner.readUInt32Number()
                    val cellYStart = scanner.readUInt32Number()
                    val cellYEnd = scanner.readUInt32Number()

                    val cellWidth = 360.0 / 2.0.pow(indexLevel.toInt())
                    val cellHeight = 18.0 / 2.0.pow(indexLevel.toInt())

                    val minLon = cellXStart.toDouble() * cellWidth - 180.0
                    val maxLon = (cellXEnd + 1u).toDouble() * cellWidth - 180.0

                    val minLat = cellYStart.toDouble() * cellHeight - 90.0
                    val maxLat = (cellYEnd + 1u).toDouble() * cellHeight - 90.0

                    typeData = TypeData(
                        type, bitmapOffset, indexLevel, dataOffsetBytes,
                        cellXStart, cellXEnd, cellYStart, cellYEnd,
                        cellXEnd - cellXStart + 1u,
                        cellYEnd - cellYStart + 1u,
                        cellWidth,
                        cellHeight,
                        minLon,
                        maxLon,
                        minLat,
                        maxLat
                    )
                } else {
                    typeData = TypeData(type, bitmapOffset)
                }

                wayTypeData.add(typeData)
            }
        }
    }

    fun close() {
        scanner?.apply {
            close()
        }
    }

    private fun GetOffsets(
        typeData: TypeData,
        boundingBox: GeoBox,
        offsets: Set<FileOffset>
    ) {

    }

    fun getOffsets(
        boundingBox: GeoBox,
        types: TypeInfoSet,
        offsets: Vector<FileOffset>,
        loadedTypes: TypeInfoSet
    ): Boolean {
        // TODO: Reserve capacity for offsets
        loadedTypes.clear()

        return true
    }

}