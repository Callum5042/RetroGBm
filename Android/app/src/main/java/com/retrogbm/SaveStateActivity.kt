package com.retrogbm

import android.app.Activity
import android.app.AlertDialog
import android.content.Intent
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Divider
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.Surface
import androidx.compose.material3.TextButton
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.time.Instant
import java.time.ZoneId
import java.time.format.DateTimeFormatter
import java.time.temporal.ChronoUnit
import java.util.Locale

// Structure to hold the parsed header data
data class SaveStateHeader(
    val identifier: String,
    val version: Int,
    val dateCreated: Long,
    val dateModified: Long,
    val timePlayed: Double,
    val reserved: ByteArray)

fun readSaveStateHeader(file: File): SaveStateHeader {

    // Read the entire file into a byte array
    val bytes = file.readBytes()

    // Create a ByteBuffer with the file's data
    val buffer = ByteBuffer.wrap(bytes)
    buffer.order(ByteOrder.LITTLE_ENDIAN)  // Match C++'s typical default

    // Read the identifier (8 bytes)
    val identifierBytes = ByteArray(8)
    buffer.get(identifierBytes)
    val identifier = String(identifierBytes)

    // Read the version (4 bytes)
    val version = buffer.int

    // Read the date_created (8 bytes)
    val dateCreated = buffer.long

    // Read the date_modified (8 bytes)
    val dateModified = buffer.long

    // Read the time_played (8 bytes, double)
    val timePlayed = buffer.double

    // Read the reserved space (28 bytes)
    val reserved = ByteArray(28)
    buffer.get(reserved)

    return SaveStateHeader(
        identifier = identifier,
        version = version,
        dateCreated = dateCreated,
        dateModified = dateModified,
        timePlayed = timePlayed,
        reserved = reserved
    )
}

fun formatDateModified(dateModified: Long): String {
    // Convert seconds since epoch to Instant
    val instant = Instant.ofEpochSecond(dateModified)

    // Convert Instant to local date (assuming system's default time zone)
    val localDate = instant.atZone(ZoneId.systemDefault()).toLocalDate()

    // Format the date as "yyyy/MM/dd"
    val formatter = DateTimeFormatter.ofPattern("yyyy/MM/dd", Locale.ENGLISH)

    return localDate.format(formatter)
}

fun formatTimePlayed(timePlayed: Double): String {
    // Convert timePlayed from seconds to Duration
    val duration = java.time.Duration.ofMillis((timePlayed * 1000).toLong())

    // Calculate hours and minutes
    val hours = duration.toHours()
    val minutes = duration.minus(hours, ChronoUnit.HOURS).toMinutes()

    val sb = StringBuilder()

    if (hours != 0L) {
        sb.append(hours)
        if (minutes != 0L) {
            sb.append(" hours ")
            sb.append(minutes)
        }
    } else {
        sb.append(minutes).append(" minutes")
    }

    return sb.toString()
}

class SaveStateActivity : ComponentActivity() {

    private var statetype: Int = -1

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val romPath = intent.getStringExtra("RomTitle")
        statetype = intent.getIntExtra("StateType", -1)

        val type = if (statetype == 1) "Save" else "Load"
        // Toast.makeText(this, "Type $type", Toast.LENGTH_SHORT).show()

        val medata = mutableListOf<SaveStateData>()

        // Using the rom title try fetch 1-9 save-state files
        for (i in 1..9) {

            val path = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath!! + "/" + "${romPath}.slot${i}.state"
            var file = File(path)

            if (!file.exists()) {
                medata.add(SaveStateData(i, "", "", statetype))
                continue
            }

            val data = readSaveStateHeader(file)

            // Format the date modified
            val dateModifiedStr = formatDateModified(data.dateModified)

            // Format the time played
            val timePlayedStr = formatTimePlayed(data.timePlayed)

            medata.add(SaveStateData(i, dateModifiedStr, timePlayedStr, statetype))
        }

        setContent {
            LazyColumn(
                verticalArrangement = Arrangement.spacedBy(12.dp),
            ) {
                items(count = medata.size) { index ->
                    val item = medata[index]
                    SaveStateSlotCard(SaveStateData(item.slot, item.dateModified, item.timePlayed, statetype))
                    HorizontalDivider(color = Color.Black)
                }
            }
        }
    }
}

data class SaveStateData(val slot: Int, val dateModified: String, val timePlayed: String, val type: Int)

@Composable
fun SaveStateSlotCard(data: SaveStateData) {

    val context = LocalContext.current as Activity

    Surface(onClick = {
            val resultIntent = Intent().apply {
                putExtra("Slot", data.slot)
                putExtra("StateType", data.type)
            }

            context.setResult(Activity.RESULT_OK, resultIntent)
            context.finish()
        }
    ) {
        Row(modifier = Modifier
            .fillMaxWidth()
            .padding(all = 20.dp)
        ) {
            Text(
                if (data.dateModified.isEmpty() && data.timePlayed.isEmpty()) {
                    "Slot ${data.slot} - Empty"
                } else{
                    "Slot ${data.slot} - ${data.dateModified} - ${data.timePlayed}"
                },

                fontSize = 20.sp
            )
        }
    }
}

@Preview
@Composable
fun PreviewMessageCard() {
    SaveStateSlotCard(
        data = SaveStateData(1, "2024/08/20", "26.4 hours", 0)
    )
}
