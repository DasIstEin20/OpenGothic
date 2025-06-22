package com.example.profilesapp

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView

class ProfileAdapter(
    private var items: List<String>,
    private val onClick: (String) -> Unit,
    private val onLongClick: (String) -> Unit = {}
) : RecyclerView.Adapter<ProfileAdapter.VH>() {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VH {
        val view = LayoutInflater.from(parent.context)
            .inflate(android.R.layout.simple_list_item_1, parent, false)
        return VH(view)
    }

    override fun onBindViewHolder(holder: VH, position: Int) {
        holder.text.text = items[position]
        holder.itemView.setOnClickListener { onClick(items[position]) }
        holder.itemView.setOnLongClickListener {
            onLongClick(items[position])
            true
        }
    }

    override fun getItemCount(): Int = items.size

    fun update(newItems: List<String>) {
        items = newItems
        notifyDataSetChanged()
    }

    class VH(view: View) : RecyclerView.ViewHolder(view) {
        val text: TextView = view.findViewById(android.R.id.text1)
    }
}
