package com.retrogbm.composables

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Dialog

@Composable
fun CustomPopup(
    title: String,
    confirmText: String = "OK",
    cancelText: String = "Cancel",
    onConfirm: () -> Unit = {},
    onCancel: () -> Unit = {},
    content: @Composable () -> Unit = {},
) {
    Dialog(onDismissRequest = {
        onCancel()
    }) {
        Surface(
            shape = RoundedCornerShape(8.dp),
            color = MaterialTheme.colorScheme.surface,
            tonalElevation = 8.dp
        ) {
            Column(
                modifier = Modifier
                    .padding(16.dp)
                    .width(300.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                // Title
                Text(
                    title,
                    style = MaterialTheme.typography.titleMedium
                )

                // Content
                content()

                // Buttons
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.End
                ) {
                    TextButton(onClick = {
                        onCancel()
                    }) {
                        Text(cancelText)
                    }

                    Spacer(modifier = Modifier.width(8.dp))

                    Button(onClick = {
                        onConfirm()
                    }) {
                        Text(confirmText)
                    }
                }
            }
        }
    }
}