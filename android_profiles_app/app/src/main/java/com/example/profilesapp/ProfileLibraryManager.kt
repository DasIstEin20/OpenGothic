package com.example.profilesapp

import android.content.Context
import android.net.Uri
import androidx.documentfile.provider.DocumentFile
import java.io.File
import java.io.InputStream

/**
 * Manages local profile library. Profiles are stored as JSON within the
 * application files directory. This manager also supports importing and
 * exporting profiles for easy sharing between devices.
 */
class ProfileLibraryManager(private val context: Context) {
    private val dir: File = File(context.filesDir, "profiles").apply { mkdirs() }

    fun ensureDefaults() {
        val names = context.assets.list("profiles") ?: return
        for (n in names) {
            val out = File(dir, n)
            if (!out.exists()) {
                context.assets.open("profiles/$n").use { input ->
                    out.outputStream().use { input.copyTo(it) }
                }
            }
        }
    }

    fun listProfiles(): List<String> = dir.list()?.toList() ?: emptyList()

    fun profileFile(name: String): File = File(dir, name)

    /**
     * Reads and parses a profile file using [VirtualActionEngine]. Returns the
     * mapping map or `null` if loading failed.
     */
    fun loadProfile(name: String): Map<String, com.example.input.VirtualAction>? {
        return try {
            val text = profileFile(name).takeIf { it.exists() }?.readText() ?: return null
            com.example.input.VirtualActionEngine.fromJson(text)
        } catch (e: Exception) {
            null
        }
    }

    fun saveProfile(name: String, json: String): Boolean {
        return try {
            profileFile(name).writeText(json)
            true
        } catch (e: Exception) {
            false
        }
    }

    fun exportProfiles(dst: File): Boolean {
        return try {
            dst.mkdirs()
            dir.listFiles()?.forEach { f ->
                f.copyTo(File(dst, f.name), overwrite = true)
            }
            true
        } catch (e: Exception) {
            false
        }
    }

    /**
     * Loads all JSON files from the given folder into the local profile storage.
     * Existing profiles are not overwritten; instead numeric suffixes are used.
     */
    fun importFromFolder(folder: File) {
        if (!folder.isDirectory) return
        folder.listFiles { f -> f.extension.equals("json", true) }?.forEach { f ->
            val name = f.name
            var out = File(dir, name)
            var idx = 1
            while (out.exists()) {
                val base = name.substringBeforeLast('.')
                val ext = name.substringAfterLast('.', "")
                val newName = "${base}_${idx}.${ext}"
                out = File(dir, newName)
                idx++
            }
            f.copyTo(out, overwrite = false)
        }
    }

    fun importProfile(uri: Uri, input: InputStream): Boolean {
        return try {
            var name = DocumentFile.fromSingleUri(context, uri)?.name ?: return false
            var file = profileFile(name)
            var idx = 1
            while (file.exists()) {
                val base = name.substringBeforeLast('.')
                val ext = name.substringAfterLast('.', "")
                val newName = "${base}_${idx}${if(ext.isNotEmpty()) ".${ext}" else ""}"
                file = profileFile(newName)
                idx++
            }
            file.outputStream().use { out ->
                input.copyTo(out)
            }
            true
        } catch (e: Exception) {
            false
        }
    }
}
