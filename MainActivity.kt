package com.example.pool

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Button
import android.content.Intent
import android.widget.ImageButton


class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val buttonClick = findViewById<Button>(R.id.Pool_1)
        buttonClick.setOnClickListener {
            val intent = Intent(this, Pool1::class.java)
            startActivity(intent)}

        val imagebuttonClick = findViewById<ImageButton>(R.id.settings_button)
        imagebuttonClick.setOnClickListener {
            val intent = Intent(this, Settings::class.java)
            startActivity(intent)}
    }

}