package com.retrogbm.profile

import java.util.Date

data class ProfileOptions(
    var romDirectories: String
)

data class ProfileGameData(
    var name: String,
    var fileName: String,
    var checksum: String,
    var lastPlayed: Date?,
    var totalPlayTimeMinutes: Int
)

data class ProfileData(
    var options: ProfileOptions?,
    val gameData: MutableList<ProfileGameData>
)