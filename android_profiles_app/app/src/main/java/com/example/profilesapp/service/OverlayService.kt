package com.example.profilesapp.service

import android.accessibilityservice.AccessibilityService
import android.accessibilityservice.AccessibilityServiceInfo
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.accessibility.AccessibilityEvent
import android.widget.Toast
import androidx.lifecycle.MutableLiveData
import com.example.profilesapp.InputMapper
import com.example.profilesapp.service.InputOverlay

/** Floating overlay service capturing input events globally */

class OverlayService : AccessibilityService() {
    private val mapper = InputMapper()
    val lastAction = MutableLiveData<String>()
    private lateinit var overlay: InputOverlay

    override fun onServiceConnected() {
        val info = AccessibilityServiceInfo().apply {
            eventTypes = AccessibilityEvent.TYPES_ALL_MASK
            feedbackType = AccessibilityServiceInfo.FEEDBACK_GENERIC
            flags = AccessibilityServiceInfo.FLAG_REQUEST_FILTER_KEY_EVENTS or
                    AccessibilityServiceInfo.FLAG_RETRIEVE_INTERACTIVE_WINDOWS
        }
        serviceInfo = info
        Toast.makeText(this, "OverlayService started", Toast.LENGTH_SHORT).show()
        overlay = InputOverlay(this)
        overlay.show(lastAction)
    }

    override fun onAccessibilityEvent(event: AccessibilityEvent?) {
        // no-op
    }

    override fun onInterrupt() {
    }

    override fun onKeyEvent(event: KeyEvent): Boolean {
        val action = mapper.mapKeyEvent(event)
        if (action != null) {
            lastAction.postValue(action)
        }
        return false
    }

    override fun onGenericMotionEvent(event: MotionEvent): Boolean {
        val src = event.source
        val axis = MotionEvent.AXIS_X
        val v = event.getAxisValue(axis)
        val action = mapper.mapMotionEvent(src, axis, v)
        if (action != null) {
            lastAction.postValue(action)
        }
        return super.onGenericMotionEvent(event)
    }

    override fun onDestroy() {
        mapper.destroy()
        if(::overlay.isInitialized) overlay.hide()
        super.onDestroy()
    }
}
