package com.retrogbm

import android.app.Activity
import android.app.AlertDialog
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.Rect
import android.net.Uri
import android.os.Bundle
import android.os.Environment
import android.provider.OpenableColumns
import android.util.Log
import android.view.HapticFeedbackConstants
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import android.view.MotionEvent
import android.view.View
import android.widget.ImageButton
import android.widget.ImageView
import android.widget.Toast
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.retrogbm.databinding.ActivityMainBinding
import com.retrogbm.profile.ProfileGameData
import com.retrogbm.profile.ProfileRepository
import com.retrogbm.utilities.SaveStateType
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.ByteArrayOutputStream
import java.io.File
import java.nio.IntBuffer
import java.security.MessageDigest
import java.util.Date
import java.util.concurrent.TimeUnit

class MainActivity : AppCompatActivity() {

    // Coroutines
    private val emulatorCoroutineScope = CoroutineScope(Dispatchers.Main)
    private val updateTextureCoroutineScope = CoroutineScope(Dispatchers.Main)

    private lateinit var emulatorThread: Job
    private lateinit var updateTextureThread: Job

    // UI components
    private lateinit var binding: ActivityMainBinding
    private lateinit var image: ImageView
    private lateinit var resultLauncher: ActivityResultLauncher<Intent>

    // Emulator components
    private var emulator: EmulatorWrapper = EmulatorWrapper()
    private val bitmap: Bitmap = Bitmap.createBitmap(160, 144, Bitmap.Config.ARGB_8888)

    private var romName: String? = null
    private var fileName: String? = null

    private lateinit var timeStarted: Date

    // Dependency
    private val profileRepository = ProfileRepository()


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        image = findViewById(R.id.ivEmulator)

        val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath

        // Buttons
        registerButtons()

        // Load the ROM from the intent set by the home page
        val title = intent.getStringExtra("ROM_TITLE")
        if (!title.isNullOrEmpty()) {
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

        // SaveState intent result
        resultLauncher = registerForActivityResult(
            ActivityResultContracts.StartActivityForResult()
        ) { result ->
            if (result.resultCode == Activity.RESULT_OK) {
                // Get intent data
                val data: Intent? = result.data
                val slot = data?.getIntExtra("Slot", -1)
                val stateType = data?.getIntExtra("StateType", 0)

                val saveStateType = if (stateType == 1) SaveStateType.Save else SaveStateType.Load
                lifecycleScope.launch(Dispatchers.IO) {
                    handleSaveState("slot$slot", saveStateType)
                }
            }
        }
    }

    private fun handleSaveState(slotName: String, stateType: SaveStateType) {
        // Create path
        val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
        val saveStatePath = absolutePath?.let { "$it/SaveStates/$fileName/$slotName.state" } ?: "$slotName.state"

        // Make the missing directories
        val saveStateFolder = absolutePath?.let { "$it/SaveStates/$fileName/" }
        val folder = File(saveStateFolder!!)
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

    private fun getFileName(context: Context, uri: Uri): String? {
        val cursor = context.contentResolver.query(uri, null, null, null, null)
        return cursor?.use {
            val nameIndex = it.getColumnIndex(OpenableColumns.DISPLAY_NAME)
            it.moveToFirst()
            it.getString(nameIndex)
        }
    }

    override fun onPause() {
        super.onPause()

        if (emulator.isRunning()) {
            emulator.pause()
        }
    }

    override fun onStop() {
        super.onStop()

        // Update the total time
        if (emulator.isRunning()) {

            val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
            val path = absolutePath?.let { "$it/profile.json" } ?: "profile.json"

            val profileData = profileRepository.loadProfileData(path)
            val profileGameData = profileData.gameData.find { p -> p.name == romName }

            val diffInMillis = Date().time - timeStarted.time
            val timeDifference = TimeUnit.MILLISECONDS.toMinutes(diffInMillis)

            profileGameData!!.totalPlayTimeMinutes += timeDifference.toInt()
            profileGameData?.fileName = fileName!!
            profileRepository.saveProfileData(path, profileData)

            wasStopped = true
        }
    }

    private var wasStopped = false

    override fun onResume() {
        super.onResume()

        if (romName != null && wasStopped) {
            timeStarted = Date()
            wasStopped = false
        }

        if (emulator.isRunning()) {
            emulator.resume()
        }
    }

    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        val inflater: MenuInflater = menuInflater
        inflater.inflate(R.menu.game_menu, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.quick_save -> {
                handleSaveState("quick", SaveStateType.Save)
                Toast.makeText(this, "State Saved", Toast.LENGTH_SHORT).show()
                return true
            }
            R.id.quick_load -> {
                handleSaveState("quick", SaveStateType.Load)
                Toast.makeText(this, "State Loaded", Toast.LENGTH_SHORT).show()
                return true
            }
            R.id.load_rom -> {
                loadRom()
                true
            }
            R.id.save_state -> {
                if (!romName.isNullOrEmpty()){
                    val intent = Intent(this, SaveStateActivity::class.java)
                    intent.putExtra("RomFileName", fileName)
                    intent.putExtra("StateType", 1)
                    resultLauncher.launch(intent)
                }

                true
            }
            R.id.load_state -> {
                if (!romName.isNullOrEmpty()){
                    val intent = Intent(this, SaveStateActivity::class.java)
                    intent.putExtra("RomFileName", fileName)
                    intent.putExtra("StateType", 2)
                    resultLauncher.launch(intent)
                }

                true
            }
            R.id.help -> {
                Toast.makeText(this, "Settings", Toast.LENGTH_SHORT).show()
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }

    private val openDocumentLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
        if (result.resultCode == Activity.RESULT_OK) {
            val uri: Uri? = result.data?.data
            try {
                loadRom(uri, "hmmm")
            }
            catch (e: Exception){
                Log.d("RetroGBm", e.message.toString())
            }
        }
    }

    private fun loadRom(uri: Uri?, fileName: String) {
        val inputStream = contentResolver.openInputStream(uri!!)
        val outputStream = ByteArrayOutputStream()
        inputStream?.use { stream ->
            val buffer = ByteArray(1024)
            var length: Int

            while (stream.read(buffer).also { length = it } != -1) {
                outputStream.write(buffer, 0, length)
            }
        }

        val bytes = outputStream.toByteArray()

        val absolutePath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
        val profilePath = absolutePath?.let { "$it/profile.json" } ?: "profile.json"

        if (emulator.isRunning()) {
            emulator.stop()
            emulatorThread.cancel()
            updateTextureThread.cancel()

            // Update the total time
            val profileData = profileRepository.loadProfileData(profilePath)
            val profileGameData = profileData.gameData.find { p -> p.name == romName }

            val diffInMillis = Date().time - timeStarted.time
            val timeDifference = TimeUnit.MILLISECONDS.toMinutes(diffInMillis)

            profileGameData?.fileName = fileName
            profileGameData!!.totalPlayTimeMinutes += timeDifference.toInt()
            profileRepository.saveProfileData(profilePath, profileData)
        }

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
        startEmulation()

        // Update the profile
        val profileData = profileRepository.loadProfileData(profilePath)

        // Find the game data
        romName = emulator.getCartridgeTitle()
        var profileGameData = profileData.gameData.find { p -> p.name == romName }
        if (profileGameData == null) {
            profileGameData = ProfileGameData(
                name = romName!!,
                checksum = calculateFileChecksum(bytes),
                lastPlayed = null,
                totalPlayTimeMinutes = 0,
                fileName = fileName
            )

            profileData.gameData.add(profileGameData)
        }

        timeStarted = Date()
        profileGameData.lastPlayed = Date()
        profileGameData.fileName = fileName

        // Attempt to save the last played to the profile
        try {
            profileRepository.saveProfileData(profilePath, profileData)
        } catch (e: Exception) {
            Log.w("MainActivity", "Unable to update profile")
            Toast.makeText(this, "Unable to update the profile", Toast.LENGTH_LONG).show()
        }

        this.fileName = fileName
    }

    private fun calculateFileChecksum(bytes: ByteArray, algorithm: String = "SHA-256"): String {
        // Initialize MessageDigest with the chosen algorithm (SHA-256 by default)
        val messageDigest = MessageDigest.getInstance(algorithm)
        messageDigest.update(bytes)

        // Calculate the checksum and format as hexadecimal
        return messageDigest.digest().joinToString("") { "%02x".format(it) }
    }

    private fun loadRom() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT)
        intent.type = "*/*"
        openDocumentLauncher.launch(intent)
    }

    private fun stopEmulator() {
        emulator.stop()
        emulatorThread.cancel()
        updateTextureThread.cancel()
    }

    private fun startEmulation() {
        // Emulator background thread
        emulatorThread = emulatorCoroutineScope.launch(Dispatchers.Default) {
            while (emulator.isRunning()) {
                emulator.tick()
            }
        }

        // Emulator update thread
        updateTextureThread = updateTextureCoroutineScope.launch(Dispatchers.Default) {
            while (emulator.isRunning()) {
                withContext(Dispatchers.Main) {
                    val pixels = emulator.getVideoBuffer()
                    setColours(pixels.toList())
                }
            }
        }
    }

    private fun registerButtons() {

        val buttonDPad = findViewById<ImageButton>(R.id.btnDPad)
        var currentSection = 0

        buttonDPad.setOnTouchListener(object: View.OnTouchListener {
            override fun onTouch(v: View?, event: MotionEvent?): Boolean {

                // Pointer coordinates relative to View
                val x = event?.x?.toInt()
                val y = event?.y?.toInt()

                // Rect hitbox and calculate sub-sections
                val rect: Rect = Rect()
                buttonDPad.getHitRect(rect)
                rect.offsetTo(0, 0)
                val subRects = subdivideRectInto9(rect)

                when (event?.action) {
                    MotionEvent.ACTION_MOVE -> {
                        // Check which subsection we are in
                        for (i in subRects.indices) {
                            if (subRects[i].contains(x!!, y!!)) {
                                // We only care about 1, 3, 5, 7
                                if (i == 1 || i == 3 || i == 5 || i == 7) {
                                    if (currentSection != i) {
                                        val buttonIndex = selectedButton(currentSection)
                                        if (buttonIndex != null) {
                                            emulator.pressButton(buttonIndex, false)
                                            v?.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY, HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING)
                                            Log.d("RetroGBm", "DPad section ${buttonIndex} up")
                                        }

                                        currentSection = i

                                        val buttonIndexDown = selectedButton(currentSection)
                                        if (buttonIndexDown != null) {
                                            emulator.pressButton(buttonIndexDown, true)
                                            v?.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY_RELEASE, HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING)
                                            Log.d("RetroGBm", "DPad section ${buttonIndexDown} down"                                            )
                                        }
                                    }
                                }
                            }
                        }

                        if (!rect.contains(x!!, y!!)) {
                            if (currentSection != -1) {
                                val buttonIndex = selectedButton(currentSection)
                                emulator.pressButton(buttonIndex!!, false)
                                v?.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY_RELEASE, HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING)
                                Log.d("RetroGBm", "DPad left  ${buttonIndex} up")
                                currentSection = -1
                            }
                        }
                    }
                    MotionEvent.ACTION_DOWN -> {
                        // Check which subsection we are in
                        for (i in subRects.indices) {
                            if (subRects[i].contains(x!!, y!!)) {
                                if (i == 1 || i == 3 || i == 5 || i == 7) {
                                    currentSection = i

                                    val buttonIndex = selectedButton(currentSection)
                                    if (buttonIndex != null){
                                        emulator.pressButton(buttonIndex, true)
                                        v?.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY, HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING)
                                        Log.d("RetroGBm", "DPad section ${buttonIndex} down")
                                    }
                                }
                            }
                        }
                    }
                    MotionEvent.ACTION_UP -> {
                        // Check which subsection we are in
                        val subRects = subdivideRectInto9(rect)
                        for (i in subRects.indices) {
                            if (subRects[i].contains(x!!, y!!)) {
                                val buttonIndex = selectedButton(currentSection)
                                if (buttonIndex != null){
                                    emulator.pressButton(buttonIndex, false)
                                    v?.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY_RELEASE, HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING)
                                    Log.d("RetroGBm", "DPad section ${buttonIndex} up")
                                }
                            }
                        }
                    }
                }

                return v?.onTouchEvent(event) ?: true
            }
        })

        val buttonA = findViewById<ImageButton>(R.id.btnButtonA)
        setButtonTouchListener(buttonA, 0)

        val buttonB = findViewById<ImageButton>(R.id.btnButtonB)
        setButtonTouchListener(buttonB, 1)

        val buttonStart = findViewById<ImageButton>(R.id.btnButtonStart)
        setButtonTouchListener(buttonStart, 2)

        val buttonSelect = findViewById<ImageButton>(R.id.btnButtonSelect)
        setButtonTouchListener(buttonSelect, 3)
    }

    fun selectedButton(currentSection: Int): Int? {
        when (currentSection) {
            1 -> return 4 // Up
            3 -> return 6 // Left
            5 -> return 7 // Right
            7 -> return 5  // Down
        }
        return null
    }

    fun subdivideRectInto9(rect: Rect): List<Rect> {
        val width = rect.width() / 3
        val height = rect.height() / 3
        val subRects = mutableListOf<Rect>()

        for (i in 0 until 3) {
            for (j in 0 until 3) {
                val left = rect.left + j * width
                val top = rect.top + i * height
                val right = left + width
                val bottom = top + height
                subRects.add(Rect(left, top, right, bottom))
            }
        }

        return subRects
    }

    private fun setButtonTouchListener(button: View, btn: Int) {
        button.setOnTouchListener(object : View.OnTouchListener {
            override fun onTouch(view: View?, event: MotionEvent?): Boolean {
                Log.d("RetroGBm", "A button has been touched indeed")
                when (event?.action) {
                    MotionEvent.ACTION_DOWN -> {
                        // Handle touch down event
                        // This is triggered when the user first touches the screen
                        emulator.pressButton(btn, true)
                        view?.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY, HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING)
                        return true
                    }
                    MotionEvent.ACTION_UP -> {
                        // Handle touch up event
                        // This is triggered when the user releases the touch
                        emulator.pressButton(btn, false)
                        view?.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY_RELEASE, HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING)
                        return true
                    }
                    // You can handle other MotionEvent actions as needed
                    // For example: MotionEvent.ACTION_MOVE for movement events

                    else -> return false
                }
            }
        })
    }

    private fun setColours(pixels: List<Int>) {
        val pixelsBuffer = IntBuffer.wrap(pixels.toIntArray())
        bitmap.copyPixelsFromBuffer(pixelsBuffer)
        image.setImageBitmap(bitmap)
    }

    companion object {
        // Used to load the 'retrogbm' library on application startup.
        init {
            System.loadLibrary("retrogbm")
        }
    }
}