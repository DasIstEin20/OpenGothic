package com.example.profilesapp

import android.os.Bundle
import android.view.KeyEvent
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.viewModels
import androidx.lifecycle.Observer
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import android.content.Intent

class MainActivity : ComponentActivity() {
    private val viewModel: ProfileViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val active = findViewById<android.widget.TextView>(R.id.activeProfile)
        val overlay = findViewById<android.widget.TextView>(R.id.actionOverlay)
        val recycler = findViewById<RecyclerView>(R.id.profileList)
        recycler.layoutManager = LinearLayoutManager(this)

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
}
