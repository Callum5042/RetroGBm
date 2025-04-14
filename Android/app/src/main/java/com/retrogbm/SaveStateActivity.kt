package com.retrogbm

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.os.Environment
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.DeleteForever
import androidx.compose.material.icons.filled.Edit
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.OutlinedTextFieldDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TextFieldColors
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.toMutableStateList
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.retrogbm.ui.theme.RetroGBmTheme
import com.retrogbm.utilities.SaveStateType
import com.retrogbm.utilities.TimeFormatter
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.time.Instant
import java.time.ZoneId
import java.time.format.DateTimeFormatter
import java.util.Locale
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

    private var stateType: SaveStateType = SaveStateType.Save

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Get passed value
        val romFileName = intent.getStringExtra("RomFileName")
        stateType = intent.getSerializableExtra("StateType") as SaveStateType

        // Values
        val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
        val saveStatePath = absolutePath?.let { "$it/SaveStates/$romFileName/" }

        val folder = saveStatePath?.let { File(it) }
        val files = folder?.listFiles()

        val saveStateData = mutableListOf<SaveStateData>()
        files?.forEach { file ->
            val data = readSaveStateHeader(file)

            // Format the date modified
            val dateModifiedStr = formatDateModified(data.dateModified)

            // Format the time played
            val timePlayedStr = formatTimePlayed(data.timePlayed)

            saveStateData.add(SaveStateData(file.nameWithoutExtension, dateModifiedStr, timePlayedStr, stateType))
        }

        setContent {
            RetroGBmTheme {
                Content(
                    saveStateData,
                    saveStateType = stateType,
                    onUpdate = { oldPath, path ->
                        val file = File("$saveStatePath/$oldPath.state")
                        val newFile = File("$saveStatePath/$path.state")
                        val result = file.renameTo(newFile)

                        if (result) {
                            Log.i("Retro", "Renamed SaveState")

                        } else {
                            Log.i("Retro", "Renamed SaveState FAILED")
                        }
                    },
                    onDelete = { path ->
                        val file = File("$saveStatePath/$path.state")
                        file.delete()
                    }
                )
            }
        }
    }
}

data class SaveStateData(val slot: String, val dateModified: String, val timePlayed: String, val type: SaveStateType)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun InputDialog(
    title: String,
    onDismiss: () -> Unit,
    onConfirm: (String) -> Unit
) {
    var text by remember { mutableStateOf("") }

    val titleColor = MaterialTheme.colorScheme.onSurface

    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(title) },
        containerColor = Color.Black,
        titleContentColor = Color.White,
        text = {
            OutlinedTextField(
                colors = TextFieldDefaults.outlinedTextFieldColors(
                    focusedBorderColor = Color.White,
                    focusedLabelColor = Color.White,
                    cursorColor = Color.White,
                    unfocusedTextColor = Color.White,
                    unfocusedBorderColor = Color.White,
                    unfocusedLabelColor = Color.White
                ),
                value = text,
                onValueChange = { text = it },
                label = { Text("Input") }
            )
        },
        confirmButton = {
            TextButton(
                colors = ButtonDefaults.textButtonColors(
                    containerColor = Color.Black,
                    contentColor = Color.White
                ),
                onClick = {
                    onConfirm(text)
                }
            ) {
                Text("OK")
            }
        },
        dismissButton = {
            TextButton(
                colors = ButtonDefaults.textButtonColors(
                    containerColor = Color.Black,
                    contentColor = Color.White
                ),
                onClick = onDismiss) {
                Text("Cancel")
            }
        }
    )
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun Content(
    saveStateData: MutableList<SaveStateData>, saveStateType: SaveStateType,
    onUpdate: (oldPath: String, path: String) -> Unit,
    onDelete: (path: String) -> Unit
) {

    val saveStates = remember { saveStateData.toMutableStateList() }

    // TODO: Once everything is on Jetpack Compose, this then might work...
    // val navController = rememberNavController()
    val context = LocalContext.current as? Activity

    var showDialog by remember { mutableStateOf(false) }
    var userInput by remember { mutableStateOf("") }

    Scaffold(
        topBar = {
            TopAppBar(
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = Color.Black,
                    titleContentColor = Color.White,
                    navigationIconContentColor = Color.White,
                    actionIconContentColor = Color.White,
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
                    IconButton(enabled = saveStateType == SaveStateType.Save, onClick = {
                        showDialog = true
                    }) {
                        Icon(
                            imageVector = Icons.Filled.Add,
                            contentDescription = stringResource(id = R.string.quick_save)
                        )
                    }
                }
            )
        },
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .fillMaxSize() // Ensure system bars are handled properly
                .padding(innerPadding)  // Adjust padding if needed
        ) {
            LazyColumn(
                modifier = Modifier
                    .fillMaxSize()
            ) {
                items(count = saveStates.size) { index ->
                    val item = saveStates[index]
                    SaveStateSlotCard(
                        SaveStateData(item.slot, item.dateModified, item.timePlayed, saveStateType),
                        onUpdate = { oldPath, path ->
                            saveStates[index] = saveStates[index].copy(slot = path)
                            onUpdate(oldPath, path)
                        },
                        onDelete = {
                            saveStates.remove(item)
                            onDelete(it)
                        }
                    )
                    HorizontalDivider(
                        color = Color.Gray, // Color of the border
                        thickness = 1.dp,   // Thickness of the border
                        modifier = Modifier.padding(vertical = 0.dp)
                    )
                }
            }

            if (showDialog) {
                InputDialog(
                    title = "Add SaveState Slot",
                    onDismiss = { showDialog = false },
                    onConfirm = {
                        userInput = it
                        showDialog = false

                        saveStates.add(
                            SaveStateData(
                            slot = it,
                            "Never Played",
                            "No Time Played",
                            saveStateType)
                        )
                    }
                )
            }
        }
    }
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
fun SaveStateSlotCard(data: SaveStateData, onUpdate: (oldPath: String, path: String) -> Unit, onDelete: (path: String) -> Unit) {
    val context = LocalContext.current as? Activity
    val titleColor = MaterialTheme.colorScheme.onSurface
    val subtitleColor = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.7f)

    val title = data.slot
    val time = data.timePlayed
    val date = data.dateModified

    var showContextMenu by remember { mutableStateOf(false) }
    var showUpdateDialog by remember { mutableStateOf(false) }
    var showDeleteDialog by remember { mutableStateOf(false) }

    if (showUpdateDialog) {
        InputDialog(
            title = "Update SaveState Slot",
            onDismiss = { showUpdateDialog = false },
            onConfirm = {
                showUpdateDialog = false
                onUpdate(data.slot, it)
            },
        )
    }

    if (showDeleteDialog) {
        AlertDialog(
            onDismissRequest = {
                showDeleteDialog = false
            },
//            icon = {
//                Icon(Icons.Filled.Dangerous, null)
//            },
            title = { Text("Delete Save State") },
            text = {
                Text("This action is irreversible and cannot be undone.")
            },
            confirmButton = {
                TextButton(onClick = {
                    onDelete(title)
                }) {
                    Text("Confirm")
                }
            },
            dismissButton = {
                TextButton(onClick = {
                    showDeleteDialog = false
                }) {
                    Text("Cancel")
                }
            }
        )
    }

    Box(
        modifier = Modifier.fillMaxWidth(), // Ensure full width to help with positioning
        contentAlignment = Alignment.BottomEnd // Aligns the menu to the bottom-right
    ) {
        Column(
            modifier = Modifier
                .padding(horizontal = 0.dp)
                .combinedClickable(
                    onClick = {
                        val resultIntent = Intent().apply {
                            putExtra("Slot", data.slot)
                            putExtra("StateType", data.type)
                        }
                        context?.setResult(Activity.RESULT_OK, resultIntent)
                        context?.finish()
                    },
                    onLongClick = {
                        showContextMenu = true
                    }
                )
        ) {
            Text(
                text = title,
                modifier = Modifier
                    .fillMaxWidth()
                    .horizontalScroll(rememberScrollState()),
                color = titleColor
            )
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text(
                    text = time,
                    color = subtitleColor,
                    fontSize = 12.sp
                )
                Text(
                    text = date,
                    color = subtitleColor,
                    fontSize = 12.sp
                )
            }
        }

        if (showContextMenu) {
            Box(
                modifier = Modifier.padding(8.dp),
                contentAlignment = Alignment.BottomEnd
            ) {
                DropdownMenu(
                    expanded = showContextMenu,
                    onDismissRequest = { showContextMenu = false },
                    modifier = Modifier
                        .align(Alignment.BottomEnd)
                ) {
                    DropdownMenuItem(
                        text = { Text("Update") },
                        leadingIcon = { Icon(Icons.Filled.Edit, null) },
                        onClick = {
                            showContextMenu = false
                            showUpdateDialog = true
                        }
                    )
                    HorizontalDivider()
                    DropdownMenuItem(
                        text = { Text("Delete") },
                        leadingIcon = { Icon(Icons.Filled.DeleteForever, null) },
                        onClick = {
                            showContextMenu = false
                            showDeleteDialog = true
                        }
                    )
                }
            }
        }
    }
}

//@Preview(showBackground = true)
//@Composable
//fun PreviewContent() {
//    val saveStateData = mutableListOf(
//        SaveStateData("Slot 1", "04/12/2023", "2 hours 15 minutes", SaveStateType.Save),
//        SaveStateData("Slot 2", "04/05/2019", "26.4 hours", SaveStateType.Save),
//        SaveStateData("Slot 3", "04/12/2021", "26.4 hours", SaveStateType.Save)
//    )
//
//    RetroGBmTheme {
//        Content(saveStateData, SaveStateType.Save)
//    }
//}

//@Preview(showBackground = true)
//@Composable
//fun PreviewMessageCard() {
//    RetroGBmTheme {
//        SaveStateSlotCard(
//            data = SaveStateData("Slot 1", "2024/08/20", "26.4 hours", SaveStateType.Save)
//        )
//    }
//}
