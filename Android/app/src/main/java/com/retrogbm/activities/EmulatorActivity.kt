package com.retrogbm.activities

import android.app.Activity
import com.retrogbm.composables.BackButtonHandler
import com.retrogbm.composables.EmulatorMenuBar
import com.retrogbm.composables.EmulatorScreen
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.net.Uri
import android.os.Bundle
import android.os.Environment
import android.provider.OpenableColumns
import android.util.Log
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.appcompat.app.AlertDialog
import androidx.compose.material3.Scaffold
import androidx.compose.ui.platform.LocalContext
import androidx.core.net.toUri
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.LifecycleObserver
import androidx.lifecycle.ProcessLifecycleOwner
import com.retrogbm.EmulatorWrapper
import com.retrogbm.LoggerWrapper
import com.retrogbm.models.EmulatorViewModel
import com.retrogbm.profile.ProfileGameData
import com.retrogbm.profile.ProfileRepository
import com.retrogbm.ui.theme.RetroGBmTheme
import com.retrogbm.utilities.SaveStateType
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.FileOutputStream
import java.nio.IntBuffer
import java.security.MessageDigest
import java.util.Date
import java.util.UUID
import java.util.concurrent.TimeUnit
import androidx.core.graphics.createBitmap
import com.retrogbm.CheatCode
import com.retrogbm.JoyPadButton
import com.retrogbm.profile.ProfileCheatCode
import kotlin.math.abs

class EmulatorActivity : ComponentActivity() {

    private val viewModel: EmulatorViewModel by viewModels()

    private lateinit var lifecycleObserver: LifecycleObserver

    private lateinit var fileName: String
    private lateinit var timeStarted: Date
    private lateinit var checksum: String

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Load the ROM from the Uri
        val romUriString = intent?.getStringExtra("ROM_URI")
        if (!romUriString.isNullOrEmpty()) {
            val romUri = romUriString.toUri()
            fileName = getFileName(this, romUri) ?: return

            if (!viewModel.isActive()) {
                loadRom(romUri, fileName)
            }
        }

        // Observe the app lifecycle
        configureLifecycle()

        // Find that last Quick Save number
        val quickSaveMap = mutableMapOf<Int, Long>()
        val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath

        for (i in 1..3) {
            val saveStatePath = absolutePath.let { "$it/SaveStates/$fileName/Quick Save $i.state" }
            val file = File(saveStatePath)
            if (file.exists()) {
                quickSaveMap[i] = readSaveStateHeader(file).dateModified
            }
        }

        val sortedByValue = quickSaveMap.toList().sortedBy { it.second }.toMap(LinkedHashMap())
        var slotNumber = if (sortedByValue.isNotEmpty()) sortedByValue.keys.last() else 1

        // Composable view
        setContent {
            RetroGBmTheme {
                Scaffold(
                    topBar = {
                        val context = LocalContext.current

                        // Paths
                        // TODO: This should probably live in viewModel?
                        var slotName = "Quick Save $slotNumber"
                        val absolutePath = context.getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath

                        // Launcher for the ACTION_OPEN_DOCUMENT intent
                        val openDocumentLauncher = rememberLauncherForActivityResult(
                            contract = ActivityResultContracts.OpenDocument(),
                            onResult = { uri ->
                                if (uri != null) {
                                    val currentActivity = context as? Activity
                                    currentActivity?.let { activity ->

                                        if (viewModel.emulator.isRunning()) {
                                            viewModel.emulator.stop()
                                        }

                                        activity.finish()
                                        val intent = Intent(activity, activity::class.java).apply {
                                            putExtra("ROM_URI", uri.toString())
                                            addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK)
                                        }

                                        activity.startActivity(intent)
                                    }
                                }
                            }
                        )

                        // Save state intent
                        val saveStateLauncher = rememberLauncherForActivityResult(
                            contract = ActivityResultContracts.StartActivityForResult()
                        ) { result ->
                            if (result.resultCode == RESULT_OK) {
                                // Retrieve data from the result Intent
                                val slotStr = result.data?.getStringExtra("Slot") ?: "Unknown"
                                val stateType = result.data?.getSerializableExtra("StateType") as SaveStateType

                                handleSaveState(
                                    viewModel.emulator,
                                    absolutePath!!,
                                    fileName,
                                    slotStr,
                                    stateType,
                                    context
                                )

                                val toastText = if (stateType == SaveStateType.Save) "State Saved" else "State Loaded"
                                Toast.makeText(context, toastText, Toast.LENGTH_SHORT).show()
                            }
                        }

                        EmulatorMenuBar(
                            onQuickSave = {
                                slotNumber++
                                if (slotNumber > 3) {
                                    slotNumber = 1
                                }

                                slotName = "Quick Save $slotNumber"
                                handleSaveState(
                                    viewModel.emulator,
                                    absolutePath!!,
                                    fileName,
                                    slotName,
                                    SaveStateType.Save,
                                    context
                                )

                                Toast.makeText(context, "State Saved", Toast.LENGTH_SHORT).show()
                            },
                            onQuickLoad = {
                                handleSaveState(
                                    viewModel.emulator,
                                    absolutePath!!,
                                    fileName,
                                    slotName,
                                    SaveStateType.Load,
                                    context
                                )

                                Toast.makeText(context, "State Loaded", Toast.LENGTH_SHORT).show()
                            },
                            onEmulationSpeed = {

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

                                    if (viewModel.emulator.isRunning()) {
                                        viewModel.emulator.stop()
                                    }

                                    activity.finish()
                                    val intent = Intent(activity, activity::class.java).apply {
                                        putExtra("ROM_TITLE", fileName)
                                        addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK)
                                    }

                                    activity.startActivity(intent)
                                }
                            },
                            onStop = {
                                if (viewModel.emulator.isRunning()) {
                                    viewModel.emulator.stop()
                                }

                                val intent = Intent(context, HomeActivity::class.java)
                                context.startActivity(intent)
                            },
                            onOptions = {
                                viewModel.emulator.pause()

                                val intent = Intent(context, OptionsActivity::class.java)
                                context.startActivity(intent)
                            },
                            onCheats = {
                                viewModel.emulator.pause()

                                val intent = Intent(context, CheatsActivity::class.java)
                                context.startActivity(intent)
                            },
                            onScreenshot = {
                                viewModel.emulator.pause()

                                val bitmap = createBitmap(160, 144)
                                val screenshotPath = absolutePath?.let { "$it/Screenshots" }
                                val guid = UUID.randomUUID().toString()

                                val screenshotFolder = File(screenshotPath!!)
                                if (!screenshotFolder.exists()) {
                                    screenshotFolder.mkdirs()
                                }

                                val file = File("$screenshotPath/$guid.png") // Save as PNG

                                val pixels = viewModel.emulator.getVideoBuffer()
                                val pixelsBuffer = IntBuffer.wrap(pixels.toList().toIntArray())
                                bitmap.copyPixelsFromBuffer(pixelsBuffer)

                                FileOutputStream(file).use { out ->
                                    bitmap.compress(Bitmap.CompressFormat.PNG, 100, out) // Compress and write
                                }

                                viewModel.emulator.resume()

                                Toast.makeText(context, "Screenshot Saved", Toast.LENGTH_SHORT).show()
                            }
                        )
                    }
                ) { innerPadding ->
                    EmulatorScreen(innerPadding)
                    BackButtonHandler()
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()

        if (isChangingConfigurations) {
            return
        }

        val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
        handleSaveState(viewModel.emulator, absolutePath!!, fileName, "AutoSave", SaveStateType.Save, this)
        Log.w("RetroGBm", "AutoSaved Successfully")

        if (viewModel.emulator.isRunning()) {
            viewModel.emulator.stop()
        }

        // Attempt to save the last played to the profile
        try {
            updateProfile()
        } catch (e: Exception) {
            Log.e("EmulatorActivity", "Unable to update profile", e)
            Toast.makeText(this, "Unable to update the profile", Toast.LENGTH_LONG).show()
        }

        ProcessLifecycleOwner.get().lifecycle.removeObserver(this.lifecycleObserver)
    }

    override fun onResume() {
        super.onResume()
        viewModel.emulator.resume()
    }

    override fun onPause() {
        super.onPause()
        viewModel.emulator.pause()
    }

    override fun onGenericMotionEvent(event: MotionEvent): Boolean {
        val isGamepad = event.source and InputDevice.SOURCE_JOYSTICK == InputDevice.SOURCE_JOYSTICK ||
                event.source and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD ||
                event.source and InputDevice.SOURCE_DPAD == InputDevice.SOURCE_DPAD

        if (event.action == MotionEvent.ACTION_MOVE && isGamepad) {

            // LEFT JOYSTICK
            val lx = getCenteredAxis(event, MotionEvent.AXIS_X)
            val ly = getCenteredAxis(event, MotionEvent.AXIS_Y)

            // DPAD HAT
            val hatX = event.getAxisValue(MotionEvent.AXIS_HAT_X)
            val hatY = event.getAxisValue(MotionEvent.AXIS_HAT_Y)

            // Combined joystick + dpad
            val finalX = if (hatX != 0f) hatX else lx
            val finalY = if (hatY != 0f) hatY else ly

            // Digital mapping
            viewModel.emulator.pressButton(JoyPadButton.Left,  finalX < -0.3f)
            viewModel.emulator.pressButton(JoyPadButton.Right, finalX >  0.3f)
            viewModel.emulator.pressButton(JoyPadButton.Up,    finalY < -0.3f)
            viewModel.emulator.pressButton(JoyPadButton.Down,  finalY >  0.3f)

            return true
        }

        return super.onGenericMotionEvent(event)
    }

    private fun getCenteredAxis(event: MotionEvent, axis: Int): Float {
        val range = event.device?.getMotionRange(axis, event.source)
        return if (range != null) {
            val value = event.getAxisValue(axis)
            if (abs(value) > range.flat) value else 0f
        } else 0f
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        when (keyCode) {
            KeyEvent.KEYCODE_BUTTON_A -> viewModel.emulator.pressButton(JoyPadButton.A, true)
            KeyEvent.KEYCODE_BUTTON_B -> viewModel.emulator.pressButton(JoyPadButton.B, true)
            KeyEvent.KEYCODE_BUTTON_Y -> viewModel.emulator.pressButton(JoyPadButton.A, true)
            KeyEvent.KEYCODE_BUTTON_X -> viewModel.emulator.pressButton(JoyPadButton.B, true)
            KeyEvent.KEYCODE_BUTTON_START -> viewModel.emulator.pressButton(JoyPadButton.Start, true)
            KeyEvent.KEYCODE_BUTTON_SELECT -> viewModel.emulator.pressButton(JoyPadButton.Select, true)

            else -> return super.onKeyDown(keyCode, event)
        }
        return true
    }

    override fun onKeyUp(keyCode: Int, event: KeyEvent): Boolean {
        when (keyCode) {
            KeyEvent.KEYCODE_BUTTON_A -> viewModel.emulator.pressButton(JoyPadButton.A, false)
            KeyEvent.KEYCODE_BUTTON_B -> viewModel.emulator.pressButton(JoyPadButton.B, false)
            KeyEvent.KEYCODE_BUTTON_Y -> viewModel.emulator.pressButton(JoyPadButton.A, false)
            KeyEvent.KEYCODE_BUTTON_X -> viewModel.emulator.pressButton(JoyPadButton.B, false)
            KeyEvent.KEYCODE_BUTTON_START -> viewModel.emulator.pressButton(JoyPadButton.Start, false)
            KeyEvent.KEYCODE_BUTTON_SELECT -> viewModel.emulator.pressButton(JoyPadButton.Select, false)

            else -> return super.onKeyUp(keyCode, event)
        }
        return true
    }

    private fun configureLifecycle() {
        val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
        var wasStopped = false
        this.lifecycleObserver = LifecycleEventObserver { _, event ->
            when (event) {
                Lifecycle.Event.ON_PAUSE -> {
                    if (viewModel.emulator.isRunning()) {
                        viewModel.emulator.pause()
                    }
                }
                Lifecycle.Event.ON_RESUME -> {
                    if (viewModel.emulator.isRunning()) {
                        viewModel.emulator.resume()
                    }

                    if (wasStopped) {
                        this.timeStarted = Date()
                    }
                }
                Lifecycle.Event.ON_STOP -> {
                    wasStopped = true
                    try {
                        handleSaveState(
                            viewModel.emulator,
                            absolutePath!!,
                            fileName,
                            "AutoSave",
                            SaveStateType.Save,
                            this
                        )
                        updateProfile()

                        Log.w("EmulatorActivity", "Activity Stopped")
                    } catch (e: Exception) {
                        Log.e("EmulatorActivity", "Unable to update profile", e)
                        Toast.makeText(this, "Unable to update the profile", Toast.LENGTH_LONG)
                            .show()
                    }
                }
                Lifecycle.Event.ON_DESTROY -> {
                    Log.w("EmulatorActivity", "Activity Destroyed")
                }
                else -> {}
            }
        }

        ProcessLifecycleOwner.get().lifecycle.addObserver(this.lifecycleObserver)
    }

    private fun loadRom(uri: Uri?, fileName: String) {
        val bytes = getRomBytes(uri!!)
        val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath

        LoggerWrapper().info("Loading ROM file: $absolutePath/$fileName")

        // Get the shared options
        val sharedPreferences = this.baseContext.getSharedPreferences(
            "retrogbm_settings_prefs",
            MODE_PRIVATE
        )

        // Set battery path and possible create the folder
        val batteryPath = absolutePath?.let { "$it/RomData" } ?: return
        val batteryPathFolder = File(batteryPath)
        if (!batteryPathFolder.exists()) {
            batteryPathFolder.mkdirs()
            Log.i("LoadROM", "Created folder $batteryPath")
        }

        // Get boot rom options
        val skipBootRom = sharedPreferences.getBoolean("skip_boot_rom", true)

        // Load ROM
        val batteryFilePath = batteryPath.let { "$it/$fileName.save" }
        viewModel.emulator.loadRom(bytes, batteryFilePath, skipBootRom)

        // Store fileName
        this.fileName = fileName
        this.timeStarted = Date()
        this.checksum = calculateFileChecksum(bytes)

        // Audio
        val enableSound = sharedPreferences.getBoolean("enable_sound", true)
        viewModel.emulator.soundOutput.toggleAudio(enableSound)

        // Load the cheats
        val profilePath = absolutePath.let { "$it/profile.json" }

        val profileRepository = ProfileRepository()
        val profileData = profileRepository.loadProfileData(profilePath)

        val profileGameData = profileData.gameData.find { p -> p.checksum == this.checksum }

        profileGameData?.cheats?.let { cheats ->
            val cheats = cheats.map { it ->
                CheatCode(
                    name = it.name,
                    code = it.code.split("\r\n").toTypedArray(),
                    enabled = false
                )
            }

            viewModel.emulator.setCheatCodes(cheats.toTypedArray())
        }

        // Emulator background thread
        viewModel.startEmulator()
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
        return when (uri.scheme) {
            "content" -> {
                context.contentResolver.query(uri, null, null, null, null)?.use { cursor ->
                    val nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                    if (nameIndex != -1 && cursor.moveToFirst()) {
                        cursor.getString(nameIndex)
                    } else {
                        null
                    }
                }
            }
            "file" -> {
                File(uri.path ?: return null).name
            }
            else -> null
        }
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
                fileName = fileName,
                cheats = mutableListOf()
            )

            profileData.gameData.add(profileGameData)
        }

        // Calculate play time
        val diffInMillis = Date().time - timeStarted.time
        val timeDifference = TimeUnit.MILLISECONDS.toMinutes(diffInMillis)

        profileGameData.lastPlayed = timeStarted
        profileGameData.totalPlayTimeMinutes += timeDifference.toInt()

        // Save cheats
        profileGameData.cheats = viewModel.emulator.getCheatCodes().map { it ->
            ProfileCheatCode(
                name = it.name,
                code = it.code.joinToString(separator = "\r\n")
            )
        }.toMutableList()

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

    private fun handleSaveState(emulator: EmulatorWrapper,
                                absolutePath: String,
                                fileName: String,
                                slotName: String,
                                stateType: SaveStateType,
                                context: Context) {
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
        try {
            if (stateType == SaveStateType.Save) {
                emulator.saveState(saveStatePath)
                Log.i("SaveState", "State saved to $saveStatePath")
            } else if (stateType == SaveStateType.Load) {
                emulator.loadState(saveStatePath)
                Log.i("SaveState", "State loaded from $saveStatePath")
            }
        }
        catch (ex: Exception) {
            val builder = AlertDialog.Builder(context)
            builder
                .setMessage(ex.message)
                .setTitle("Error")

            val dialog = builder.create()
            dialog.show()
        }
    }
}

