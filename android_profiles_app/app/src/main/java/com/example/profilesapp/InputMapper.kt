package com.example.profilesapp

import android.util.Log
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent

class InputMapper {
    private var nativePtr: Long = nativeCreate()

    fun loadProfile(path: String): Boolean {
        return nativeLoadProfile(nativePtr, path)
    }

    fun activateProfile(id: String) {
        nativeActivateProfile(nativePtr, id)
        Log.i("InputMapper", "Activated profile $id")
    }

    fun mapKeyEvent(event: KeyEvent): String? {
        val src = if (event.source and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD) 4 else 3
        return nativeMapEvent(nativePtr, src, event.keyCode)
    }

    fun mapMotionEvent(src: Int, axis: Int, value: Float): String? {
        return nativeMapMotionEvent(nativePtr, src, axis, value)
    }

    fun destroy() {
        nativeDestroy(nativePtr)
    }

    private external fun nativeCreate(): Long
    private external fun nativeDestroy(ptr: Long)
    private external fun nativeLoadProfile(ptr: Long, path: String): Boolean
    private external fun nativeActivateProfile(ptr: Long, id: String)
    private external fun nativeMapEvent(ptr: Long, source: Int, code: Int): String?
    private external fun nativeMapMotionEvent(ptr: Long, source: Int, axis: Int, value: Float): String?

    companion object {
        init {
            System.loadLibrary("inputmapper")
        }
    }
}
