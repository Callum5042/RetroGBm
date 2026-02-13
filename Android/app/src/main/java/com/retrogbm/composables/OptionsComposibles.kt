package com.retrogbm.composables

import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.material3.ElevatedCard
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

@Composable
fun OptionsCard(
    title: String,
    content: @Composable () -> Unit
) {
    val titleColor = MaterialTheme.colorScheme.onSurface
    val subtitleColor = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.7f)

    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(bottom = 8.dp)
    ) {
        // Header
        Text(
            text = title,
            modifier = Modifier
                .padding(horizontal = 8.dp, vertical = 4.dp),
            color = titleColor,
            fontSize = 20.sp,
            fontWeight = FontWeight.Bold
        )

        // Card
        ElevatedCard(
            modifier = Modifier
                .fillMaxWidth()
        ) {
            content()
        }
    }
}

@Composable
fun OptionsSwitch(
    text: String,
    value: Boolean,
    onChange: ((Boolean) -> Unit)?
) {
    val titleColor = MaterialTheme.colorScheme.onSurface

    var state by remember {
        mutableStateOf(value)
    }

    Column(
        modifier = Modifier.fillMaxWidth()
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 8.dp, vertical = 0.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = text,
                fontSize = 18.sp,
                color = titleColor
            )

            Switch(
                checked = state,
                onCheckedChange = {
                    state = it
                    onChange?.invoke(it)
                }
            )
        }
    }
}

@Composable
fun OptionsInfo(
    title: String,
    text: String
) {
    val titleColor = MaterialTheme.colorScheme.onSurface
    val subtitleColor = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.7f)

    Column(
        modifier = Modifier
    ) {
        Text(
            text = title,
            fontSize = 18.sp,
            modifier = Modifier
                .padding(start = 8.dp, top = 12.dp, end = 8.dp, bottom = 0.dp)
                .fillMaxWidth(),
            color = titleColor
        )
        Text(
            text = text,
            modifier = Modifier
                .padding(horizontal = 8.dp)
                .fillMaxWidth()
                .horizontalScroll(rememberScrollState()),
            color = subtitleColor,
            fontSize = 12.sp
        )
    }
}

@Composable
fun OptionsSlider(
    text: String,
    value: Float,
    steps: Int,
    valueRange: ClosedFloatingPointRange<Float>,
    onValueChanged: ((Float) -> Unit)?
) {
    val titleColor = MaterialTheme.colorScheme.onSurface

    var state by remember {
        mutableFloatStateOf(value)
    }

    Column(
        modifier = Modifier
    ) {
        Text(
            text = text,
            fontSize = 18.sp,
            modifier = Modifier
                .padding(start = 8.dp, top = 12.dp, end = 8.dp, bottom = 0.dp)
                .fillMaxWidth(),
            color = titleColor
        )

        Slider(
            modifier = Modifier
                .padding(horizontal = 8.dp),
            value = state,
            steps = steps,
            valueRange = valueRange,
            onValueChange = {
                state = it
                onValueChanged?.invoke(it)
            }
        )
    }
}

@Composable
fun OptionsDivider() {
    HorizontalDivider(
        color = Color.Gray,
        thickness = 1.dp,
        modifier = Modifier.padding(vertical = 0.dp)
    )
}