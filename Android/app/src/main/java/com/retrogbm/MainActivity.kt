package com.retrogbm

import android.graphics.Bitmap
import android.graphics.Color
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Environment
import android.view.MotionEvent
import android.view.View
import android.widget.Button
import android.widget.ImageView
import android.widget.TextView
import com.retrogbm.databinding.ActivityMainBinding
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.nio.IntBuffer

class MainActivity : AppCompatActivity() {

    // Coroutines
    private val emulatorCoroutineScope = CoroutineScope(Dispatchers.Main)
    private val updateTexturecoroutineScope = CoroutineScope(Dispatchers.Main)

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
        emulator.loadRom(path)

        // Emulator background thread
        emulatorCoroutineScope.launch(Dispatchers.Default) {
            while (true) {
                emulator.tick()
            }
        }

        // Emulator update thread
        updateTexturecoroutineScope.launch(Dispatchers.Default) {
            while (true) {
                withContext(Dispatchers.Main) {
                    val pixels = emulator.getVideoBuffer()
                    setColours(pixels.toList())
                }
            }
        }

        val title = emulator.getCartridgeTitle()
        binding.sampleText.text = title

        // Buttons
        registerButtons()
    }

    private fun registerButtons() {
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