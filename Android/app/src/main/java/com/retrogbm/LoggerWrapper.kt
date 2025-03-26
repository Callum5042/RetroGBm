package com.retrogbm

class LoggerWrapper {

    external fun initializeLogger()

    external fun info(message: String)
    external fun waring(message: String)
    external fun error(message: String)
    external fun critical(message: String)

    companion object {
        init {
            System.loadLibrary("retrogbm")
        }
    }
}