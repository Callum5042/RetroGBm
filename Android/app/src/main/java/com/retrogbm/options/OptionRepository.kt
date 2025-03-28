package com.retrogbm.options

import android.util.Log
import com.google.gson.GsonBuilder
import java.io.File
import java.io.IOException

class OptionRepository {

    private val gson = GsonBuilder().setDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZ").create()

    fun loadOptions(path: String): OptionData {
        val file = File(path)

        // Reads the profile data, otherwise creates a new profile
        return if (file.exists()) {
            try {
                val json = file.readText()
                gson.fromJson(json, OptionData::class.java)
            } catch (e: IOException) {
                Log.e("ProfileRepository", "Error reading option from file", e)
                createNewProfile(file)
            } catch (e: Exception) {
                Log.e("ProfileRepository", "Error parsing option JSON", e)
                throw e
            }
        } else {
            return OptionData(2.0f)
        }
    }

    fun saveOptions(path: String, options: OptionData) {
        val file = File(path)

        if (file.exists()) {
            file.setReadable(true)
            file.setWritable(true)
        }

        try {
            val json = gson.toJson(options)
            file.writeText(json)
        } catch (e: Exception) {
            Log.e("ProfileRepository", "Error writing profile to file", e)
            throw e
        }
    }

    private fun createNewProfile(file: File): OptionData {
        val options = OptionData(2.0f)

        try {
            // Create new file and write profile
            file.writeText(gson.toJson(options))
            Log.i("ProfileRepository", "New options created at ${file.absolutePath}}")
        } catch (e: IOException) {
            Log.e("ProfileRepository", "Error writing options to file", e)
        }

        return options
    }
}