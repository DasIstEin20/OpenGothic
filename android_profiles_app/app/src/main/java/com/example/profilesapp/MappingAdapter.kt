package com.example.profilesapp

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView

class MappingAdapter(
    private val items: MutableList<Mapping>,
    private val onBindRequest: (Int) -> Unit
) : RecyclerView.Adapter<MappingAdapter.VH>() {

    /**
     * Representation of a single binding in the profile list. Additional fields
     * are kept so that scaling or inversion values can be edited later.
     */
    data class Mapping(
        var action: String,
        var source: Int,
        var code: Int,
        var scale: Float = 1f,
        var invert: Boolean = false,
        var deadzone: Float = 0f
    )

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VH {
        val view = LayoutInflater.from(parent.context)
            .inflate(android.R.layout.simple_list_item_2, parent, false)
        return VH(view)
    }

    override fun onBindViewHolder(holder: VH, position: Int) {
        val m = items[position]
        holder.text1.text = m.action
        holder.text2.text = "${'$'}{m.source}:${'$'}{m.code} dz=${'$'}{m.deadzone} sc=${'$'}{m.scale}${if(m.invert) " inv" else ""}"
        holder.itemView.setOnLongClickListener {
            onBindRequest(position)
            true
        }
    }

    override fun getItemCount(): Int = items.size

    class VH(view: View) : RecyclerView.ViewHolder(view) {
        val text1: TextView = view.findViewById(android.R.id.text1)
        val text2: TextView = view.findViewById(android.R.id.text2)
    }
}
