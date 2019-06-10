package com.framstag.osmscout.objecttypes

import com.framstag.osmscout.io.FileScanner
import com.framstag.osmscout.types.TypeId
import java.io.File
import java.lang.IllegalArgumentException
import java.util.*

class TypeConfig {
    companion object {
        @ExperimentalUnsignedTypes
        const val FILE_FORMAT_VERSION = 19u

        @ExperimentalUnsignedTypes
        fun loadFromData(file: File): TypeConfig {
            val scanner = FileScanner(file.inputStream())

            scanner.use {
                val fileFormatVersion = scanner.readUInt32()

                if (fileFormatVersion != FILE_FORMAT_VERSION) {
                    throw IllegalArgumentException("TypeConfig file ${file.name} has file format version $fileFormatVersion, but version $FILE_FORMAT_VERSION is expected")
                }

                val featureCount = scanner.readUInt32Number()

                // TODO Fetch feature

                for (i in 1u..featureCount) {
                    val name = scanner.readString()
                    val descriptionCount = scanner.readUInt32Number()

                    for (f in 1u ..descriptionCount) {
                        val languageCode = scanner.readString()
                        val description  = scanner.readString()

                        // TODO: Add description to feature
                    }
                }

                val typeCount = scanner.readUInt32Number()

                for (i in 1u..typeCount) {
                    val typeInfo = TypeInfo.read(scanner)

                    val featureCount = scanner.readUInt32Number()

                    for (f in 1u..featureCount) {
                        val featureName = scanner.readString()
                    }

                    val groupCount = scanner.readUInt32Number()

                    for (g in 1u..groupCount) {
                        val groupName = scanner.readString()
                    }

                    val descriptionCount = scanner.readUInt32Number()

                    for (d in 1u..descriptionCount) {
                        val languageCode = scanner.readString()
                        val description  = scanner.readString()
                    }
                }

                // TODO: RegisterType(typeInfo)
            }

            return TypeConfig()
        }
    }

    private var wayTypeIdBytes = 1u
    private var types = Vector<TypeInfo>()
    private var nodeTypes = Vector<TypeInfo>()
    private var wayTypes = Vector<TypeInfo>()
    private var areaTypes = Vector<TypeInfo>()

    fun getTypes():Vector<TypeInfo> {
        return types
    }

    fun getNodeTypes():Vector<TypeInfo> {
        return nodeTypes
    }

    fun getWayTypes():Vector<TypeInfo> {
        return wayTypes
    }

    fun getAreaTypes():Vector<TypeInfo> {
        return areaTypes
    }

    fun getTypeCount(): Int {
        return types.size
    }

    fun getWayTypeIdBytes(): UInt {
        return wayTypeIdBytes
    }

    fun registerType(type : TypeInfo):TypeInfo {
        //TODO: Calculate wayTypeIdBytes & Co.

        // TODO: Fix
        return type
    }

    fun getWayTypeInfo(typeId: TypeId): TypeInfo {

        //TODO: Fix
        return TypeInfo("")
    }
}