package com.retrogbm.utilities

import kotlin.time.Duration
import kotlin.time.Duration.Companion.hours

class TimeFormatter {

    fun formatTimePlayed(timePlayed: Duration): String {

        // Calculate hours and minutes
        val hours = timePlayed.inWholeHours
        var minutes = timePlayed.inWholeMinutes

        if (minutes >= 120L) {
            minutes = timePlayed.minus(timePlayed.inWholeHours.hours).inWholeMinutes
        }

        // Format
        return when {
            hours >= 2L && minutes > 0L -> "$hours hours $minutes minutes"
            hours >= 2L -> "$hours hours"
            minutes == 0L -> "Less than a minute"
            else -> "$minutes minutes"
        }
    }
}