package com.example.profilesapp

import android.os.Bundle
import android.view.KeyEvent
import android.view.Menu
import android.view.MenuItem
import android.view.inputmethod.InputMethodManager
import android.widget.Toast
import android.hardware.input.InputManager
import android.view.InputDevice
import android.os.Environment
import android.app.Activity
import android.provider.Settings
import androidx.activity.ComponentActivity
import androidx.activity.viewModels
import androidx.lifecycle.Observer
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import android.content.Intent

class MainActivity : ComponentActivity() {
    private val viewModel: ProfileViewModel by viewModels()
    private val REQ_IMPORT = 1001

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        if(!Settings.canDrawOverlays(this)) {
            val perm = Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION)
            startActivity(perm)
            Toast.makeText(this, "Grant overlay permission", Toast.LENGTH_LONG).show()
        }

        val active = findViewById<android.widget.TextView>(R.id.activeProfile)
        val overlay = findViewById<android.widget.TextView>(R.id.actionOverlay)
        val recycler = findViewById<RecyclerView>(R.id.profileList)
        val noHw = findViewById<android.widget.TextView>(R.id.noHardwareMessage)
        recycler.layoutManager = LinearLayoutManager(this)

        val inputManager = getSystemService(InputManager::class.java)
        val hasHw = inputManager.inputDeviceIds.any { id ->
            inputManager.getInputDevice(id)?.let { d ->
                val src = d.sources
                (src and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD) ||
                        (src and InputDevice.SOURCE_KEYBOARD == InputDevice.SOURCE_KEYBOARD)
            } ?: false
        }
        recycler.visibility = if (hasHw) android.view.View.VISIBLE else android.view.View.GONE
        noHw.visibility = if (hasHw) android.view.View.GONE else android.view.View.VISIBLE

        val adapter = ProfileAdapter(emptyList(), { name ->
            viewModel.selectProfile(name)
        }, { name ->
            val intent = Intent(this, EditProfileActivity::class.java)
            intent.putExtra("name", name)
            startActivity(intent)
        })
        recycler.adapter = adapter

        viewModel.activeProfile.observe(this, Observer { p ->
            active.text = "Active: $p"
        })

        viewModel.profiles.observe(this, Observer { list ->
            adapter.update(list)
        })

        viewModel.message.observe(this, Observer { msg ->
            Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
            overlay.text = msg
            overlay.visibility = android.view.View.VISIBLE
            overlay.postDelayed({ overlay.visibility = android.view.View.GONE }, 500)
        })
    }

    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        viewModel.mapKey(event)
        return super.dispatchKeyEvent(event)
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.main_menu, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when(item.itemId) {
            R.id.menu_import -> {
                val intent = android.content.Intent(android.content.Intent.ACTION_OPEN_DOCUMENT).apply {
                    addCategory(android.content.Intent.CATEGORY_OPENABLE)
                    type = "application/json"
                }
                startActivityForResult(intent, REQ_IMPORT)
                true
            }
            R.id.menu_export -> {
                val dir = getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS) ?: cacheDir
                val ok = viewModel.exportProfiles(dir)
                val msg = if(ok) "Exported to ${dir.absolutePath}" else "Export failed"
                Toast.makeText(this, msg, Toast.LENGTH_LONG).show()
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if(requestCode == REQ_IMPORT && resultCode == Activity.RESULT_OK) {
            data?.data?.let { uri ->
                contentResolver.openInputStream(uri)?.use { input ->
                    val ok = viewModel.importProfile(uri, input)
                    val msg = if(ok) "Imported" else "Import failed"
                    Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
                }
            }
        }
    }
}
