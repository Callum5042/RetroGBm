package com.retrogbm.profile

import java.util.Date

data class ProfileOptions(
    var romDirectories: String
)

data class ProfileCheatCode(
    var name: String,
    var code: String
)

data class ProfileGameData(
    var name: String,
    var fileName: String,
    var checksum: String,
    var lastPlayed: Date?,
    var totalPlayTimeMinutes: Int,
    var cheats: Array<ProfileCheatCode>
)

data class ProfileData(
    var options: ProfileOptions?,
    val gameData: MutableList<ProfileGameData>
)