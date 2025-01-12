package com.retrogbm

import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.FileOpen
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.retrogbm.ui.theme.RetroGBmTheme

class EmulatorActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
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

    val context = LocalContext.current

    // Launcher for the ACTION_OPEN_DOCUMENT intent
    val openDocumentLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocument(),
        onResult = { uri ->
            val intent = Intent(context, MainActivity::class.java)
            intent.putExtra("ROM_URI", uri.toString())
            context.startActivity(intent)
        }
    )

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
                actions = {
                    IconButton(onClick = {
                        openDocumentLauncher.launch(arrayOf("*/*"))
                    }) {
                        Icon(
                            imageVector = Icons.Filled.FileOpen,
                            contentDescription = "Load ROM"
                        )
                    }
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
        EmulatorScreen(innerPadding)
    }
}

@Composable
fun EmulatorScreen(innerPadding: PaddingValues) {
    Box(
        modifier = Modifier.fillMaxHeight().padding(innerPadding)
    ) {
        Column {
            Viewport()
            Controls()
        }
    }
}

@Composable
fun Controls() {
    Column {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .height(IntrinsicSize.Max)
        ) {
            // D-Pad
            Box(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight() // Ensure the Box fills the height of the Row
                    .padding(10.dp)
            ) {
                Image(
                    painter = painterResource(R.drawable.button_dpad),
                    contentDescription = null,
                    modifier = Modifier
                        .aspectRatio(1f) // Ensures it stays square
                        .align(Alignment.Center),
                    contentScale = ContentScale.Fit
                )
            }

            // A/B Buttons
            Box(
                modifier = Modifier
                    .weight(1f) // Take up available space
                    .fillMaxHeight() // Fill the vertical space
            ) {
                // A Button
                Image(
                    painter = painterResource(R.drawable.button_a),
                    contentDescription = null,
                    modifier = Modifier
                        .align(Alignment.Center) // Align A button to the left center
                        .offset(x = 30.dp, y = (-15).dp),
                    contentScale = ContentScale.Fit
                )

                // B Button
                Image(
                    painter = painterResource(R.drawable.button_b),
                    contentDescription = null,
                    modifier = Modifier
                        .align(Alignment.Center) // Align B button to the right center
                        .offset(x = (-30).dp, y = 15.dp), // Offset to create spacing (half of button width)
                    contentScale = ContentScale.Fit
                )
            }
        }

        // Start/Select Buttons
        Box(
            modifier = Modifier.fillMaxSize()
        ) {
            Row(modifier = Modifier.align(Alignment.BottomCenter)) {
                Image(
                    painter = painterResource(R.drawable.startselect_button),
                    contentDescription = null,
                    contentScale = ContentScale.Fit,
                    modifier = Modifier.padding(10.dp).size(40.dp)
                )
                Image(
                    painter = painterResource(R.drawable.startselect_button),
                    contentDescription = null,
                    contentScale = ContentScale.Fit,
                    modifier = Modifier.padding(10.dp).size(40.dp)
                )
            }
        }
    }
}


@Composable
fun Viewport() {
    Image(
        painter = painterResource(R.drawable.no_rom_loaded),
        contentDescription = null,
        modifier = Modifier
            .fillMaxWidth(),
        contentScale = ContentScale.FillWidth
    )
}

@Preview(showBackground = true)
@Composable
fun EmulatorScreenPreview() {
    RetroGBmTheme {
        Content()
    }
}

@Preview(showBackground = true)
@Composable
fun ControlScreenPreview() {
    RetroGBmTheme {
        Controls()
    }
}