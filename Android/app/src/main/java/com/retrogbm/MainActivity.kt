package com.retrogbm

import android.app.Activity
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.Color
import android.net.Uri
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Environment
import android.provider.DocumentsContract
import android.util.Log
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import android.view.MotionEvent
import android.view.View
import android.widget.Button
import android.widget.ImageView
import android.widget.TextView
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import com.retrogbm.databinding.ActivityMainBinding
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import java.io.ByteArrayOutputStream
import java.io.File
import java.nio.IntBuffer

class MainActivity : AppCompatActivity() {

    // Coroutines
    private val emulatorCoroutineScope = CoroutineScope(Dispatchers.Main)
    private val updateTexturecoroutineScope = CoroutineScope(Dispatchers.Main)

    private lateinit var emulatorThread: Job
    private lateinit var updateTextureThread: Job

    // UI components
    private lateinit var binding: ActivityMainBinding
    private lateinit var image: ImageView

    // Emulator components
    private var emulator: EmulatorWrapper = EmulatorWrapper()
    private val bitmap: Bitmap = Bitmap.createBitmap(160, 144, Bitmap.Config.ARGB_8888)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        image = findViewById(R.id.ivEmulator)

        // Load emulator and rom
        val documentPath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
        // val path = "$documentPath/cgb-acid2.gbc"
        // val path = "$documentPath/Tetris.gb"
        // val path = "$documentPath/Dr. Mario (World).gb"
        // val path = "$documentPath/Pokemon Red.gb"
        // val path = "$documentPath/PokemonGold.gbc"
        // val path = "$documentPath/Pokemon - Yellow Version.gbc"
        val path = "$documentPath/Super Mario Land.gb"
        emulator.loadRom(path, documentPath!!)
        startEmulation()

        // Buttons
        registerButtons()
    }

    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        val inflater: MenuInflater = menuInflater
        inflater.inflate(R.menu.game_menu, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.load_rom -> {
                loadRom()
                true
            }
            R.id.save_state -> {
                // Toast.makeText(this, "Save State", Toast.LENGTH_SHORT).show()
                emulator.saveState(getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath!!)
                true
            }
            R.id.load_state -> {
                // Toast.makeText(this, "Load State", Toast.LENGTH_SHORT).show()
                emulator.loadState(getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath!!)
                true
            }
            R.id.help -> {
                stopEmulator()
                Toast.makeText(this, "Settings", Toast.LENGTH_SHORT).show()
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }

    private val openDocumentLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
        if (result.resultCode == Activity.RESULT_OK) {
            val uri: Uri? = result.data?.data
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

            emulator.stop()
            emulatorThread.cancel()
            updateTextureThread.cancel()

            emulator.loadRom(bytes, getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath!!)
            startEmulation()
        }
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
        updateTextureThread = updateTexturecoroutineScope.launch(Dispatchers.Default) {
            while (emulator.isRunning()) {
                withContext(Dispatchers.Main) {
                    val pixels = emulator.getVideoBuffer()
                    setColours(pixels.toList())
                }
            }
        }
    }

    private fun registerButtons() {

        Log.d("RetroGBm", "Register Button double yes")

        val buttonLeft = findViewById<Button>(R.id.btnLeft)
        val buttonRight = findViewById<Button>(R.id.btnRight)
        val buttonUp = findViewById<Button>(R.id.btnUp)
        val buttonDown = findViewById<Button>(R.id.btnDown)

        val buttonA = findViewById<Button>(R.id.btnA)
        val buttonB = findViewById<Button>(R.id.btnB)
        val buttonStart = findViewById<Button>(R.id.btnStart)
        val buttonSelect = findViewById<Button>(R.id.btnSelect)

        setButtonTouchListener(buttonLeft, 6)
        setButtonTouchListener(buttonRight, 7)
        setButtonTouchListener(buttonUp, 4)
        setButtonTouchListener(buttonDown, 5)

        setButtonTouchListener(buttonA, 0)
        setButtonTouchListener(buttonB, 1)
        setButtonTouchListener(buttonStart, 2)
        setButtonTouchListener(buttonSelect, 3)
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
                        return true
                    }
                    MotionEvent.ACTION_UP -> {
                        // Handle touch up event
                        // This is triggered when the user releases the touch
                        emulator.pressButton(btn, false)
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