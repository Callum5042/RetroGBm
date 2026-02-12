package com.retrogbm

import android.util.Log
import java.io.InputStream
import java.io.OutputStream
import java.net.Socket
import kotlin.concurrent.thread

class SocketClient(private val emulator: EmulatorWrapper) {
    private var socket: Socket? = null
    private var outputStream: OutputStream? = null
    private var inputStream: InputStream? = null
    private var serverPort = 54000

    // Connect to the server
    fun connect(ip: String) {
        thread {
            try {
                socket = Socket(ip, serverPort)
                outputStream = socket?.outputStream
                inputStream = socket?.inputStream

                Log.d("Socket","Connected to the server!")
                listenForData()
            } catch (e: Exception) {
                e.printStackTrace()
                Log.d("Socket","Failed to connect to the server")
            }
        }
    }

    fun sendData(data: Byte) {
        thread {
            try {
                if (socket != null && socket?.isConnected == true) {
                    val dataArray: ByteArray = byteArrayOf(0xFF.toByte(), data)
                    outputStream?.write(dataArray)
                    outputStream?.flush()
                } else {
                    Log.d("Socket","Socket is not connected!")
                }
            } catch (e: Exception) {
                e.printStackTrace()
                Log.d("Socket", "Failed to send data")
            }
        }
    }

    // Listen for incoming data from the server
    private fun listenForData() {
        thread {
            try {
                val buffer = ByteArray(1024) // Buffer to store incoming data
                var bytesRead: Int

                // Continuously read from the input stream
                while (socket?.isConnected == true) {
                    bytesRead = inputStream?.read(buffer) ?: -1

                    if (bytesRead > 0) {
                        val dataReceived = buffer.copyOf(bytesRead)
                        Log.d("Socket", "Received data: ${dataReceived.joinToString(", ") { it.toString() }}")

                        emulator.linkCableData(buffer[1])
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
                Log.d("Socket", "Error while listening for data")
            }
        }
    }

    // Close the socket when you're done
    fun disconnect() {
        thread {
            try {
                socket?.close()
                Log.d("Socket","Disconnected from the server")
            } catch (e: Exception) {
                e.printStackTrace()
                Log.d("Socket","Failed to disconnect")
            }
        }
    }

    fun isConnected(): Boolean {
        return socket?.isConnected == true
    }
}