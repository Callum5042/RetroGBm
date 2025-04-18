package com.retrogbm

import android.content.Intent
import android.os.Bundle
import android.os.Environment
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Edit
import androidx.compose.material.icons.filled.FileOpen
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
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
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.retrogbm.profile.ProfileRepository
import com.retrogbm.ui.theme.RetroGBmTheme
import com.retrogbm.utilities.TimeFormatter
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import kotlin.time.Duration.Companion.minutes

class HomeActivity : ComponentActivity() {

    // Dependency
    private val profileRepository = ProfileRepository()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Path variables
        val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
        val profilePath = absolutePath?.let { "$it/profile.json" } ?: "profile.json"
        val romPath = absolutePath?.let { "$it/ROMS" }

        // Load the ROMS
        if (romPath.isNullOrEmpty()) {
            Log.i("ROMFiles", "ROM Path is empty")
        } else {
            val files = listFilesInDirectory(romPath)
            val profileData = profileRepository.loadProfileData(profilePath)

            files.forEach{ file ->
                Log.i("ROMFiles", "File: $file")
            }

            val previewData = convertFilesToGameData(files)
            val previewMap = previewData.associateBy { it.title }

            profileData.gameData.forEach { gameData ->
                previewMap[gameData.fileName]?.let { previewGameData ->
                    if (gameData.lastPlayed != null) {
                        previewGameData.lastPlayed = formatDateModified(gameData.lastPlayed!!)
                        previewGameData.totalPlayTimeMinutes = formatTimePlayed(gameData.totalPlayTimeMinutes.toDouble())
                    }
                }
            }

            val previewRomData = ProfileRomData(previewData)

            setContent {
                RetroGBmTheme {
                    Content(previewRomData)
                }
            }
        }
    }

    private fun convertFilesToGameData(files: List<String>) : MutableList<ProfileRomGameData> {
        return files.map { fileName ->
            ProfileRomGameData(
                title = fileName,
                lastPlayed = "Never Played",
                totalPlayTimeMinutes = "No Time Played"
            )
        }.toMutableList()
    }

    private fun listFilesInDirectory(directoryPath: String) : List<String> {
        val filteredFiles = mutableListOf<String>()

        val directory = File(directoryPath)
        if (directory.exists() && directory.isDirectory) {
            val files = directory.listFiles()
            if (files != null) {
                for (file in files) {
                    if (file.isFile && (file.name.endsWith(".gb") || file.name.endsWith(".gbc"))) {
                        filteredFiles.add(file.name)
                    }
                }
            } else {
                Log.d("ROMFiles", "The directory is empty or an error occurred.")
            }
        } else {
            Log.d("ROMFiles", "The specified path is not a valid directory.")
        }

        return filteredFiles
    }

    private fun formatDateModified(dateModified: Date): String {
        // Format the date as "yyyy/MM/dd"
        val formatter = SimpleDateFormat("dd/MM/yyyy", Locale.ENGLISH)
        return formatter.format(dateModified)
    }

    private fun formatTimePlayed(timePlayed: Double): String {
        val timeFormatter = TimeFormatter()
        return timeFormatter.formatTimePlayed(timePlayed.minutes)
    }
}

data class ProfileRomGameData(
    var title: String,
    var lastPlayed: String,
    var totalPlayTimeMinutes: String
)

data class ProfileRomData(
    val gameData: MutableList<ProfileRomGameData>
)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun Content(previewRomData: ProfileRomData) {

    val context = LocalContext.current

    var showMenu by remember { mutableStateOf(false) }

    // Launcher for the ACTION_OPEN_DOCUMENT intent
    val openDocumentLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocument(),
        onResult = { uri ->
            if (uri != null) {
                val intent = Intent(context, EmulatorActivity::class.java)
                intent.putExtra("ROM_URI", uri.toString())
                context.startActivity(intent)
            }
        }
    )

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
                actions = {
                    IconButton(onClick = {
                        openDocumentLauncher.launch(arrayOf("*/*"))
                    }) {
                        Icon(
                            imageVector = Icons.Filled.FileOpen,
                            contentDescription = "Load ROM"
                        )
                    }
                    // Menu
                    Box {
                        // Overflow Menu
                        IconButton(onClick = { showMenu = !showMenu }) {
                            Icon(
                                imageVector = Icons.Filled.MoreVert,
                                contentDescription = "More Options"
                            )
                        }

                        // Dropdown menu
                        DropdownMenu(
                            expanded = showMenu,
                            onDismissRequest = { showMenu = false },
                        ) {
                            DropdownMenuItem(
                                text = { Text(stringResource(id = R.string.settings)) },
                                leadingIcon = { Icon(Icons.Filled.Settings, null) },
                                onClick = {
                                    showMenu = false

                                    val intent = Intent(context, OptionsActivity::class.java)
                                    context.startActivity(intent)
                                }
                            )
                        }
                    }
                },
            )
        },
    ) { innerPadding ->
        BodyContent(previewRomData, innerPadding)
    }
}

@Composable
fun BodyContent(previewRomData: ProfileRomData, innerPadding: PaddingValues) {
    Column(
        modifier = Modifier
            .fillMaxSize() // Ensure system bars are handled properly
            .padding(innerPadding)  // Adjust padding if needed
    ) {
        if (previewRomData.gameData.isEmpty()) {
            NoRomFound()
        } else {
            List(data = previewRomData)
        }
    }
}

@Composable
fun List(data: ProfileRomData) {
    LazyColumn(
        modifier = Modifier
            .fillMaxSize(),
    ) {
        items(data.gameData) { gameData ->
            RomInfoCard(
                title = gameData.title,
                time = gameData.totalPlayTimeMinutes,
                date = gameData.lastPlayed
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
fun RomInfoCard(title: String, time: String, date: String) {

    val context = LocalContext.current
    val titleColor = MaterialTheme.colorScheme.onSurface
    val subtitleColor = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.7f)

    Column(
        modifier = Modifier
            .padding(horizontal = 0.dp)
            .clickable {
                val intent = Intent(context, EmulatorActivity::class.java)
                intent.putExtra("ROM_TITLE", title)
                context.startActivity(intent)
            }
    ) {
        Text(
            text = title,
            modifier = Modifier
                .padding(horizontal = 8.dp)
                .fillMaxWidth()
                .horizontalScroll(rememberScrollState()),
            color = titleColor
        )
        Row(
            modifier = Modifier
                .padding(horizontal = 8.dp)
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

@Composable
fun NoRomFound() {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Text(
            text = "No ROMS found in directory",
            modifier = Modifier
                .align(Alignment.Center),
            color = Color.DarkGray
        )
    }
}

@Preview(showBackground = true)
@Composable
fun RomInfoCardPreview() {
    RetroGBmTheme {
        RomInfoCard("Pokemon - Crystal Version (UE) (V1.1) [C][!].gbc", "13 minutes", "14/03/2024")
    }
}

@Preview(showBackground = true)
@Composable
fun BodyContentHasRomsPreview() {
    RetroGBmTheme {
        val previewData = ProfileRomData(
            gameData = mutableListOf(
                ProfileRomGameData("Pokemon Red", "04/12/2023", "2 hours"),
                ProfileRomGameData("Legend of Zelda", "04/05/2019", "2 hours 20 minutes"),
                ProfileRomGameData("Super Mario", "04/12/2021", "95 minutes")
            )
        )

        Content(previewData)
    }
}

@Preview(showBackground = true)
@Composable
fun BodyContentNoRomsPreview() {
    RetroGBmTheme {
        val previewData = ProfileRomData(
            gameData = mutableListOf()
        )

        Content(previewData)
    }
}