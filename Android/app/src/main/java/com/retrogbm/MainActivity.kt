package com.retrogbm

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Environment
import android.widget.TextView
import com.retrogbm.databinding.ActivityMainBinding
import java.io.File

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Example of a call to a native method
        val documentPath = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath
        val path = "$documentPath/cgb-acid2.gbc"

        // Load cartridge test
        val cartridgePtr: Long = createAndLoadCartridge(path)
        val title: String = cartridgeGetTitle(cartridgePtr)


        binding.sampleText.text = title //stringFromFile(path)
    }

    /**
     * A native method that is implemented by the 'retrogbm' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    private external fun stringFromFile(path: String): String

    // Cartridge life
    private external fun createAndLoadCartridge(path: String): Long
    private external fun cartridgeGetTitle(cartridgePtr: Long): String

    companion object {
        // Used to load the 'retrogbm' library on application startup.
        init {
            System.loadLibrary("retrogbm")
        }
    }
}