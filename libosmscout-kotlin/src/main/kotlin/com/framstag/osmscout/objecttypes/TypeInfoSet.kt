package com.framstag.osmscout.objecttypes

import java.util.*

class TypeInfoSet {
    @ExperimentalUnsignedTypes
    private val types = Vector<TypeInfo>()
    private var count = 0

    constructor(typeConfig: TypeConfig) {
        types.setSize(typeConfig.getTypeCount())
    }

    fun set(typeInfo: TypeInfo) {
        if (types.size<=typeInfo.index) {
            types.setSize(typeInfo.index+1)
        }

        if (types[typeInfo.index] == null) {
            types[typeInfo.index] = typeInfo
            count++
        }
    }

    fun set(types: Collection<TypeInfo>) {
        clear()

        types.forEach {
            set(it)
        }
    }

    /*
    fun add(types: TypeInfoSet) {
        clear()

        types.forEach {
            set(it)
        }
    }*/

    fun count():Int {
        return count
    }

    fun clear() {
        types.removeAllElements()
        count = 0
    }
}