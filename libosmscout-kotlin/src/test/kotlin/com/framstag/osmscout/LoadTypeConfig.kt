package com.framstag.osmscout

import com.framstag.osmscout.objecttypes.TypeConfig
import java.io.File

fun main(args: Array<String>) {
    TypeConfig.loadFromData(File(args[0]))
}