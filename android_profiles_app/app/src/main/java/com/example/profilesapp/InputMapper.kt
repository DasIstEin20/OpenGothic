package com.example.profilesapp

import android.content.Context
import android.util.Log

class InputMapper {
    private var nativePtr: Long = nativeCreate()

    fun loadProfile(path: String): Boolean {
        return nativeLoadProfile(nativePtr, path)
    }

    fun activateProfile(id: String) {
        nativeActivateProfile(nativePtr, id)
        Log.i("InputMapper", "Activated profile $id")
    }

    fun destroy() {
        nativeDestroy(nativePtr)
    }

    private external fun nativeCreate(): Long
    private external fun nativeDestroy(ptr: Long)
    private external fun nativeLoadProfile(ptr: Long, path: String): Boolean
    private external fun nativeActivateProfile(ptr: Long, id: String)

    companion object {
        init {
            System.loadLibrary("inputmapper")
        }
    }
}
