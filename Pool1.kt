package com.example.pool

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.ImageButton
import android.widget.TextView
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL
import java.io.BufferedReader
import java.io.InputStreamReader



class Pool1 : AppCompatActivity() {

    private fun fetchDataFromThingSpeak(): String {
        return runBlocking {
            withContext(Dispatchers.IO) {
                val url = URL("https://api.thingspeak.com/channels/2120070/fields/1.json?api_key=TLITHTFB75ZF0QQW&results=2")
                val connection = url.openConnection() as HttpURLConnection

                val inputStream = connection.inputStream
                val bufferedReader = BufferedReader(InputStreamReader(inputStream))
                val jsonResponse = bufferedReader.readText()

                val jsonObject = JSONObject(jsonResponse)
                val feeds = jsonObject.getJSONArray("feeds")
                if (feeds.length() > 0) {
                    val firstFeed = feeds.getJSONObject(0)
                    return@withContext firstFeed.getString("field1")
                } else {
                    return@withContext "No data available"
                }
            }
        }
    }
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_pool1)
        // Home button press to take home
        val imagebuttonClick = findViewById<ImageButton>(R.id.homebutton)
        imagebuttonClick.setOnClickListener {
            val intent = Intent(this, MainActivity::class.java)
            startActivity(intent)
        }
        // settings button to go to settings
        val imagebutton1Click = findViewById<ImageButton>(R.id.settings_button)
        imagebutton1Click.setOnClickListener {
            val intent = Intent(this, Settings::class.java)
            startActivity(intent)
        }
        // code to update UI
        try {
            GlobalScope.launch {
                val data = fetchDataFromThingSpeak()
                withContext(Dispatchers.Main) {
                    val textView = findViewById<TextView>(R.id.TempOut)
                    textView.text = data
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
    }
