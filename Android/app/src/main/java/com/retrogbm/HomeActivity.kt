package com.retrogbm

import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.retrogbm.ui.theme.RetroGBmTheme
import java.io.File
import java.util.Date

class HomeActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Locate and log ROM files in internal storage
        val romFiles = findRomFilesInInternalStorage("Documents/ROMS")
        romFiles.forEach { fileName ->
            Log.d("ROMFiles", "Found ROM: $fileName")
        }

        enableEdgeToEdge()
        setContent {
            RetroGBmTheme {
                RomInfoCard(
                    title = "Android",
                    time = "13 minutes",
                    date = "1/1/2001"
                )
            }
        }
    }

    private fun findRomFilesInInternalStorage(folderName: String): List<String> {
        // Locate the folder inside the app's internal storage
        val folder = File(filesDir, folderName)

        Log.d("RetoGBm", "Folder name: ${folder.path}")

        // Check if the folder exists and is a directory
        if (!folder.exists() || !folder.isDirectory) {
            Log.w("ROMFiles", "The folder $folderName does not exist.")
            return emptyList()
        }

        // Filter files ending with .gb or .gbc
        return folder.listFiles { file ->
            file.isFile && (file.extension.equals("gb", ignoreCase = true) || file.extension.equals("gbc", ignoreCase = true))
        }?.map { it.name } ?: emptyList()
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
