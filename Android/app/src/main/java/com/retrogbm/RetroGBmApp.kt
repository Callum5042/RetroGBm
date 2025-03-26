package com.retrogbm

import android.app.Application

class RetroGBmApp : Application() {
    override fun onCreate() {
        super.onCreate()

        val logger = LoggerWrapper()
        logger.initializeLogger()
    }
}