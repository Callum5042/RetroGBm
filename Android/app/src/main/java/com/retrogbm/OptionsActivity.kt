package com.retrogbm

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.os.Environment
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
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
import androidx.compose.material3.RangeSlider
import androidx.compose.material3.RangeSliderState
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.retrogbm.options.OptionData
import com.retrogbm.options.OptionRepository
import com.retrogbm.ui.theme.RetroGBmTheme
import com.retrogbm.utilities.SaveStateType

class OptionsActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
        val optionsPath = absolutePath?.let { "$it/options.json" } ?: "options.json"

        val optionRepository = OptionRepository()
        val options = optionRepository.loadOptions(optionsPath)

        options.romDirectory = absolutePath.let { "$it/ROMS" }
        options.saveStateDirectory = absolutePath.let { "$it/SaveStates" }

        setContent {
            RetroGBmTheme {
                Content(options) {
                    optionRepository.saveOptions(optionsPath, options)
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun Content(options: OptionData, action: () -> Unit) {

    val context = LocalContext.current as? Activity

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
                }
            )
        },
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .fillMaxSize() // Ensure system bars are handled properly
                .padding(innerPadding)  // Adjust padding if needed
        ) {
            ListContent(options, action)
        }
    }
}

@Composable
fun ListContent(options: OptionData, action: () -> Unit) {
    var sliderPosition by remember { mutableFloatStateOf(options.emulationMultiplier) }

    val titleColor = MaterialTheme.colorScheme.onSurface
    val subtitleColor = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.7f)

    LazyColumn {
        item {
            Column(
                modifier = Modifier
                    .clickable {
                    }
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
                    text = options.romDirectory,
                    modifier = Modifier
                        .padding(horizontal = 8.dp)
                        .fillMaxWidth(),
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
                    .clickable {
                    }
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
                    text = options.saveStateDirectory,
                    modifier = Modifier
                        .padding(horizontal = 8.dp)
                        .fillMaxWidth(),
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
                    .clickable {
                    }
            ) {
                Text(
                    text = "Set Speed Multiplier x$sliderPosition",
                    fontSize = 18.sp,
                    modifier = Modifier
                        .padding(horizontal = 8.dp)
                        .fillMaxWidth(),
                    color = titleColor
                )

                Slider(
                    modifier = Modifier
                        .padding(horizontal = 8.dp),
                    value = sliderPosition,
                    steps = 8,
                    valueRange = 1f..10f,
                    onValueChange = {
                        sliderPosition =  it.toInt().toFloat()
                        options.emulationMultiplier = sliderPosition
                        action()
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
                modifier = Modifier
                    .clickable {
                    }
            ) {
                Text(
                    text = "Clear Profile",
                    fontSize = 18.sp,
                    modifier = Modifier
                        .padding(horizontal = 8.dp, vertical = 12.dp)
                        .fillMaxWidth(),
                    color = titleColor
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
    }
}

@Composable
fun OptionsCard(name: String) {
    val titleColor = MaterialTheme.colorScheme.onSurface

    Column(
        modifier = Modifier
            .clickable {
            }
    ) {
        Text(
            text = name,
            fontSize = 12.sp,
            modifier = Modifier
                .padding(vertical = 12.dp)
                .fillMaxWidth(),
            color = titleColor
        )
    }

    HorizontalDivider(
        color = Color.Gray, // Color of the border
        thickness = 1.dp,   // Thickness of the border
        modifier = Modifier.padding(vertical = 0.dp)
    )
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
