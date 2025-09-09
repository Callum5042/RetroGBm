package com.retrogbm.composables

import android.app.Activity
import android.content.Context
import android.content.res.Configuration
import android.view.MotionEvent
import android.widget.Toast
import androidx.activity.compose.BackHandler
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
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
import androidx.compose.material.icons.filled.FastForward
import androidx.compose.material.icons.filled.FileOpen
import androidx.compose.material.icons.filled.Image
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.RestartAlt
import androidx.compose.material.icons.filled.Science
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Stop
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.graphics.painter.BitmapPainter
import androidx.compose.ui.graphics.painter.Painter
import androidx.compose.ui.hapticfeedback.HapticFeedbackType
import androidx.compose.ui.input.pointer.PointerEventType
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.input.pointer.pointerInteropFilter
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.layout.onGloballyPositioned
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalHapticFeedback
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.core.graphics.createBitmap
import androidx.lifecycle.DefaultLifecycleObserver
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.lifecycle.viewmodel.compose.viewModel
import com.retrogbm.JoyPadButton
import com.retrogbm.R
import com.retrogbm.activities.EmulatorActivity
import com.retrogbm.models.EmulatorViewModel
import com.retrogbm.ui.theme.RetroGBmTheme
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.launch
import java.nio.IntBuffer

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun EmulatorMenuBar(
    viewModel: EmulatorViewModel = viewModel(),
    onQuickSave: () -> Unit = {},
    onQuickLoad: () -> Unit = {},
    onEmulationSpeed: () -> Unit = {},
    onLoadRom: () -> Unit = {},
    onSaveState: () -> Unit = {},
    onLoadState: () -> Unit = {},
    onRestart: () -> Unit = {},
    onStop: () -> Unit = {},
    onCheats: () -> Unit = {},
    onOptions: () -> Unit = {},
    onScreenshot: () -> Unit = {}

) {
    var showMenu by remember { mutableStateOf(false) }
    val activity = LocalContext.current as EmulatorActivity

    val sharedPreferences = remember {
        activity.getSharedPreferences("retrogbm_settings_prefs", Context.MODE_PRIVATE)
    }

    var emulationSpeed by remember {
        mutableFloatStateOf(sharedPreferences.getFloat("emulation_speed", 2.0f))
    }

    var emulationDoubleSpeed by remember {
        mutableStateOf(false)
    }

    TopAppBar(
        colors = TopAppBarDefaults.topAppBarColors(
            containerColor = Color.Black,
            titleContentColor = Color.White,
            navigationIconContentColor = Color.White,
            actionIconContentColor = Color.White,
        ),
        title = { Text("RetroGBm") },
        actions = {
            // Quick Save
            IconButton(onClick = {
                onQuickSave()
            }) {
                Icon(
                    painter = painterResource(id = R.drawable.baseline_save_24),
                    contentDescription = stringResource(id = R.string.quick_save)
                )
            }
            // Quick Load
            IconButton(onClick = {
                onQuickLoad()
            }) {
                Icon(
                    painter = painterResource(id = R.drawable.baseline_history_24),
                    contentDescription = stringResource(id = R.string.quick_load)
                )
            }
            // Emulation speed
            IconButton(onClick = {
                val speed = if (emulationDoubleSpeed) {
                    1.0f
                } else {
                    1.0f / emulationSpeed
                }

                viewModel.emulator.setEmulationSpeed(speed)
                emulationDoubleSpeed = !emulationDoubleSpeed
            }) {
                Icon(
                    imageVector = if (emulationDoubleSpeed) {
                        Icons.Filled.FastForward
                    } else {
                        Icons.Filled.PlayArrow
                    },
                    contentDescription = "Emulation Speed"
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

                var isPaused by remember { mutableStateOf(false) }

                // Dropdown menu
                DropdownMenu(
                    expanded = showMenu,
                    onDismissRequest = { showMenu = false },
                ) {
                    DropdownMenuItem(
                        leadingIcon = { Icon(Icons.Filled.FileOpen, null) },
                        text = { Text(stringResource(id = R.string.load_rom)) },
                        onClick = {
                            showMenu = false
                            onLoadRom()
                        }
                    )
                    HorizontalDivider()
                    DropdownMenuItem(
                        text = { Text(stringResource(id = R.string.save_state)) },
                        leadingIcon = {
                            Icon(
                                painter = painterResource(id = R.drawable.baseline_save_24),
                                contentDescription = stringResource(id = R.string.quick_save)
                            )
                        },
                        onClick = {
                            showMenu = false
                            onSaveState()
                        }
                    )
                    DropdownMenuItem(
                        text = { Text(stringResource(id = R.string.load_state)) },
                        leadingIcon = {
                            Icon(
                                painter = painterResource(id = R.drawable.baseline_history_24),
                                contentDescription = stringResource(id = R.string.quick_load)
                            )
                        },
                        onClick = {
                            showMenu = false
                            onLoadState()
                        }
                    )
                    HorizontalDivider()
                    DropdownMenuItem(
                        text = { Text("Pause") },
                        leadingIcon = { Icon(Icons.Filled.Pause, null) },
                        enabled = !isPaused,
                        onClick = {
                            showMenu = false
                            viewModel.emulator.pause()
                            isPaused = true
                        }
                    )
                    DropdownMenuItem(
                        text = { Text("Resume") },
                        leadingIcon = { Icon(Icons.Filled.PlayArrow, null) },
                        enabled = isPaused,
                        onClick = {
                            showMenu = false
                            viewModel.emulator.resume()
                            isPaused = false
                        }
                    )
                    DropdownMenuItem(
                        text = { Text("Restart") },
                        leadingIcon = { Icon(Icons.Filled.RestartAlt, null) },
                        onClick = {
                            showMenu = false
                            onRestart()
                        }
                    )
                    DropdownMenuItem(
                        text = { Text("Stop") },
                        leadingIcon = { Icon(Icons.Filled.Stop, null) },
                        onClick = {
                            showMenu = false
                            onStop()
                        }
                    )
                    HorizontalDivider()
                    DropdownMenuItem(
                        leadingIcon = { Icon(Icons.Filled.Science, null) },
                        text = { Text("Cheats") },
                        onClick = {
                            showMenu = false
                            onCheats()
                        }
                    )
                    DropdownMenuItem(
                        leadingIcon = { Icon(Icons.Filled.Image, null) },
                        text = { Text("Screenshot") },
                        onClick = {
                            showMenu = false
                            onScreenshot()
                        }
                    )
                    DropdownMenuItem(
                        text = { Text(stringResource(id = R.string.settings)) },
                        leadingIcon = { Icon(Icons.Filled.Settings, null) },
                        onClick = {
                            showMenu = false
                            onOptions()
                        }
                    )
                }
            }
        }
    )
}

@OptIn(ExperimentalComposeUiApi::class)
@Composable
fun EmulatorScreen(innerPadding: PaddingValues, viewModel: EmulatorViewModel = viewModel()) {
    Box(
        modifier = Modifier
            .fillMaxHeight()
            .background(Color(0xFF1B2328))
            .padding(innerPadding)
    ) {
        val configuration = LocalConfiguration.current

        val activity = LocalContext.current as EmulatorActivity
        val hapticFeedback = LocalHapticFeedback.current

        val sharedPreferences = remember {
            activity.getSharedPreferences("retrogbm_settings_prefs", Context.MODE_PRIVATE)
        }

        var enableHapticFeedback by remember {
            mutableStateOf(sharedPreferences.getBoolean("haptic_feedback", true))
        }

        val lifecycleOwner = LocalLifecycleOwner.current
        LaunchedEffect(lifecycleOwner) {
            lifecycleOwner.lifecycle.addObserver(object : DefaultLifecycleObserver {
                override fun onResume(owner: LifecycleOwner) {
                    enableHapticFeedback = sharedPreferences.getBoolean("haptic_feedback", true)
                }
            })
        }

        when (configuration.orientation) {
            Configuration.ORIENTATION_LANDSCAPE -> {
                // Landscape
                Row {
                    // Left column (auto width)
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
                        var activeButton by remember { mutableStateOf<JoyPadButton?>(null) }

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
                                                activeButton?.let {
                                                    viewModel.emulator.pressButton(
                                                        it,
                                                        false
                                                    )
                                                }

                                                // Activate the new button
                                                newButton?.let { viewModel.emulator.pressButton(it, true) }

                                                // Update the active button state
                                                activeButton = newButton

                                                if (enableHapticFeedback) {
                                                    hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
                                                }
                                            }
                                            true
                                        }

                                        MotionEvent.ACTION_UP -> {
                                            // Deactivate the current button on release
                                            activeButton?.let {
                                                viewModel.emulator.pressButton(it, false)

                                                if (enableHapticFeedback) {
                                                    hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
                                                }
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

                    // Middle column (Viewport)
                    Box(
                        modifier = Modifier
                            .weight(2f)
                            .fillMaxHeight()
                    ) {
                        Viewport()
                    }

                    // Right column (auto width)
                    // Spacer(modifier = Modifier.weight(1f))

                    // A/B Buttons
                    Box(
                        modifier = Modifier
                            .weight(1f) // Take up available space
                            .fillMaxHeight() // Fill the vertical space
                    ) {
                        // A Button
                        EmulatorButton(
                            painterResource(R.drawable.button_a),
                            onPress = {
                                viewModel.emulator.pressButton(JoyPadButton.A, true)
                            },
                            onRelease = {
                                viewModel.emulator.pressButton(JoyPadButton.A, false)
                            },
                            modifier = Modifier
                                .align(Alignment.Center)
                                .offset(
                                    x = 30.dp,
                                    y = (-15).dp
                                )
                        )

                        // B Button
                        EmulatorButton(
                            painterResource(R.drawable.button_b),
                            onPress = {
                                viewModel.emulator.pressButton(JoyPadButton.B, true)
                            },
                            onRelease = {
                                viewModel.emulator.pressButton(JoyPadButton.B, false)
                            },
                            modifier = Modifier
                                .align(Alignment.Center)
                                .offset(
                                    x = (-30).dp,
                                    y = 15.dp
                                )
                        )

                        // Start/Select Buttons
                        Box(
                            modifier = Modifier.fillMaxSize()
                        ) {
                            Row(modifier = Modifier.align(Alignment.BottomCenter)) {

                                // Select Button
                                EmulatorButton(
                                    painterResource(R.drawable.startselect_button),
                                    onPress = {
                                        viewModel.emulator.pressButton(JoyPadButton.Select, true)
                                    },
                                    onRelease = {
                                        viewModel.emulator.pressButton(JoyPadButton.Select, false)
                                    },
                                    modifier = Modifier
                                        .padding(10.dp)
                                        .size(40.dp)
                                )

                                // Start Button
                                EmulatorButton(
                                    painterResource(R.drawable.startselect_button),
                                    onPress = {
                                        viewModel.emulator.pressButton(JoyPadButton.Start, true)
                                    },
                                    onRelease = {
                                        viewModel.emulator.pressButton(JoyPadButton.Start, false)
                                    },
                                    modifier = Modifier
                                        .padding(10.dp)
                                        .size(40.dp)
                                )
                            }
                        }
                    }
                }
            }
            else -> {
                // Portrait
                Column {
                    Viewport()
                    EmulatorControlsPortrait()
                }
            }
        }
    }
}

private fun detectDirection(x: Float, y: Float, centerX: Float, centerY: Float): JoyPadButton? {
    return when {
        x < centerX * 0.8 -> JoyPadButton.Left // Left side
        x > centerX * 1.2 -> JoyPadButton.Right // Right side
        y < centerY * 0.8 -> JoyPadButton.Up // Top side
        y > centerY * 1.2 -> JoyPadButton.Down // Bottom side
        else -> null // Center or undefined region
    }
}

@OptIn(ExperimentalComposeUiApi::class)
@Composable
fun EmulatorControlsPortrait(viewModel: EmulatorViewModel = viewModel()) {

    val activity = LocalContext.current as EmulatorActivity
    val hapticFeedback = LocalHapticFeedback.current

    val sharedPreferences = remember {
        activity.getSharedPreferences("retrogbm_settings_prefs", Context.MODE_PRIVATE)
    }

    var enableHapticFeedback by remember {
        mutableStateOf(sharedPreferences.getBoolean("haptic_feedback", true))
    }

    val lifecycleOwner = LocalLifecycleOwner.current
    LaunchedEffect(lifecycleOwner) {
        lifecycleOwner.lifecycle.addObserver(object : DefaultLifecycleObserver {
            override fun onResume(owner: LifecycleOwner) {
                enableHapticFeedback = sharedPreferences.getBoolean("haptic_feedback", true)
            }
        })
    }

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
                var activeButton by remember { mutableStateOf<JoyPadButton?>(null) }

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
                                        activeButton?.let {
                                            viewModel.emulator.pressButton(
                                                it,
                                                false
                                            )
                                        }

                                        // Activate the new button
                                        newButton?.let { viewModel.emulator.pressButton(it, true) }

                                        // Update the active button state
                                        activeButton = newButton

                                        if (enableHapticFeedback) {
                                            hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
                                        }
                                    }
                                    true
                                }

                                MotionEvent.ACTION_UP -> {
                                    // Deactivate the current button on release
                                    activeButton?.let {
                                        viewModel.emulator.pressButton(it, false)

                                        if (enableHapticFeedback) {
                                            hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
                                        }
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
                EmulatorButton(
                    painterResource(R.drawable.button_a),
                    onPress = {
                        viewModel.emulator.pressButton(JoyPadButton.A, true)
                    },
                    onRelease = {
                        viewModel.emulator.pressButton(JoyPadButton.A, false)
                    },
                    modifier = Modifier
                        .align(Alignment.Center)
                        .offset(
                            x = 30.dp,
                            y = (-15).dp
                        )
                )

                // B Button
                EmulatorButton(
                    painterResource(R.drawable.button_b),
                    onPress = {
                        viewModel.emulator.pressButton(JoyPadButton.B, true)
                    },
                    onRelease = {
                        viewModel.emulator.pressButton(JoyPadButton.B, false)
                    },
                    modifier = Modifier
                        .align(Alignment.Center)
                        .offset(
                            x = (-30).dp,
                            y = 15.dp
                        )
                )
            }
        }

        // Start/Select Buttons
        Box(
            modifier = Modifier.fillMaxSize()
        ) {
            Row(modifier = Modifier.align(Alignment.BottomCenter)) {

                // Select Button
                EmulatorButton(
                    painterResource(R.drawable.startselect_button),
                    onPress = {
                        viewModel.emulator.pressButton(JoyPadButton.Select, true)
                    },
                    onRelease = {
                        viewModel.emulator.pressButton(JoyPadButton.Select, false)
                    },
                    modifier = Modifier
                        .padding(10.dp)
                        .size(40.dp)
                )

                // Start Button
                EmulatorButton(
                    painterResource(R.drawable.startselect_button),
                    onPress = {
                        viewModel.emulator.pressButton(JoyPadButton.Start, true)
                    },
                    onRelease = {
                        viewModel.emulator.pressButton(JoyPadButton.Start, false)
                    },
                    modifier = Modifier
                        .padding(10.dp)
                        .size(40.dp)
                )
            }
        }
    }
}

@Composable
fun EmulatorButton(
    painter: Painter,
    onPress: () -> Unit,
    onRelease:() -> Unit,
    modifier: Modifier
) {
    val activity = LocalContext.current as EmulatorActivity
    val hapticFeedback = LocalHapticFeedback.current

    val sharedPreferences = remember {
        activity.getSharedPreferences("retrogbm_settings_prefs", Context.MODE_PRIVATE)
    }

    var enableHapticFeedback by remember {
        mutableStateOf(sharedPreferences.getBoolean("haptic_feedback", true))
    }

    Image(
        painter = painter,
        contentDescription = null,
        contentScale = ContentScale.Fit,
        modifier = modifier
            .pointerInput(Unit) {
                awaitPointerEventScope {
                    while (true) {
                        val event = awaitPointerEvent()
                        if (event.type == PointerEventType.Press) {
                            onPress()
                            if (enableHapticFeedback) {
                                hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
                            }
                        } else if (event.type == PointerEventType.Release) {
                            onRelease()
                            if (enableHapticFeedback) {
                                hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
                            }
                        }
                    }
                }
            },
    )
}

@Preview(showBackground = true)
@Composable
fun EmulatorControlsPortraitPreview() {
    RetroGBmTheme {
        EmulatorControlsPortrait()
    }
}

@Composable
fun BackButtonHandler(viewModel: EmulatorViewModel = viewModel()) {
    val context = LocalContext.current
    var backButtonPressCount by remember { mutableIntStateOf(0) }
    var resetJob by remember { mutableStateOf<Job?>(null) }
    val scope = rememberCoroutineScope()

    BackHandler {
        backButtonPressCount++

        if (backButtonPressCount >= 2) {
            viewModel.stopEmulator()
            (context as? Activity)?.finish()
        } else {
            Toast.makeText(context, "Press Again To Quit", Toast.LENGTH_SHORT).show()

            // Cancel any previous reset job to avoid multiple coroutines
            resetJob?.cancel()
            resetJob = scope.launch {
                delay(2000)
                backButtonPressCount = 0
            }
        }
    }
}

@Composable
fun Viewport(viewModel: EmulatorViewModel = viewModel()) {
    // State to hold the Bitmap
    val bitmapFlow = remember {
        MutableStateFlow(createBitmap(160, 144))
    }

    // Update display on callback
    LaunchedEffect(Unit) {
        viewModel.emulator.displayOutput.onDraw { pixels ->
            val updatedBitmap = createBitmap(160, 144)
            IntBuffer.wrap(pixels).let { buffer -> updatedBitmap.copyPixelsFromBuffer(buffer) }
            bitmapFlow.value = updatedBitmap
        }
    }

    // Display image
    val bitmap = bitmapFlow.collectAsState()
    ViewportContent(bitmap.value.asImageBitmap())
}

@Composable
fun ViewportContent(image: ImageBitmap) {
    val configuration = LocalConfiguration.current
    when (configuration.orientation) {
        Configuration.ORIENTATION_LANDSCAPE -> {
            Image(
                painter = BitmapPainter(image),
                contentDescription = null,
                modifier = Modifier.fillMaxHeight(),
                contentScale = ContentScale.FillHeight
            )
        }
        else -> {
            Image(
                painter = BitmapPainter(image),
                contentDescription = null,
                modifier = Modifier.fillMaxWidth(),
                contentScale = ContentScale.FillWidth
            )
        }
    }
}

//@Preview(showBackground = true)
//@Composable
//fun EmulatorMenuBarPreview() {
//    RetroGBmTheme {
//        EmulatorMenuBar()
//    }
//}

@Preview(showBackground = true)
@Composable
fun ViewportPreview() {
    val previewBitmap = createBitmap(160, 144).apply {
        eraseColor(android.graphics.Color.RED)
    }

    RetroGBmTheme {
        ViewportContent(previewBitmap.asImageBitmap())
    }
}

@Preview(showBackground = true)
@Composable
fun EmulatorScreenPreview() {
    RetroGBmTheme {
        EmulatorScreen(
            PaddingValues(0.dp)
        )
    }
}