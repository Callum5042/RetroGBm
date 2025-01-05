package com.retrogbm

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.os.Environment
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.navigation.compose.rememberNavController
import com.retrogbm.ui.theme.RetroGBmTheme
import com.retrogbm.utilities.TimeFormatter
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.text.SimpleDateFormat
import java.time.Instant
import java.time.ZoneId
import java.time.format.DateTimeFormatter
import java.time.temporal.ChronoUnit
import java.util.Locale
import kotlin.time.Duration.Companion.milliseconds
import kotlin.time.Duration.Companion.minutes
import kotlin.time.Duration.Companion.seconds

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
    val timeFormatter = TimeFormatter()
    return timeFormatter.formatTimePlayed(timePlayed.seconds)
}

class SaveStateActivity : ComponentActivity() {

    private var stateType: Int = -1

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Get passed value
        val romFileName = intent.getStringExtra("RomFileName")
        stateType = intent.getIntExtra("StateType", -1)

        // Values
        val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath

        // Using the rom title try fetch 1-9 save-state files
        val saveStateData = mutableListOf<SaveStateData>()
        for (i in 1..9) {

            val path = absolutePath?.let { "$it/SaveStates/$romFileName/slot$i.state" }
            val file = File(path!!)

            if (!file.exists()) {
                saveStateData.add(SaveStateData(i, "", "", stateType))
                continue
            }

            val data = readSaveStateHeader(file)

            // Format the date modified
            val dateModifiedStr = formatDateModified(data.dateModified)

            // Format the time played
            val timePlayedStr = formatTimePlayed(data.timePlayed)

            saveStateData.add(SaveStateData(i, dateModifiedStr, timePlayedStr, stateType))
        }

        setContent {
            RetroGBmTheme {
                Content(saveStateData, saveStateType = stateType)
            }
        }
    }
}

data class SaveStateData(val slot: Int, val dateModified: String, val timePlayed: String, val type: Int)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun Content(saveStateData: MutableList<SaveStateData>, saveStateType: Int) {

    // TODO: Once everything is on Jetpack Compose, this then might work...
    // val navController = rememberNavController()
    val context = LocalContext.current as? Activity

    Scaffold(
        topBar = {
            TopAppBar(
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primaryContainer,
                    titleContentColor = MaterialTheme.colorScheme.primary,
                ),
                title = {
                    Text("RetroGBm")
                },
                navigationIcon = {
                    IconButton(onClick = {
                        context?.finish()
                    }) {
                        Icon(
                            imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                            contentDescription = "Localized description"
                        )
                    }
                },
                actions = {
                    IconButton(onClick = { /* do something */ }) {
                        Icon(
                            imageVector = Icons.Filled.Menu,
                            contentDescription = "Localized description"
                        )
                    }
                },
            )
        },
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .fillMaxSize() // Ensure system bars are handled properly
                .padding(innerPadding)  // Adjust padding if needed
        ) {
            ListContent(saveStateData, saveStateType)
        }
    }
}

@Composable
fun ListContent(saveStateData: MutableList<SaveStateData>, saveStateType: Int) {
    LazyColumn(
        modifier = Modifier
            .fillMaxSize()
            .padding(8.dp),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        items(count = saveStateData.size) { index ->
            val item = saveStateData[index]
            SaveStateSlotCard(
                SaveStateData(item.slot, item.dateModified, item.timePlayed, saveStateType)
            )
            HorizontalDivider(
                color = Color.Gray, // Color of the border
                thickness = 1.dp,   // Thickness of the border
                modifier = Modifier.padding(vertical = 0.dp)
            )
        }
    }
}

@Composable
fun SaveStateSlotCard(data: SaveStateData) {

    val context = LocalContext.current as? Activity
    val titleColor = MaterialTheme.colorScheme.onSurface
    val subtitleColor = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.7f)

    val title = "Slot ${data.slot}"
    val time = data.timePlayed
    val date = data.dateModified

    Column(
        modifier = Modifier
            .padding(horizontal = 0.dp)
            .clickable {
                val resultIntent = Intent().apply {
                    putExtra("Slot", data.slot)
                    putExtra("StateType", data.type)
                }
                context?.setResult(Activity.RESULT_OK, resultIntent)
                context?.finish()
            }
    ) {
        Text(
            text = title,
            modifier = Modifier
                .fillMaxWidth(),
            color = titleColor
        )
        Row(
            modifier = Modifier
                .fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(
                text = time,
                modifier = Modifier,
                color = subtitleColor,
                fontSize = 12.sp
            )
            Text(
                text = date,
                modifier = Modifier,
                color = subtitleColor,
                fontSize = 12.sp
            )
        }
    }
}

@Preview(showBackground = true)
@Composable
fun PreviewContent() {
    val saveStateData = mutableListOf(
        SaveStateData(1, "04/12/2023", "2 hours 15 minutes", 0),
        SaveStateData(2, "04/05/2019", "26.4 hours", 0),
        SaveStateData(3, "04/12/2021", "26.4 hours", 0)
    )

    RetroGBmTheme {
        Content(saveStateData, 0)
    }
}

@Preview(showBackground = true)
@Composable
fun PreviewMessageCard() {
    RetroGBmTheme {
        SaveStateSlotCard(
            data = SaveStateData(1, "2024/08/20", "26.4 hours", 0)
        )
    }
}
