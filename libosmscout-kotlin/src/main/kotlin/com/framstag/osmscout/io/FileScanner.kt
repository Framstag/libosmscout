package com.framstag.osmscout.io

import com.framstag.osmscout.types.TypeId
import java.io.ByteArrayOutputStream
import java.io.Closeable
import java.io.FileInputStream
import java.lang.IllegalArgumentException

class FileScanner(private val stream: FileInputStream) : Closeable {

    private val byteStream = ByteArrayOutputStream(256)

    @ExperimentalUnsignedTypes
    fun readUInt8():UByte {
        val byte = stream.read()

        return byte.toUByte()
    }

    @ExperimentalUnsignedTypes
    fun readUInt32():UInt {
        val bytes = stream.readNBytes(4)

        return (bytes[0].toUInt() shl 0) +
               (bytes[1].toUInt() shl 8) +
               (bytes[2].toUInt() shl 16) +
               (bytes[3].toUInt() shl 24)
    }

    @ExperimentalUnsignedTypes
    fun readUInt32Number():UInt {
        var number = 0u
        var shift = 0

        var buffer = stream.read()

        while (true) {
            number = number.or(buffer.toUInt().and(0x7fu).shl(shift))

            if (buffer.toUInt().and(0x80u)==0u) {
                return number
            }

            buffer = stream.read()
            shift+=7
        }
    }

    fun readString():String {
        byteStream.reset()

        var byte = stream.read()

        while (byte != 0) {
            byteStream.write(byte)
            byte = stream.read()
        }

        return byteStream.toString(Charsets.UTF_8)
    }

    fun readBool():Boolean {
        return when(stream.read()) {
            0 -> false
            1 -> true
            else -> throw IllegalArgumentException("Bool value is not normalized")
        }
    }

    fun readTypeId(maxBytes : UInt):TypeId
    {
        return when (maxBytes) {
            1u -> stream.read().toUShort()
            2u -> {
                val bytes = stream.readNBytes(2)

                ((bytes[0].toUInt() shl 8) + bytes[1].toUInt()).toUShort()
            }
            else -> {
                assert(false)
                0u
            }
        }
    }

    fun readFileOffset(): FileOffset {
        val bytes = stream.readNBytes(8)

        return (bytes[0].toULong() shl 0) +
               (bytes[1].toULong() shl 8) +
               (bytes[2].toULong() shl 16) +
               (bytes[3].toULong() shl 24) +
               (bytes[3].toULong() shl 32) +
               (bytes[3].toULong() shl 40) +
               (bytes[3].toULong() shl 48) +
               (bytes[3].toULong() shl 56)

    }

    override fun close() {
        stream.close()
    }
}