package com.retrogbm

import android.app.Activity
import android.content.Context
import android.os.Bundle
import android.os.Environment
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.retrogbm.ui.theme.RetroGBmTheme

class OptionsActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            RetroGBmTheme {
                Content()
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun Content() {
    val context = LocalContext.current as Activity

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
                        context.finish()
                    }) {
                        Icon(
                            imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                            contentDescription = "Localized description"
                        )
                    }
                }
            )
        },
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(innerPadding)
        ) {
            ListContent()
        }
    }
}

@Composable
fun ListContent() {
    val titleColor = MaterialTheme.colorScheme.onSurface
    val subtitleColor = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.7f)

    val context = LocalContext.current as Activity
    val sharedPreferences = remember {
        context.getSharedPreferences("retrogbm_settings_prefs", Context.MODE_PRIVATE)
    }

    var emulationSpeed by remember {
        mutableFloatStateOf(sharedPreferences.getFloat("emulation_speed", 2.0f))
    }

    var enableHapticFeedback by remember {
        mutableStateOf(sharedPreferences.getBoolean("haptic_feedback", true))
    }

    var enableSound by remember {
        mutableStateOf(sharedPreferences.getBoolean("enable_sound", true))
    }

    var skipBootRom by remember {
        mutableStateOf(sharedPreferences.getBoolean("skip_boot_rom", true))
    }

    val absolutePath = context.getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
    val romDirectory = absolutePath.let { "$it/ROMS" }
    val saveStateDirectory = absolutePath.let { "$it/SaveStates" }

    LazyColumn {
        item {
            Text(
                text = "Paths",
                modifier = Modifier
                    .padding(horizontal = 8.dp, vertical = 12.dp),
                color = titleColor,
                fontSize = 20.sp,
                fontWeight = FontWeight.Bold
            )
        }
        item {
            HorizontalDivider(
                color = Color.Gray, // Color of the border
                thickness = 1.dp,   // Thickness of the border
                modifier = Modifier.padding(vertical = 0.dp)
            )
        }
        item {
            Column(
                modifier = Modifier
            ) {
                Text(
                    text = "ROM Directory",
                    fontSize = 18.sp,
                    modifier = Modifier
                        .padding(horizontal = 8.dp, vertical = 12.dp)
                        .fillMaxWidth(),
                    color = titleColor
                )
                Text(
                    text = romDirectory,
                    modifier = Modifier
                        .padding(horizontal = 8.dp)
                        .fillMaxWidth()
                        .horizontalScroll(rememberScrollState()),
                    color = subtitleColor,
                    fontSize = 12.sp
                )
            }
        }
        item {
            HorizontalDivider(
                color = Color.Gray, // Color of the border
                thickness = 1.dp,   // Thickness of the border
                modifier = Modifier.padding(vertical = 0.dp)
            )
        }
        item {
            Column(
                modifier = Modifier
            ) {
                Text(
                    text = "SaveState Directory",
                    fontSize = 18.sp,
                    modifier = Modifier
                        .padding(horizontal = 8.dp, vertical = 12.dp)
                        .fillMaxWidth(),
                    color = titleColor
                )
                Text(
                    text = saveStateDirectory,
                    modifier = Modifier
                        .padding(horizontal = 8.dp)
                        .fillMaxWidth()
                        .horizontalScroll(rememberScrollState()),
                    color = subtitleColor,
                    fontSize = 12.sp
                )
            }
        }
        item {
            HorizontalDivider(
                color = Color.Gray, // Color of the border
                thickness = 1.dp,   // Thickness of the border
                modifier = Modifier.padding(vertical = 0.dp)
            )
        }
        item {
            Text(
                text = "Emulation",
                modifier = Modifier
                    .padding(horizontal = 8.dp, vertical = 12.dp),
                color = titleColor,
                fontSize = 20.sp,
                fontWeight = FontWeight.Bold
            )
        }
        item {
            HorizontalDivider(
                color = Color.Gray, // Color of the border
                thickness = 1.dp,   // Thickness of the border
                modifier = Modifier.padding(vertical = 0.dp)
            )
        }
        item {
            Column(
                modifier = Modifier
            ) {
                Text(
                    text = "Set Speed Multiplier x$emulationSpeed",
                    fontSize = 18.sp,
                    modifier = Modifier
                        // .padding(horizontal = 8.dp, vertical = 12.dp)
                        .padding(8.dp, 12.dp, 8.dp, 0.dp)
                        .fillMaxWidth(),
                    color = titleColor
                )

                Slider(
                    modifier = Modifier
                        .padding(horizontal = 8.dp),
                    value = emulationSpeed,
                    steps = 8,
                    valueRange = 1f..10f,
                    onValueChange = {
                        emulationSpeed =  it.toInt().toFloat()
                        with (sharedPreferences.edit()) {
                            putFloat("emulation_speed", emulationSpeed)
                            apply()
                        }
                    }
                )
            }
        }
        item {
            HorizontalDivider(
                color = Color.Gray, // Color of the border
                thickness = 1.dp,   // Thickness of the border
                modifier = Modifier.padding(vertical = 0.dp)
            )
        }
        item {
            Column(
                modifier = Modifier.fillMaxWidth()
            ) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(horizontal = 8.dp, vertical = 0.dp),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = "Enable Haptic Feedback",
                        fontSize = 18.sp,
                        color = titleColor
                    )

                    Switch(
                        checked = enableHapticFeedback,
                        onCheckedChange = {
                            enableHapticFeedback = it
                            with (sharedPreferences.edit()) {
                                putBoolean("haptic_feedback", enableHapticFeedback)
                                apply()
                            }
                        }
                    )
                }
            }
        }
        item {
            Column(
                modifier = Modifier.fillMaxWidth()
            ) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(horizontal = 8.dp, vertical = 0.dp),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = "Skip Boot ROM",
                        fontSize = 18.sp,
                        color = titleColor
                    )

                    Switch(
                        checked = skipBootRom,
                        onCheckedChange = {
                            skipBootRom = it
                            with (sharedPreferences.edit()) {
                                putBoolean("skip_boot_rom", skipBootRom)
                                apply()
                            }
                        }
                    )
                }
            }
        }
        item {
            HorizontalDivider(
                color = Color.Gray, // Color of the border
                thickness = 1.dp,   // Thickness of the border
                modifier = Modifier.padding(vertical = 0.dp)
            )
        }
        item {
            Column(
                modifier = Modifier.fillMaxWidth()
            ) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(horizontal = 8.dp, vertical = 0.dp),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = "Enable Sound",
                        fontSize = 18.sp,
                        color = titleColor
                    )

                    Switch(
                        checked = enableSound,
                        onCheckedChange = {
                            enableSound = it
                            with (sharedPreferences.edit()) {
                                putBoolean("enable_sound", enableSound)
                                apply()
                            }

                            Emulator.emulator.soundOutput.toggleAudio(enableSound)
                        }
                    )
                }
            }
        }
        item {
            HorizontalDivider(
                color = Color.Gray, // Color of the border
                thickness = 1.dp,   // Thickness of the border
                modifier = Modifier.padding(vertical = 0.dp)
            )
        }

//        item {
//            Column(
//                modifier = Modifier
//                    .clickable {
//                    }
//            ) {
//                Text(
//                    text = "Clear Profile",
//                    fontSize = 18.sp,
//                    modifier = Modifier
//                        .padding(horizontal = 8.dp, vertical = 12.dp)
//                        .fillMaxWidth(),
//                    color = titleColor
//                )
//            }
//        }
//        item {
//            HorizontalDivider(
//                color = Color.Gray, // Color of the border
//                thickness = 1.dp,   // Thickness of the border
//                modifier = Modifier.padding(vertical = 0.dp)
//            )
//        }
    }
}

//@Preview(showBackground = true)
//@Composable
//fun PreviewListContent() {
//    RetroGBmTheme {
//        ListContent(options = OptionData(2.0f)) {}
//    }
//}
//
//@Preview(showBackground = true)
//@Composable
//fun PreviewOptionsCard() {
//    RetroGBmTheme {
//        OptionsCard("Clear Profile")
//    }
//}
