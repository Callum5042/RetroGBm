package com.retrogbm

import android.app.Activity
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.DeleteForever
import androidx.compose.material.icons.filled.Edit
import androidx.compose.material3.Button
import androidx.compose.material3.Checkbox
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.toMutableStateList
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Dialog
import com.retrogbm.ui.theme.RetroGBmTheme
import java.util.UUID

class CheatsActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        cheats = Emulator.emulator.getCheatCodes()
            .map { cheat->
                CheatCodeItem(
                    id = UUID.randomUUID().toString(),
                    name = cheat.name,
                    code = cheat.code,
                    enabled = cheat.enabled
                )
            }
            .toMutableStateList()

        setContent {
            RetroGBmTheme {
                Content()
            }
        }
    }

    private fun toCheatCodeList(): Array<CheatCode> {
        return cheats
            .map { cheat ->
                CheatCode(
                    name = cheat.name,
                    code = cheat.code,
                    enabled = cheat.enabled
                )
            }
            .toTypedArray()
    }

    data class CheatCodeItem(
        val id: String,
        var name: String,
        var code: Array<String>,
        var enabled: Boolean
    )

    private lateinit var cheats: MutableList<CheatCodeItem>

    @Composable
    fun DeleteDialog(title: String,
                     onCancel: () -> Unit,
                     onConfirm: () -> Unit) {
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
                    Text(title, style = MaterialTheme.typography.titleMedium)

                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.End
                    ) {
                        TextButton(onClick = {
                            onCancel()
                        }) {
                            Text("Cancel")
                        }

                        Spacer(modifier = Modifier.width(8.dp))

                        Button(onClick = {
                            onConfirm()
                        }) {
                            Text("OK")
                        }
                    }
                }
            }
        }
    }

    @Composable
    fun CheatFormDialog(title: String,
                        cheatName: String,
                        cheatCode: String,
                        onCancel: () -> Unit,
                        onConfirm: (name: String, code: String) -> Unit
                        ) {

        var cheatNameInput by remember { mutableStateOf(cheatName) }
        var cheatCodeInput by remember { mutableStateOf(cheatCode) }

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
                    Text(title, style = MaterialTheme.typography.titleMedium)

                    OutlinedTextField(
                        value = cheatNameInput,
                        onValueChange = { cheatNameInput = it },
                        label = { Text("Cheat Name") },
                        singleLine = true
                    )

                    OutlinedTextField(
                        value = cheatCodeInput,
                        onValueChange = { cheatCodeInput = it },
                        label = { Text("Cheat Code") },
                        modifier = Modifier.height(100.dp), // Or whatever height suits
                        maxLines = 5
                    )

                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.End
                    ) {
                        TextButton(onClick = {
                            onCancel()
                        }) {
                            Text("Cancel")
                        }

                        Spacer(modifier = Modifier.width(8.dp))

                        Button(onClick = {
                            onConfirm(cheatNameInput, cheatCodeInput)
                        }) {
                            Text("OK")
                        }
                    }
                }
            }
        }
    }

    @OptIn(ExperimentalMaterial3Api::class, ExperimentalFoundationApi::class)
    @Composable
    fun Content() {

        val context = LocalContext.current as? Activity

        var showAddDialog by remember { mutableStateOf(false) }

        if (showAddDialog) {
            CheatFormDialog(
                title = "Add Cheat",
                cheatName = "",
                cheatCode = "",
                onCancel = { showAddDialog = false },
                onConfirm = { name, code ->
                    showAddDialog = false

                    cheats.add(
                        CheatCodeItem(
                            id = UUID.randomUUID().toString(),
                            name = name,
                            code = code.split("\n").toTypedArray(),
                            enabled = false
                        )
                    )

                    Emulator.emulator.setCheatCodes(toCheatCodeList())
                }
            )
        }

        Scaffold(
            topBar = {
                TopAppBar(
                    colors = TopAppBarDefaults.topAppBarColors(
                        containerColor = Color.Black,
                        titleContentColor = Color.White,
                        navigationIconContentColor = Color.White,
                        actionIconContentColor = Color.White,
                    ),
                    title = {
                        Text("RetroGBm")
                    },
                    navigationIcon = {
                        IconButton(onClick = {
                            context?.finish()
                        }) {
                            Icon(
                                imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                                contentDescription = "Localized description"
                            )
                        }
                    },
                    actions = {
                        IconButton(onClick = {
                            showAddDialog = true
                        }) {
                            Icon(
                                imageVector = Icons.Filled.Add,
                                contentDescription = stringResource(id = R.string.quick_save)
                            )
                        }
                    }
                )
            },
        ) { innerPadding ->
            Column(
                modifier = Modifier
                    .fillMaxSize() // Ensure system bars are handled properly
                    .padding(innerPadding)  // Adjust padding if needed
            ) {

                LazyColumn(
                    modifier = Modifier
                        .fillMaxSize()
                ) {
                    items(
                        items = cheats,
                        key = { it.id }
                    ) { item ->

                        var checked by remember { mutableStateOf(item.enabled) }
                        var showContextMenu by remember { mutableStateOf(false) }

                        Box(
                            modifier = Modifier.fillMaxWidth(), // Ensure full width to help with positioning
                            contentAlignment = Alignment.BottomEnd // Aligns the menu to the bottom-right
                        ) {
                            Column(
                                modifier = Modifier
                                    .padding(horizontal = 0.dp)
                                    .combinedClickable(
                                        onClick = {
                                            checked = !checked
                                        },
                                        onLongClick = {
                                            showContextMenu = true
                                        }
                                    )
                            ) {

                                Row(
                                    modifier = Modifier
                                        .padding(horizontal = 8.dp)
                                        .fillMaxWidth(),
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    Checkbox(
                                        checked = checked,
                                        onCheckedChange = {
                                            checked = it
                                            item.enabled = it
                                            Emulator.emulator.setCheatCodes(toCheatCodeList())
                                        }
                                    )
                                    Text(
                                        item.name
                                    )
                                }
                            }

                            var showUpdateDialog by remember { mutableStateOf(false) }
                            if (showUpdateDialog) {
                                CheatFormDialog(
                                    title = "Update Cheat",
                                    cheatName = item.name,
                                    cheatCode = item.code.joinToString("\n"),
                                    onCancel = { showUpdateDialog = false },
                                    onConfirm = { name, code ->
                                        showUpdateDialog = false

                                        item.name = name
                                        item.code = code.split("\n").toTypedArray()

                                        Emulator.emulator.setCheatCodes(toCheatCodeList())
                                    }
                                )
                            }

                            var showDeleteDialog by remember { mutableStateOf(false) }
                            if (showDeleteDialog) {
                                DeleteDialog(
                                    title = "Delete Cheat",
                                    onCancel = {
                                        showDeleteDialog = false
                                    },
                                    onConfirm = {
                                        showDeleteDialog = false
                                        if (cheats.remove(item)) {
                                            Emulator.emulator.setCheatCodes(toCheatCodeList())
                                        }
                                    })
                            }

                            if (showContextMenu) {
                                Box(
                                    modifier = Modifier.padding(8.dp),
                                    contentAlignment = Alignment.BottomEnd
                                ) {
                                    DropdownMenu(
                                        expanded = showContextMenu,
                                        onDismissRequest = { showContextMenu = false },
                                        modifier = Modifier
                                            .align(Alignment.BottomEnd)
                                    ) {
                                        DropdownMenuItem(
                                            text = { Text("Update") },
                                            leadingIcon = { Icon(Icons.Filled.Edit, null) },
                                            onClick = {
                                                showContextMenu = false
                                                showUpdateDialog = true
                                            }
                                        )
                                        HorizontalDivider()
                                        DropdownMenuItem(
                                            text = { Text("Delete") },
                                            leadingIcon = { Icon(Icons.Filled.DeleteForever, null) },
                                            onClick = {
                                                showContextMenu = false
                                                showDeleteDialog = true
                                            }
                                        )
                                    }
                                }
                            }
                        }

                        HorizontalDivider(
                            color = Color.Gray, // Color of the border
                            thickness = 1.dp,   // Thickness of the border
                            modifier = Modifier.padding(vertical = 0.dp)
                        )
                    }
                }
            }
        }
    }
}
