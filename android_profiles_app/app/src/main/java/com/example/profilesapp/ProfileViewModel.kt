package com.example.profilesapp

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.MutableLiveData
import android.view.KeyEvent
import android.view.MotionEvent
import android.hardware.input.InputManager
import android.content.Context
import android.view.InputDevice
import android.net.Uri
import java.io.File
import java.io.InputStream

class ProfileViewModel(application: Application) : AndroidViewModel(application) {
    val activeProfile = MutableLiveData<String>()
    val profiles = MutableLiveData<List<String>>()
    val message = MutableLiveData<String>()
    private val mapper = InputMapper()
    private val manager = ProfileManager(application)

    init {
        manager.ensureDefaults()
        autoLoadDefault()
        refresh()
    }

    private fun autoLoadDefault() {
        val ctx = getApplication<Application>()
        val im = ctx.getSystemService(Context.INPUT_SERVICE) as InputManager
        val hasGamepad = im.inputDeviceIds.any { id ->
            im.getInputDevice(id)?.sources?.let { s ->
                s and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD
            } ?: false
        }
        val profile = if(hasGamepad) "alt.json" else "default.json"
        val file = manager.profileFile(profile)
        if(file.exists()) {
            mapper.loadProfile(file.path)
            mapper.activateProfile(profile)
            activeProfile.postValue(profile)
        }
    }

    private fun refresh() {
        profiles.postValue(manager.listProfiles())
    }

    fun selectProfile(name: String) {
        val file = manager.profileFile(name)
        if (mapper.loadProfile(file.path)) {
            mapper.activateProfile(name)
            activeProfile.postValue(name)
        } else {
            message.postValue("Failed to load $name")
        }
    }

    fun mapKey(event: KeyEvent): String? {
        val res = mapper.mapKeyEvent(event)
        if (res != null) message.postValue(res)
        return res
    }

    fun mapMotion(event: MotionEvent): String? {
        val axis = MotionEvent.AXIS_X
        val res = mapper.mapMotionEvent(event.source, axis, event.getAxisValue(axis))
        if (res != null) message.postValue(res)
        return res
    }

    fun exportProfiles(dir: File): Boolean {
        val ok = manager.exportProfiles(dir)
        if(ok) refresh()
        return ok
    }

    fun importProfile(uri: Uri, input: InputStream): Boolean {
        val ok = manager.importProfile(uri, input)
        if(ok) refresh()
        return ok
    }

    override fun onCleared() {
        super.onCleared()
        mapper.destroy()
    }
}
