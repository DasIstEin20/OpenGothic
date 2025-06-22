package com.example.profilesapp

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.MutableLiveData
import java.io.File

class ProfileViewModel(application: Application) : AndroidViewModel(application) {
    val activeProfile = MutableLiveData<String>()
    private val mapper = InputMapper()

    fun selectProfile(path: String, name: String) {
        if (mapper.loadProfile(path)) {
            mapper.activateProfile(name)
            activeProfile.postValue(name)
        }
    }

    override fun onCleared() {
        super.onCleared()
        mapper.destroy()
    }
}
