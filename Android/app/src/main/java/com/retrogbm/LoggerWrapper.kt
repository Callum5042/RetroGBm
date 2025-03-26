package com.retrogbm

class LoggerWrapper {

    external fun initializeLogger()

    companion object {
        init {
            System.loadLibrary("retrogbm")
        }
    }
}