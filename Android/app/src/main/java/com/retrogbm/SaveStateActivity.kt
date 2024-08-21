package com.retrogbm

import android.app.Activity
import android.app.AlertDialog
import android.content.Intent
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.widget.Toast
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
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import java.io.File

class SaveStateActivity : ComponentActivity() {

    private var statetype: Int = -1

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val romPath = intent.getStringExtra("RomTitle")
        statetype = intent.getIntExtra("StateType", -1)

        val type = if (statetype == 1) "Save" else "Load"
        // Toast.makeText(this, "Type $type", Toast.LENGTH_SHORT).show()

        val medata = mutableListOf<SaveStateData>()

        // Using the rom title try fetch 1-9 save-state files
        for (i in 1..9) {

            val path = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath!! + "/" + "${romPath}.slot${i}.state"
            var file = File(path)

            if (!file.exists()) {
                medata.add(SaveStateData(i, "", "", statetype))
                continue
            }

            medata.add(SaveStateData(i, "2024/08/21", "24 hours", statetype))
        }

        setContent {
            LazyColumn(
                verticalArrangement = Arrangement.spacedBy(12.dp),
            ) {
                items(count = medata.size) { index ->
                    val item = medata[index]
                    SaveStateSlotCard(SaveStateData(item.slot, item.dateModified, item.timePlayed, statetype))
                    HorizontalDivider(color = Color.Black)
                }
            }
        }
    }
}

data class SaveStateData(val slot: Int, val dateModified: String, val timePlayed: String, val type: Int)

@Composable
fun SaveStateSlotCard(data: SaveStateData) {

    val context = LocalContext.current as Activity

    Surface(onClick = {
            val resultIntent = Intent().apply {
                putExtra("Slot", data.slot)
                putExtra("StateType", data.type)
            }

            context.setResult(Activity.RESULT_OK, resultIntent)
            context.finish()
        }
    ) {
        Row(modifier = Modifier.padding(all = 20.dp)) {
            Text(
                if (data.dateModified.isEmpty() && data.timePlayed.isEmpty()) {
                    "Slot ${data.slot} - Empty"
                } else{
                    "Slot ${data.slot} - ${data.dateModified} - ${data.timePlayed}"
                },

                fontSize = 20.sp
            )
        }
    }
}

@Preview
@Composable
fun PreviewMessageCard() {
    SaveStateSlotCard(
        data = SaveStateData(1, "2024/08/20", "26.4 hours", 0)
    )
}
