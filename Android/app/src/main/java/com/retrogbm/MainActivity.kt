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

    private lateinit var binding: ActivityMainBinding
    private val coroutineScope = CoroutineScope(Dispatchers.Main)
    private val coroutineScope2 = CoroutineScope(Dispatchers.Main)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Example of a call to a native method
        val documentPath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
        // val path = "$documentPath/cgb-acid2.gbc"
        // val path = "$documentPath/Tetris.gb"
        // val path = "$documentPath/Dr. Mario (World).gb"
        // val path = "$documentPath/Pokemon Red.gb"
        // val path = "$documentPath/PokemonGold.gbc"
        // val path = "$documentPath/Pokemon - Yellow Version.gbc"
        val path = "$documentPath/Super Mario Land.gb"

        // Load cartridge test
//        val cartridgePtr: Long = createAndLoadCartridge(path)
//        val title: String = cartridgeGetTitle(cartridgePtr)



        // Emulator life
        val emulatorPtr = createEmulator()
        loadRom(emulatorPtr, path)

        coroutineScope.launch(Dispatchers.Default) {
            while (true) {
                tick(emulatorPtr)
            }
        }

        coroutineScope2.launch(Dispatchers.Default) {
            while (true) {
                withContext(Dispatchers.Main) {
                    val pixels = getVideoBuffer(emulatorPtr)
                    // val pixels = getPixels(Color.GREEN)
                    setColours(pixels.toList())
                }
            }
        }

        val title = cartridgeGetTitle(emulatorPtr)
        binding.sampleText.text = title //stringFromFile(path)

        // Buttons
        val buttonLeft = findViewById<Button>(R.id.btnLeft)
        val buttonRight = findViewById<Button>(R.id.btnRight)
        val buttonUp = findViewById<Button>(R.id.btnUp)
        val buttonDown = findViewById<Button>(R.id.btnDown)

        val buttonA = findViewById<Button>(R.id.btnA)
        val buttonB = findViewById<Button>(R.id.btnB)
        val buttonStart = findViewById<Button>(R.id.btnStart)
        val buttonSelect = findViewById<Button>(R.id.btnSelect)

        setButtonTouchListener(buttonLeft, emulatorPtr, 6)
        setButtonTouchListener(buttonRight, emulatorPtr, 7)
        setButtonTouchListener(buttonUp, emulatorPtr, 4)
        setButtonTouchListener(buttonDown, emulatorPtr, 5)

        setButtonTouchListener(buttonA, emulatorPtr, 0)
        setButtonTouchListener(buttonB, emulatorPtr, 1)
        setButtonTouchListener(buttonStart, emulatorPtr, 2)
        setButtonTouchListener(buttonSelect, emulatorPtr, 3)
    }

    private fun setButtonTouchListener(button: View, emulatorPtr: Long, btn: Int) {
        button.setOnTouchListener(object : View.OnTouchListener {
            override fun onTouch(view: View?, event: MotionEvent?): Boolean {
                when (event?.action) {
                    MotionEvent.ACTION_DOWN -> {
                        // Handle touch down event
                        // This is triggered when the user first touches the screen
                        pressButton(emulatorPtr, btn, true)
                        return true
                    }
                    MotionEvent.ACTION_UP -> {
                        // Handle touch up event
                        // This is triggered when the user releases the touch
                        pressButton(emulatorPtr, btn, false)
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

        val bitmap: Bitmap = Bitmap.createBitmap(160, 144, Bitmap.Config.ARGB_8888)
        bitmap.copyPixelsFromBuffer(pixelsBuffer)

        val scaledBitmap: Bitmap = Bitmap.createScaledBitmap(bitmap, 160 * 2, 144 * 2, true)

        val image: ImageView = findViewById(R.id.ivEmulator)
        image.setImageBitmap(scaledBitmap)
    }

    /**
     * A native method that is implemented by the 'retrogbm' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    private external fun stringFromFile(path: String): String

    // Cartridge life
    private external fun createAndLoadCartridge(path: String): Long
    private external fun cartridgeGetTitle(emulatorPtr: Long): String

    // Emulator life
    private external fun createEmulator(): Long
    private external fun loadRom(emulatorPtr: Long, path: String)
    private external fun tick(emulatorPtr: Long)

    private external fun getVideoBuffer(emulatorPtr: Long): IntArray

    private external fun getPixels(colour: Int): IntArray

    private external fun pressButton(emulatorPtr: Long, button: Int, state: Boolean)

    companion object {
        // Used to load the 'retrogbm' library on application startup.
        init {
            System.loadLibrary("retrogbm")
        }
    }
}