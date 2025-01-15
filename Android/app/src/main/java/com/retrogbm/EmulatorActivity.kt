package com.retrogbm

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.net.Uri
import android.os.Bundle
import android.os.Environment
import android.provider.OpenableColumns
import android.util.Log
import android.view.MotionEvent
import android.widget.Toast
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
import androidx.compose.material.icons.filled.MoreVert
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
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.graphics.painter.BitmapPainter
import androidx.compose.ui.hapticfeedback.HapticFeedbackType
import androidx.compose.ui.input.pointer.pointerInteropFilter
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.layout.onGloballyPositioned
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalHapticFeedback
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.LifecycleObserver
import androidx.lifecycle.ProcessLifecycleOwner
import com.retrogbm.profile.ProfileGameData
import com.retrogbm.profile.ProfileRepository
import com.retrogbm.ui.theme.RetroGBmTheme
import com.retrogbm.utilities.SaveStateType
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.ByteArrayOutputStream
import java.io.File
import java.nio.IntBuffer
import java.security.MessageDigest
import java.util.Date
import java.util.concurrent.TimeUnit

class EmulatorActivity : ComponentActivity() {

    private lateinit var fileName: String
    private lateinit var timeStarted: Date
    private lateinit var checksum: String

    // Emulator components
    private var emulator: EmulatorWrapper = EmulatorWrapper()

    // Coroutines
    private val emulatorCoroutineScope = CoroutineScope(Dispatchers.Main)
    private lateinit var emulatorThread: Job

    private lateinit var lifecycleObserver: LifecycleObserver

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Load the ROM from the intent set by the home page
        val title = intent.getStringExtra("ROM_TITLE")
        if (!title.isNullOrEmpty()) {
            val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
            val path = absolutePath!! + "/ROMS/" + title
            loadRom(Uri.fromFile(File(path)), title)
        }

        // Load the ROM from the ACTION_OPEN_DOCUMENT intent
        val romUriString = intent?.getStringExtra("ROM_URI")
        if (!romUriString.isNullOrEmpty()) {
            val romUri = romUriString.let { Uri.parse(it) }
            val romFileName = getFileName(this, romUri)
            loadRom(romUri, romFileName!!)
        }

        // Observe the app lifecycle
        var wasStopped = false
        this.lifecycleObserver = LifecycleEventObserver { _, event ->
            when (event) {
                Lifecycle.Event.ON_PAUSE -> {
                    if (emulator.isRunning()) {
                        emulator.pause()
                    }
                }
                Lifecycle.Event.ON_RESUME -> {
                    if (emulator.isRunning()) {
                        emulator.resume()
                    }

                    if (wasStopped) this.timeStarted = Date()
                }
                Lifecycle.Event.ON_STOP -> {
                    wasStopped = true
                    try {
                        updateProfile()
                    } catch (e: Exception) {
                        Log.w("EmulatorActivity", "Unable to update profile")
                        Toast.makeText(this, "Unable to update the profile", Toast.LENGTH_LONG).show()
                    }
                }
                Lifecycle.Event.ON_DESTROY -> {
                    println("App is being destroyed")
                }
                else -> {}
            }
        }

        ProcessLifecycleOwner.get().lifecycle.addObserver(this.lifecycleObserver)

        setContent {
            RetroGBmTheme {
                Content(emulator, fileName)
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()

        if (emulator.isRunning()) {
            emulator.stop()
            emulatorThread.cancel()
            emulatorCoroutineScope.cancel()
        }

        // Attempt to save the last played to the profile
        try {
            updateProfile()
        } catch (e: Exception) {
            Log.w("EmulatorActivity", "Unable to update profile")
            Toast.makeText(this, "Unable to update the profile", Toast.LENGTH_LONG).show()
        }

        ProcessLifecycleOwner.get().lifecycle.removeObserver(this.lifecycleObserver)
    }

    private fun updateProfile() {

        val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
        val profilePath = absolutePath?.let { "$it/profile.json" } ?: "profile.json"

        // Update the total time
        val profileRepository = ProfileRepository()
        val profileData = profileRepository.loadProfileData(profilePath)

        var profileGameData = profileData.gameData.find { p -> p.fileName == this.fileName }
        if (profileGameData == null) {
            profileGameData = ProfileGameData(
                name = this.fileName,
                checksum = this.checksum,
                lastPlayed = null,
                totalPlayTimeMinutes = 0,
                fileName = fileName
            )

            profileData.gameData.add(profileGameData)
        }

        // Calculate play time
        val diffInMillis = Date().time - timeStarted.time
        val timeDifference = TimeUnit.MILLISECONDS.toMinutes(diffInMillis)

        profileGameData.lastPlayed = timeStarted
        profileGameData.totalPlayTimeMinutes += timeDifference.toInt()

        // Attempt to save the last played to the profile
        profileRepository.saveProfileData(profilePath, profileData)
    }

    private fun calculateFileChecksum(bytes: ByteArray, algorithm: String = "SHA-256"): String {
        // Initialize MessageDigest with the chosen algorithm (SHA-256 by default)
        val messageDigest = MessageDigest.getInstance(algorithm)
        messageDigest.update(bytes)

        // Calculate the checksum and format as hexadecimal
        return messageDigest.digest().joinToString("") { "%02x".format(it) }
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

        // Store fileName
        this.fileName = fileName
        this.timeStarted = Date()
        this.checksum = calculateFileChecksum(bytes)

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

    private fun getFileName(context: Context, uri: Uri): String? {
        val cursor = context.contentResolver.query(uri, null, null, null, null)
        return cursor?.use {
            val nameIndex = it.getColumnIndex(OpenableColumns.DISPLAY_NAME)
            it.moveToFirst()
            it.getString(nameIndex)
        }
    }

    companion object {
        // Used to load the 'retrogbm' library on application startup.
        init {
            System.loadLibrary("retrogbm")
        }
    }
}

private fun handleSaveState(emulator: EmulatorWrapper,
                            absolutePath: String,
                            fileName: String,
                            slotName: String,
                            stateType: SaveStateType) {
    // Create path
    val saveStatePath = absolutePath.let { "$it/SaveStates/$fileName/$slotName.state" }

    // Make the missing directories
    val saveStateFolder = absolutePath.let { "$it/SaveStates/$fileName/" }
    val folder = File(saveStateFolder)
    if (!folder.exists()) {
        folder.mkdirs()
        Log.i("SaveState", "Created folder $saveStateFolder")
    }

    // Save or load
    if (stateType == SaveStateType.Save) {
        emulator.saveState(saveStatePath)
        Log.i("SaveState", "State saved to $saveStatePath")
    } else if (stateType == SaveStateType.Load) {
        emulator.loadState(saveStatePath)
        Log.i("SaveState", "State loaded from $saveStatePath")
    }
}

@Composable
fun Content(emulator: EmulatorWrapper, fileName: String) {

    val context = LocalContext.current

    // Paths
    val slotName = "quick"
    val absolutePath = context.getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath

    // Launcher for the ACTION_OPEN_DOCUMENT intent
    val openDocumentLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocument(),
        onResult = { uri ->
            val currentActivity = context as? Activity
            currentActivity?.let { activity ->

                if (emulator.isRunning()) {
                    emulator.stop()
                }

                activity.finish()
                val intent = Intent(activity, activity::class.java).apply {
                    putExtra("ROM_TITLE", uri.toString())
                    addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK)
                }

                activity.startActivity(intent)
            }
        }
    )

    // Save state intent
    val saveStateLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode == Activity.RESULT_OK) {
            // Retrieve data from the result Intent
            val slot = result.data?.getStringExtra("Slot") ?: "Unknown"
            val stateType = result.data?.getSerializableExtra("StateType") as SaveStateType

            handleSaveState(emulator, absolutePath!!, fileName, slot, stateType)
            Toast.makeText(context, "State Saved", Toast.LENGTH_SHORT).show()
        }
    }

    Scaffold(
        topBar = {
            AppTopBar(
                emulator,
                onQuickSave = {
                    handleSaveState(emulator, absolutePath!!, fileName, slotName, SaveStateType.Save)
                    Toast.makeText(context, "State Saved", Toast.LENGTH_SHORT).show()
                },
                onQuickLoad = {
                    handleSaveState(emulator, absolutePath!!, fileName, slotName, SaveStateType.Load)
                    Toast.makeText(context, "State Loaded", Toast.LENGTH_SHORT).show()
                },
                onLoadRom = {
                    openDocumentLauncher.launch(arrayOf("*/*"))
                },
                onSaveState = {
                    val intent = Intent(context, SaveStateActivity::class.java)
                    intent.putExtra("RomFileName", fileName)
                    intent.putExtra("StateType", SaveStateType.Save)
                    saveStateLauncher.launch(intent)
                },
                onLoadState = {
                    val intent = Intent(context, SaveStateActivity::class.java)
                    intent.putExtra("RomFileName", fileName)
                    intent.putExtra("StateType", SaveStateType.Load)
                    saveStateLauncher.launch(intent)
                },
                onRestart = {
                    val currentActivity = context as? Activity
                    currentActivity?.let { activity ->

                        if (emulator.isRunning()) {
                            emulator.stop()
                        }

                        activity.finish()
                        val intent = Intent(activity, activity::class.java).apply {
                            putExtra("ROM_TITLE", fileName)
                            addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK)
                        }

                        activity.startActivity(intent)
                    }
                },
                onHelp = {
                    Toast.makeText(context, "Options Clicked", Toast.LENGTH_SHORT).show()
                }
            )
        }
    ) { innerPadding ->
        EmulatorScreen(innerPadding, emulator)
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AppTopBar(
    emulator: EmulatorWrapper,
    onQuickSave: () -> Unit,
    onQuickLoad: () -> Unit,
    onLoadRom: () -> Unit,
    onSaveState: () -> Unit,
    onLoadState: () -> Unit,
    onRestart: () -> Unit,
    onHelp: () -> Unit
) {
    var showMenu by remember { mutableStateOf(false) }

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
            IconButton(onClick = onQuickSave) {
                Icon(
                    painter = painterResource(id = R.drawable.baseline_save_24),
                    contentDescription = stringResource(id = R.string.quick_save)
                )
            }
            // Quick Load
            IconButton(onClick = onQuickLoad) {
                Icon(
                    painter = painterResource(id = R.drawable.baseline_history_24),
                    contentDescription = stringResource(id = R.string.quick_load)
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
                        text = { Text(stringResource(id = R.string.load_rom)) },
                        onClick = {
                            showMenu = false
                            onLoadRom()
                        }
                    )
                    HorizontalDivider()
                    DropdownMenuItem(
                        text = { Text(stringResource(id = R.string.save_state)) },
                        onClick = {
                            showMenu = false
                            onSaveState()
                        }
                    )
                    DropdownMenuItem(
                        text = { Text(stringResource(id = R.string.load_state)) },
                        onClick = {
                            showMenu = false
                            onLoadState()
                        }
                    )
                    HorizontalDivider()
                    DropdownMenuItem(
                        text = { Text("Pause") },
                        enabled = !isPaused,
                        onClick = {
                            showMenu = false
                            emulator.pause()
                            isPaused = true
                        }
                    )
                    DropdownMenuItem(
                        text = { Text("Resume") },
                        enabled = isPaused,
                        onClick = {
                            showMenu = false
                            emulator.resume()
                            isPaused = false
                        }
                    )
                    DropdownMenuItem(
                        text = { Text("Restart") },
                        onClick = {
                            showMenu = false
                            onRestart()
                        }
                    )
                    HorizontalDivider()
                    DropdownMenuItem(
                        text = { Text(stringResource(id = R.string.settings)) },
                        onClick = {
                            showMenu = false
                            onHelp()
                        }
                    )
                }
            }
        }
    )
}

@Composable
fun EmulatorScreen(innerPadding: PaddingValues, emulator: EmulatorWrapper) {
    Box(
        modifier = Modifier
            .fillMaxHeight()
            .padding(innerPadding)
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
private fun detectDirection(x: Float, y: Float, centerX: Float, centerY: Float): JoyPadButton? {
    return when {
        x < centerX * 0.5 -> JoyPadButton.Left // Left side
        x > centerX * 1.5 -> JoyPadButton.Right // Right side
        y < centerY * 0.5 -> JoyPadButton.Up // Top side
        y > centerY * 1.5 -> JoyPadButton.Down // Bottom side
        else -> null // Center or undefined region
    }
}

@OptIn(ExperimentalComposeUiApi::class)
@Composable
fun Controls(emulator: EmulatorWrapper) {

    val hapticFeedback = LocalHapticFeedback.current

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
                                        activeButton?.let { emulator.pressButton(it, false) }

                                        // Activate the new button
                                        newButton?.let { emulator.pressButton(it, true) }

                                        // Update the active button state
                                        activeButton = newButton

                                        hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
                                    }
                                    true
                                }

                                MotionEvent.ACTION_UP -> {
                                    // Deactivate the current button on release
                                    activeButton?.let {
                                        emulator.pressButton(it, false)
                                        hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
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
                                    emulator.pressButton(JoyPadButton.A, true)
                                    hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
                                    true
                                }

                                MotionEvent.ACTION_UP -> {
                                    emulator.pressButton(JoyPadButton.A, false)
                                    hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
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
                        .offset(
                            x = (-30).dp,
                            y = 15.dp
                        ) // Offset to create spacing (half of button width)
                        .pointerInteropFilter {
                            when (it.action) {
                                MotionEvent.ACTION_DOWN -> {
                                    emulator.pressButton(JoyPadButton.B, true)
                                    hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
                                    true
                                }

                                MotionEvent.ACTION_UP -> {
                                    emulator.pressButton(JoyPadButton.B, false)
                                    hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
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
                                    emulator.pressButton(JoyPadButton.Select, true)
                                    hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
                                    true
                                }

                                MotionEvent.ACTION_UP -> {
                                    emulator.pressButton(JoyPadButton.Select, false)
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
                                    emulator.pressButton(JoyPadButton.Start, true)
                                    hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
                                    true
                                }

                                MotionEvent.ACTION_UP -> {
                                    emulator.pressButton(JoyPadButton.Start, false)
                                    hapticFeedback.performHapticFeedback(HapticFeedbackType.LongPress)
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
fun ContentPreview() {
    RetroGBmTheme {
        Content(EmulatorWrapper(), "test.gbc")
    }
}