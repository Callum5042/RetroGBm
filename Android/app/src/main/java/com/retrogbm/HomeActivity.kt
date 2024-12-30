package com.retrogbm

import android.content.pm.PackageManager
import android.os.Bundle
import android.os.Environment
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.consumeWindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.core.view.WindowCompat
import com.retrogbm.ui.theme.RetroGBmTheme
import java.io.File
import java.util.Date


class HomeActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val path = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath!! + "/ROMS"
        val files = listFilesInDirectory(path)

        files.forEach{ file ->
            Log.d("ROMFiles", "File: ${file}")
        }

        val previewData = convertFilesToGameData(files)
        val data = ProfileRomData(previewData)

        // enableEdgeToEdge()

        setContent {
            Column(
                modifier = Modifier
                    .fillMaxSize() // Ensure system bars are handled properly
                    .padding(top = 0.dp)  // Adjust padding if needed
            ) {
                List(data = ProfileRomData(previewData)) // Use your composable here
            }
        }
    }

    private fun convertFilesToGameData(files: List<String>) : MutableList<ProfileRomGameData> {
        return files.map { fileName ->
            // For this example, we'll just use the file name as the game title
            // Assume `lastPlayed` is the current date, and `timeSpent` is some arbitrary value like 120
            ProfileRomGameData(
                title = fileName.removeSuffix(".gb").removeSuffix(".gbc"), // Remove file extension for the title
                lastPlayed = Date(), // Current date
                totalPlayTimeMinutes = 120 // Arbitrary time spent (you can change this logic as needed)
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
}

data class ProfileRomGameData(
    var title: String,
    var lastPlayed: Date?,
    var totalPlayTimeMinutes: Int
)

data class ProfileRomData(
    val gameData: MutableList<ProfileRomGameData>
)

@Composable
fun List(data: ProfileRomData) {
    androidx.compose.foundation.lazy.LazyColumn(
        modifier = Modifier.fillMaxSize(),
        verticalArrangement = Arrangement.spacedBy(8.dp) // Adds spacing between items
    ) {
        items(data.gameData) { gameData ->
            RomInfoCard(
                title = gameData.title,
                time = "${gameData.totalPlayTimeMinutes} minutes",
                date = gameData.lastPlayed?.toString() ?: "Never played"
            )
        }
    }
}

@Composable
fun RomInfoCard(title: String, time: String, date: String) {
    Column {
        Text(
            text = title,
            modifier = Modifier.fillMaxWidth()
        )
        Row(
            modifier = Modifier.fillMaxWidth().padding(bottom = 0.dp),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(
                text = time,
                modifier = Modifier,
                color = Color.DarkGray,
                fontSize = 12.sp
            )
            Text(
                text = date,
                modifier = Modifier,
                color = Color.DarkGray,
                fontSize = 12.sp
            )
        }
    }
}

@Preview(showBackground = true)
@Composable
fun RomInfoCardPreview() {
    RetroGBmTheme {
        RomInfoCard("Pokemon - Crystal Version (UE) (V1.1) [C][!]", "13 minutes", "14/03/2024")
    }
}

@Preview(showBackground = true)
@Composable
fun ListPreview() {
    RetroGBmTheme {
        val previewData = ProfileRomData(
            gameData = mutableListOf(
                ProfileRomGameData("Pokemon Red", Date(), 120),
                ProfileRomGameData("Legend of Zelda", null, 200),
                ProfileRomGameData("Super Mario", Date(), 95)
            )
        )
        List(data = previewData)
    }
}
