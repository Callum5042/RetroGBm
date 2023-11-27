package com.retrogbm

import android.graphics.Bitmap
import android.graphics.Color
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Environment
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
        val path = "$documentPath/Pokemon - Yellow Version.gbc"


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
    }

    private fun setColours(pixels: List<Int>) {
        val pixelsBuffer = IntBuffer.wrap(pixels.toIntArray())

        val bitmap: Bitmap = Bitmap.createBitmap(160, 144, Bitmap.Config.ARGB_8888)
        bitmap.copyPixelsFromBuffer(pixelsBuffer)

        val scaledBitmap: Bitmap = Bitmap.createScaledBitmap(bitmap, 160, 144, false)

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

    companion object {
        // Used to load the 'retrogbm' library on application startup.
        init {
            System.loadLibrary("retrogbm")
        }
    }
}