package com.retrogbm.profile

import java.util.Date

data class ProfileGameData(
    var name: String,
    var checksum: String,
    var lastPlayed: Date?,
    var totalPlayTimeMinutes: Int
)

data class ProfileData(
    val gameData: MutableList<ProfileGameData>
)