package com.example.profilesapp.service

import android.content.Context
import android.graphics.PixelFormat
import android.view.Gravity
import android.view.LayoutInflater
import android.view.View
import android.view.WindowManager
import android.widget.TextView
import com.example.profilesapp.R

class InputOverlay(private val context: Context) {
    private val windowManager = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
    private val view: View = LayoutInflater.from(context).inflate(R.layout.input_overlay, null)
    private val text: TextView = view.findViewById(R.id.overlayText)

    fun show(data: androidx.lifecycle.LiveData<String>) {
        val params = WindowManager.LayoutParams(
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL or WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
            PixelFormat.TRANSLUCENT
        )
        params.gravity = Gravity.TOP or Gravity.CENTER_HORIZONTAL
        windowManager.addView(view, params)
        data.observeForever { action ->
            text.text = action
            text.visibility = View.VISIBLE
            text.postDelayed({ text.visibility = View.GONE }, 500)
        }
    }

    fun hide() {
        windowManager.removeView(view)
    }
}
