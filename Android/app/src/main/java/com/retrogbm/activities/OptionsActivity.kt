package com.retrogbm.activities

import android.app.Activity
import android.content.Context
import android.os.Bundle
import android.os.Environment
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.core.content.edit
import com.retrogbm.composables.OptionsCard
import com.retrogbm.composables.OptionsDivider
import com.retrogbm.composables.OptionsInfo
import com.retrogbm.composables.OptionsSlider
import com.retrogbm.composables.OptionsSwitch
import com.retrogbm.models.Emulator
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

    Column(
        modifier = Modifier
            .padding(horizontal = 8.dp, vertical = 12.dp)
            .fillMaxWidth()
    ) {
        // Paths card
        OptionsCard(
            title = "Paths"
        ) {
            OptionsInfo(
                title = "ROM Directory",
                text = romDirectory
            )
            OptionsDivider()
            OptionsInfo(
                title = "SaveState Directory",
                text = saveStateDirectory
            )
        }

        // Emulation card
        OptionsCard(
            title = "Emulation"
        ) {
            OptionsSlider(
                text = "Set Speed Multiplier x$emulationSpeed",
                value = emulationSpeed,
                steps = 8,
                valueRange = 1f..1.0f,
                onValueChanged = { value ->
                    sharedPreferences.edit {
                        putFloat("emulation_speed", value)
                    }
                }
            )
            OptionsDivider()
            OptionsSwitch(
                text = "Enable Haptic Feedback",
                value = enableHapticFeedback,
                onChange = { it ->
                    sharedPreferences.edit {
                        putBoolean("haptic_feedback", it)
                    }
                }
            )
            OptionsDivider()
            OptionsSwitch(
                text = "Skip Boot ROM",
                value = skipBootRom,
                onChange = { it ->
                    sharedPreferences.edit {
                        putBoolean("skip_boot_rom", it)
                    }
                }
            )
            OptionsDivider()
            OptionsSwitch(
                text = "Enable Sound",
                value = enableSound,
                onChange = { it ->
                    sharedPreferences.edit {
                        putBoolean("enable_sound", it)
                    }

                    Emulator.emulator.soundOutput.toggleAudio(it)
                }
            )
        }
    }
}
