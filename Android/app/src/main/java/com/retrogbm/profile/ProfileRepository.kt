package com.retrogbm.profile

import android.util.Log
import com.google.gson.GsonBuilder
import java.io.File
import java.io.IOException

class ProfileRepository {

    private val gson = GsonBuilder().setDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZ").create()

    fun loadProfileData(path: String): ProfileData {
        val file = File(path)

        // Reads the profile data, otherwise creates a new profile
        return if (file.exists()) {
            try {
                val json = file.readText()
                gson.fromJson(json, ProfileData::class.java)
            } catch (e: IOException) {
                Log.e("ProfileRepository", "Error reading profile from file", e)
                createNewProfile(file)
            } catch (e: Exception) {
                Log.e("ProfileRepository", "Error parsing profile JSON", e)
                throw e
            }
        } else {
            createNewProfile(file)
        }
    }

    fun saveProfileData(path: String, profile: ProfileData) {
        val file = File(path)

        try {
            val json = gson.toJson(profile)
            file.writeText(json)
        } catch (e: Exception) {
            Log.e("ProfileRepository", "Error writing profile to file", e)
            throw e
        }
    }

    private fun createNewProfile(file: File): ProfileData {
        val profile = ProfileData(gameData = mutableListOf())

        try {
            // Create new file and write profile
            file.writeText(gson.toJson(profile))
            Log.i("ProfileRepository", "New profile created at ${file.absolutePath}}")
        } catch (e: IOException) {
            Log.e("ProfileRepository", "Error writing profile to file", e)
        }

        return profile
    }
}