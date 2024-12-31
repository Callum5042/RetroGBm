package com.retrogbm.utilities

import kotlin.time.Duration
import kotlin.time.Duration.Companion.hours

class TimeFormatter {

    fun formatTimePlayed(timePlayed: Duration): String {

        // Calculate hours and minutes
        val hours = timePlayed.inWholeHours
        var minutes = timePlayed.inWholeMinutes

        if (minutes > 120) {
            minutes = timePlayed.minus(timePlayed.inWholeHours.hours).inWholeMinutes
        }

        // Format
        return when {
            hours >= 2 && minutes > 0 -> "$hours hours $minutes minutes"
            hours >= 2 -> "$hours hours"
            else -> "$minutes minutes"
        }
    }
}