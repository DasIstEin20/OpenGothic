package com.example.profilesapp

import android.os.Bundle
import android.view.KeyEvent
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.viewModels
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import org.json.JSONObject

class EditProfileActivity : ComponentActivity() {
    private val viewModel: EditProfileViewModel by viewModels {
        val name = intent.getStringExtra("name") ?: ""
        EditProfileViewModel.Factory(application, name)
    }
    private lateinit var adapter: MappingAdapter
    private var bindingIndex: Int = -1

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_edit)
        adapter = MappingAdapter(viewModel.mappings) { idx ->
            bindingIndex = idx
            Toast.makeText(this, "Press a button", Toast.LENGTH_SHORT).show()
        }
        val list = findViewById<RecyclerView>(R.id.mappingList)
        list.layoutManager = LinearLayoutManager(this)
        list.adapter = adapter

        findViewById<View>(R.id.saveButton).setOnClickListener {
            if (viewModel.save())
                Toast.makeText(this, "Saved", Toast.LENGTH_SHORT).show()
            else
                Toast.makeText(this, "Failed", Toast.LENGTH_SHORT).show()
            finish()
        }
    }

    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        if (bindingIndex >= 0 && event.action == KeyEvent.ACTION_DOWN) {
            val src = if (event.source and android.view.InputDevice.SOURCE_GAMEPAD == android.view.InputDevice.SOURCE_GAMEPAD) 4 else 3
            viewModel.updateBinding(bindingIndex, src, event.keyCode)
            adapter.notifyItemChanged(bindingIndex)
            bindingIndex = -1
            return true
        }
        return super.dispatchKeyEvent(event)
    }

    class EditProfileViewModel(val app: android.app.Application, val name: String) : ViewModel() {
        val manager = ProfileManager(app)
        val mappings = mutableListOf<MappingAdapter.Mapping>()

        init {
            manager.ensureDefaults()
            val file = manager.profileFile(name)
            if (file.exists()) {
                val json = JSONObject(file.readText())
                for (srcKey in json.keys()) {
                    val inner = json.getJSONObject(srcKey)
                    for (codeKey in inner.keys()) {
                        mappings += MappingAdapter.Mapping(inner.getString(codeKey), srcKey.toInt(), codeKey.toInt())
                    }
                }
            }
        }

        fun updateBinding(idx: Int, src: Int, code: Int) {
            mappings[idx].source = src
            mappings[idx].code = code
        }

        fun save(): Boolean {
            val root = JSONObject()
            for (m in mappings) {
                val obj = root.optJSONObject(m.source.toString()) ?: JSONObject().also { root.put(m.source.toString(), it) }
                obj.put(m.code.toString(), m.action)
            }
            return manager.saveProfile(name, root.toString())
        }

        class Factory(private val app: android.app.Application, private val name: String) : ViewModelProvider.Factory {
            override fun <T : ViewModel> create(modelClass: Class<T>): T {
                return EditProfileViewModel(app, name) as T
            }
        }
    }
}
