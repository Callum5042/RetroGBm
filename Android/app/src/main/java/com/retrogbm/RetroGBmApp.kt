package com.retrogbm

import android.app.Application

class RetroGBmApp : Application() {

    lateinit var socketClient: SocketClient

    override fun onCreate() {
        super.onCreate()
        instance = this

        val logger = LoggerWrapper()
        logger.initializeLogger()
    }

    companion object {
        private var instance: RetroGBmApp? = null

        fun getInstance(): RetroGBmApp {
            return instance ?: throw IllegalStateException("RetroGBmApp is not initialized yet")
        }
    }
}