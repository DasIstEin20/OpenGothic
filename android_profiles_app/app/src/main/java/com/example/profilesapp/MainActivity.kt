package com.example.profilesapp

import android.os.Bundle
import android.util.Log
import java.io.File
import androidx.activity.ComponentActivity
import androidx.activity.viewModels
import androidx.lifecycle.Observer
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView

class MainActivity : ComponentActivity() {
    private val viewModel: ProfileViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val active = findViewById<android.widget.TextView>(R.id.activeProfile)
        val recycler = findViewById<RecyclerView>(R.id.profileList)
        recycler.layoutManager = LinearLayoutManager(this)

        val profiles = assets.list("profiles")?.toList() ?: emptyList()
        val adapter = ProfileAdapter(profiles) { name ->
            val path = File(filesDir, name).also { out ->
                assets.open("profiles/$name").use { input ->
                    out.outputStream().use { input.copyTo(it) }
                }
            }
            viewModel.selectProfile(path.path, name)
        }
        recycler.adapter = adapter

        viewModel.activeProfile.observe(this, Observer { p ->
            active.text = "Active: $p"
        })
    }
}
