package com.retrogbm

import com.retrogbm.utilities.TimeFormatter
import org.junit.Assert.assertEquals
import org.junit.Test
import kotlin.time.Duration.Companion.hours
import kotlin.time.Duration.Companion.minutes

class TimeFormatterTests {
    @Test
    fun formatTimePlayed_30minutes_FormatsWithOnlyMinutes() {
        // Arrange
        val timeFormatter = TimeFormatter()

        // Act
        val result = timeFormatter.formatTimePlayed(30.minutes)

        // Assert
        assertEquals("30 minutes", result)
    }

    @Test
    fun formatTimePlayed_90minutes_FormatsWithMinutesNoHours() {
        // Arrange
        val timeFormatter = TimeFormatter()

        // Act
        val result = timeFormatter.formatTimePlayed(90.minutes)

        // Assert
        assertEquals("90 minutes", result)
    }

    @Test
    fun formatTimePlayed_119minutes_FormatsWithMinutesNoHours() {
        // Arrange
        val timeFormatter = TimeFormatter()

        // Act
        val result = timeFormatter.formatTimePlayed(119.minutes)

        // Assert
        assertEquals("119 minutes", result)
    }


    @Test
    fun formatTimePlayed_120minutes_FormatsWithOnlyHours() {
        // Arrange
        val timeFormatter = TimeFormatter()

        // Act
        val result = timeFormatter.formatTimePlayed(120.minutes)

        // Assert
        assertEquals("2 hours", result)
    }

    @Test
    fun formatTimePlayed_1hours_FormatsWithMinutesNoHours() {
        // Arrange
        val timeFormatter = TimeFormatter()

        // Act
        val result = timeFormatter.formatTimePlayed(1.hours)

        // Assert
        assertEquals("60 minutes", result)
    }

    @Test
    fun formatTimePlayed_3hours_FormatsWithHourNoMinutes() {
        // Arrange
        val timeFormatter = TimeFormatter()

        // Act
        val result = timeFormatter.formatTimePlayed(3.hours)

        // Assert
        assertEquals("3 hours", result)
    }

    @Test
    fun formatTimePlayed_2hours30minutes_FormatsWithMinutesAndHours() {
        // Arrange
        val timeFormatter = TimeFormatter()

        // Act
        val result = timeFormatter.formatTimePlayed(150.minutes)

        // Assert
        assertEquals("2 hours 30 minutes", result)
    }

    @Test
    fun formatTimePlayed_0minutes_FormatsWithLessThanAMinute() {
        // Arrange
        val timeFormatter = TimeFormatter()

        // Act
        val result = timeFormatter.formatTimePlayed(0.minutes)

        // Assert
        assertEquals("Less than a minute", result)
    }
}