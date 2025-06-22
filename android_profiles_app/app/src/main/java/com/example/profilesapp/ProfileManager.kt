package com.example.profilesapp

import android.content.Context
import android.net.Uri
import androidx.documentfile.provider.DocumentFile
import java.io.File
import java.io.InputStream

class ProfileManager(private val context: Context) {
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

    fun importProfile(uri: Uri, input: InputStream): Boolean {
        return try {
            val name = DocumentFile.fromSingleUri(context, uri)?.name ?: return false
            profileFile(name).outputStream().use { out ->
                input.copyTo(out)
            }
            true
        } catch (e: Exception) {
            false
        }
    }
}
