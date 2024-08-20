package com.retrogbm

import android.app.AlertDialog
import android.os.Bundle
import android.os.Environment
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Divider
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.Surface
import androidx.compose.material3.TextButton
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

class SaveStateActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val medata = mutableListOf<SaveStateData>()
        medata.add(SaveStateData(1, "2024/08/20", "26.4 hours"))
        medata.add(SaveStateData(2, "2023/07/24", "12.5 hours"))
        medata.add(SaveStateData(3, "2023/07/24", "2.2 hours"))
        medata.add(SaveStateData(4, "2023/07/24", "452.3 hours"))
        medata.add(SaveStateData(5, "2023/07/24", "52.2 hours"))

        setContent {
            LazyColumn(
                verticalArrangement = Arrangement.spacedBy(12.dp),
            ) {
                items(count = medata.size) { index ->
                    val item = medata[index]
                    SaveStateSlotCard(SaveStateData(item.slot, item.dateModified, item.timePlayed))
                    HorizontalDivider(color = Color.Black)
                }
            }
        }
    }
}

data class SaveStateData(val slot: Int, val dateModified: String, val timePlayed: String)

@Composable
fun SaveStateSlotCard(data: SaveStateData) {

    val showDialog = remember { mutableStateOf(false) }

    Surface(onClick = {
            showDialog.value = true
        }
    ) {
        Row(modifier = Modifier.padding(all = 20.dp)) {
            Text(
                "Slot ${data.slot} - ${data.dateModified} - ${data.timePlayed}",
                fontSize = 20.sp
            )
        }
    }

    if (showDialog.value) {

//        AlertDialog(
//            onDismissRequest = {
//
//            },
//            confirmButton = {
//
//            }
//        )

        AlertDialog(
            onDismissRequest = {
                // Hide the dialog when dismissed
                showDialog.value = false
            },
            title = {
                Text(text = "Save Slot Information")
            },
            text = {
                Text(text = "Do you want to load this save slot?")
            },
            confirmButton = {
                Button(
                    onClick = {
                        // Handle confirm action
                        showDialog.value = false
                    }
                ) {
                    Text("Load")
                }
            },
            dismissButton = {
                Button(
                    onClick = {
                        // Handle dismiss action
                        showDialog.value = false
                    }
                ) {
                    Text("Cancel")
                }
            }
        )
    }
}

@Preview
@Composable
fun PreviewMessageCard() {
    SaveStateSlotCard(
        data = SaveStateData(1, "2024/08/20", "26.4 hours")
    )
}
