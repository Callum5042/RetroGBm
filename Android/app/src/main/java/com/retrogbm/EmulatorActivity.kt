package com.retrogbm

import android.content.Intent
import android.graphics.Bitmap
import android.net.Uri
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.view.MotionEvent
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
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
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.graphics.painter.BitmapPainter
import androidx.compose.ui.hapticfeedback.HapticFeedbackType
import androidx.compose.ui.input.pointer.pointerInteropFilter
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.layout.onGloballyPositioned
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalHapticFeedback
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.retrogbm.ui.theme.RetroGBmTheme
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.ByteArrayOutputStream
import java.io.File
import java.nio.IntBuffer

class EmulatorActivity : ComponentActivity() {

    // Emulator components
    private var emulator: EmulatorWrapper = EmulatorWrapper()

    // Coroutines
    private val emulatorCoroutineScope = CoroutineScope(Dispatchers.Main)
    private lateinit var emulatorThread: Job

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Load the ROM from the intent set by the home page
        val title = intent.getStringExtra("ROM_TITLE")
        if (!title.isNullOrEmpty()) {
            val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
            val path = absolutePath!! + "/ROMS/" + title
            loadRom(Uri.fromFile(File(path)), title)
        }

        setContent {
            RetroGBmTheme {
                Content(emulator)
            }
        }
    }

    private fun loadRom(uri: Uri?, fileName: String) {
        val bytes = getRomBytes(uri!!)
        val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath

        // Set battery path and possible create the folder
        val batteryPath = absolutePath?.let { "$it/RomData" }!!
        val batteryPathFolder = File(batteryPath)
        if (!batteryPathFolder.exists()) {
            batteryPathFolder.mkdirs()
            Log.i("LoadROM", "Created folder $batteryPath")
        }

        // Load ROM
        val batteryFilePath = batteryPath.let { "$it/$fileName.save" }
        emulator.loadRom(bytes, batteryFilePath)

        // Emulator background thread
        emulatorThread = emulatorCoroutineScope.launch(Dispatchers.Default) {
            while (emulator.isRunning()) {
                emulator.tick()
            }
        }
    }

    private fun getRomBytes(uri: Uri): ByteArray {
        val inputStream = contentResolver.openInputStream(uri)
        val outputStream = ByteArrayOutputStream()
        inputStream?.use { stream ->
            val buffer = ByteArray(1024)
            var length: Int

            while (stream.read(buffer).also { length = it } != -1) {
                outputStream.write(buffer, 0, length)
            }
        }

        return outputStream.toByteArray()
    }

    companion object {
        // Used to load the 'retrogbm' library on application startup.
        init {
            System.loadLibrary("retrogbm")
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun Content(emulator: EmulatorWrapper) {

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
        EmulatorScreen(innerPadding, emulator)
    }
}

@Composable
fun EmulatorScreen(innerPadding: PaddingValues, emulator: EmulatorWrapper) {
    Box(
        modifier = Modifier.fillMaxHeight().padding(innerPadding)
    ) {
        Column {
            Viewport(emulator)
            Controls(emulator)
        }
    }
}

@Composable
fun Viewport(emulator: EmulatorWrapper) {

    // State to hold the Bitmap
    val bitmapState = remember {
        mutableStateOf(Bitmap.createBitmap(160, 144, Bitmap.Config.ARGB_8888))
    }

    // Launch a coroutine to update the bitmap in the background
    LaunchedEffect(emulator) {
        val updateTextureCoroutineScope = CoroutineScope(Dispatchers.Default)
        updateTextureCoroutineScope.launch {
            while (emulator.isRunning()) {
                // Run the background work on a different thread
                val pixels = withContext(Dispatchers.Default) {
                    emulator.getVideoBuffer() // Get the video buffer on a background thread
                }

                // Update the Bitmap on the main thread
                withContext(Dispatchers.Main) {
                    val updatedBitmap = Bitmap.createBitmap(160, 144, Bitmap.Config.ARGB_8888)
                    val pixelsBuffer = IntBuffer.wrap(pixels.toList().toIntArray())
                    updatedBitmap.copyPixelsFromBuffer(pixelsBuffer)
                    
                    bitmapState.value = updatedBitmap
                }
            }
        }
    }

    Image(
        painter = BitmapPainter(bitmapState.value.asImageBitmap()),
        contentDescription = null,
        modifier = Modifier
            .fillMaxWidth(),
        contentScale = ContentScale.FillWidth
    )
}

// Helper function to detect direction
private fun detectDirection(x: Float, y: Float, centerX: Float, centerY: Float): JoypadButton? {
    return when {
        x < centerX * 0.5 -> JoypadButton.Left // Left side
        x > centerX * 1.5 -> JoypadButton.Right // Right side
        y < centerY * 0.5 -> JoypadButton.Up // Top side
        y > centerY * 1.5 -> JoypadButton.Down // Bottom side
        else -> null // Center or undefined region
    }
}

@OptIn(ExperimentalComposeUiApi::class)
@Composable
fun Controls(emulator: EmulatorWrapper) {
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
                    .fillMaxHeight()
                    .padding(10.dp)
            ) {
                // States to store the dimensions of the D-Pad
                var centerX by remember { mutableFloatStateOf(0f) }
                var centerY by remember { mutableFloatStateOf(0f) }

                // State to track the currently active button
                var activeButton by remember { mutableStateOf<JoypadButton?>(null) }

                Image(
                    painter = painterResource(R.drawable.button_dpad),
                    contentDescription = null,
                    modifier = Modifier
                        .aspectRatio(1f)
                        .align(Alignment.Center)
                        .onGloballyPositioned { coordinates ->
                            centerX = coordinates.size.width / 2f
                            centerY = coordinates.size.height / 2f
                        }
                        .pointerInteropFilter { event ->
                            val x = event.x
                            val y = event.y

                            when (event.action) {
                                MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE -> {
                                    val newButton = detectDirection(x, y, centerX, centerY)
                                    if (newButton != activeButton) {
                                        // Deactivate the previous button
                                        activeButton?.let { emulator.pressButton(it, false) }

                                        // Activate the new button
                                        newButton?.let { emulator.pressButton(it, true) }

                                        // Update the active button state
                                        activeButton = newButton
                                    }
                                    true
                                }

                                MotionEvent.ACTION_UP -> {
                                    // Deactivate the current button on release
                                    activeButton?.let {
                                        emulator.pressButton(it, false)
                                    }
                                    activeButton = null
                                    true
                                }

                                else -> false
                            }
                        },
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
                        .offset(x = 30.dp, y = (-15).dp)
                        .pointerInteropFilter {
                            when (it.action) {
                                MotionEvent.ACTION_DOWN -> {
                                    emulator.pressButton(JoypadButton.A, true)
                                    true
                                }
                                MotionEvent.ACTION_UP -> {
                                    emulator.pressButton(JoypadButton.A, false)
                                    true
                                }
                                else -> false
                            }
                        },
                    contentScale = ContentScale.Fit
                )

                // B Button
                Image(
                    painter = painterResource(R.drawable.button_b),
                    contentDescription = null,
                    modifier = Modifier
                        .align(Alignment.Center) // Align B button to the right center
                        .offset(x = (-30).dp, y = 15.dp) // Offset to create spacing (half of button width)
                        .pointerInteropFilter {
                            when (it.action) {
                                MotionEvent.ACTION_DOWN -> {
                                    emulator.pressButton(JoypadButton.B, true)
                                    true
                                }
                                MotionEvent.ACTION_UP -> {
                                    emulator.pressButton(JoypadButton.B, false)
                                    true
                                }
                                else -> false
                            }
                        },
                    contentScale = ContentScale.Fit
                )
            }
        }

        // Start/Select Buttons
        Box(
            modifier = Modifier.fillMaxSize()
        ) {
            Row(modifier = Modifier.align(Alignment.BottomCenter)) {

                val hapticFeedback = LocalHapticFeedback.current

                Image(
                    painter = painterResource(R.drawable.startselect_button),
                    contentDescription = null,
                    contentScale = ContentScale.Fit,
                    modifier = Modifier
                        .padding(10.dp)
                        .size(40.dp)
                        .pointerInteropFilter {
                            when (it.action) {
                                MotionEvent.ACTION_DOWN -> {
                                    emulator.pressButton(JoypadButton.Select, true)
                                    hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
                                    true
                                }
                                MotionEvent.ACTION_UP -> {
                                    emulator.pressButton(JoypadButton.Select, false)
                                    hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
                                    true
                                }
                                else -> false
                            }
                        }
                )

                Image(
                    painter = painterResource(R.drawable.startselect_button),
                    contentDescription = null,
                    contentScale = ContentScale.Fit,
                    modifier = Modifier
                        .padding(10.dp)
                        .size(40.dp)
                        .pointerInteropFilter {
                            when (it.action) {
                                MotionEvent.ACTION_DOWN -> {
                                    emulator.pressButton(JoypadButton.Start, true)
                                    true
                                }
                                MotionEvent.ACTION_UP -> {
                                    emulator.pressButton(JoypadButton.Start, false)
                                    true
                                }
                                else -> false
                            }
                        }
                )
            }
        }
    }
}

@Preview(showBackground = true)
@Composable
fun ControlScreenPreview() {
    RetroGBmTheme {
        // Controls()
    }
}