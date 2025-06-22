package com.example.profilesapp

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.MutableLiveData
import android.view.KeyEvent

class ProfileViewModel(application: Application) : AndroidViewModel(application) {
    val activeProfile = MutableLiveData<String>()
    val profiles = MutableLiveData<List<String>>()
    val message = MutableLiveData<String>()
    private val mapper = InputMapper()
    private val manager = ProfileManager(application)

    init {
        manager.ensureDefaults()
        refresh()
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

    override fun onCleared() {
        super.onCleared()
        mapper.destroy()
    }
}
