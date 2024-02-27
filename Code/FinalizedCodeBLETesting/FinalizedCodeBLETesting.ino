// Things to do before final product
// - Comment out most or all serial printing or leave for later bugfixing?
// - EEPROM size needs to fit any possible ssid/password?
//// - Remove hardcoded credentials entirely, they'll just waste time outside testing
// - Maybe add more comments but eh, it'll be fine
// - Change timings on temperature measurement and thingspeak upload intervals

// Things to change before you upload
// - Pin designations possibly
//// - Hardcoded credentials (Obsolete)
//const char* ssid = "Guest";
//String password = "";
// - Thingspeak stuff
const char* channelIDStr_write = "2422785";
const char* apiKey_write = "YTO69PC9LIR852AU";

#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <EEPROM.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLE2904.h>

// Allocate adequate space for the EEPROM and init pins
#define EEPROM_SIZE 40
#define BUTTON_PIN 13
#define SENSOR_PIN 23
#define RED_LED 25
#define GREEN_LED 26
#define BLUE_LED 27

// Init temp sensing modules
OneWire oneWire(SENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

// Init wifi client for Thingspeak communication
WiFiClient client;

// Init Thingspeak variables to write temp data
const char* server = "api.thingspeak.com";
unsigned long channelID_write = atol(channelIDStr_write);

//// Init Thingspeak variables to read password updates
//const char* channelIDStr_read = "2277799";
//unsigned long channelID_read = atol(channelIDStr_read);
//const char* apiKey_read = "8L2M0OE8TLJUEAX5";

// Define a custom service and characteristic Universally Unique ID for BLE
#define SERVICE_UUID         "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_RX "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Init BLE server vars
NimBLEServer* pServer = NULL;
NimBLECharacteristic* pCharacteristic = NULL;
bool bleConnected = false;

// Init misc vars
float tempF;
bool button_pressed;
bool waiting = false;
const long intervalUpload = 60000;
unsigned long previousMillisUpload = -intervalUpload;
const long intervalTempRead = 45000;
unsigned long previousTempRead = -intervalTempRead;
String newCred;
String temp;
String strs[5];
int StringCount = 0;

// Callbacks for BLE server events
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
      bleConnected = true;
      Serial.println("BLE Client Connected");
    };

    void onDisconnect(NimBLEServer* pServer) {
      bleConnected = false;
      Serial.println("BLE Client Disconnected");
    }
};

// Callbacks for BLE characteristic events
class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
      // Handle write events for the BLE characteristic
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        temp += String(value.c_str());
        Serial.println(temp);
      }
    }
};

void setup() {

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  rainbowWipe();   // Check to see all colors working

  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  DS18B20.begin();
  pinMode(BUTTON_PIN, INPUT);
  button_pressed = false;

  String creds = EEPROM.readString(0);


  Serial.println(creds);

  // Split the string into substrings
  while (creds.length() > 0)
  {
    int index = creds.indexOf(',');
    if (index == -1) // No comma found
    {
      strs[StringCount++] = creds;
      break;
    }
    else
    {
      strs[StringCount++] = creds.substring(0, index);
      creds = creds.substring(index + 1);
    }
  }


  Serial.print("SSID = ");
  Serial.println(strs[0]);
  Serial.print("Password = ");
  Serial.println(strs[1]);
  
  WiFi.begin(strs[0], strs[1]);
  wifiDelay();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
    green();
  } 
//  else {
//    WiFi.begin(ssid, password);
//    Serial.println("Connecting through hardcoded wifi");
//    Serial.println(ssid);
//    Serial.println(password);   // Obsolete hardcoded wifi section
//    errorFlash();
//    wifiDelay();
//  }

  ThingSpeak.begin(client);
}

void loop() {
  // Check if the button is pressed to initiate BLE communication
  
    bluetooth(); // stuck it in its own thing at the bottom
  

  else {
    unsigned long currentMillis = millis();
    // Read temperature and send data to Thingspeak

    if ((currentMillis - previousTempRead) >= intervalTempRead) {
      DS18B20.requestTemperatures();
      tempF = ((DS18B20.getTempCByIndex(0)) * 1.8) + 32;
      Serial.println(tempF);
      previousTempRead = currentMillis;
    }

    if (WiFi.status() == WL_CONNECTED) {
      if (tempF != -196.60001) {

        if (currentMillis - previousMillisUpload >= intervalUpload) {
          ThingSpeak.setField(1, tempF);
          previousMillisUpload = currentMillis;
          delay(5000);
        }

        int response = ThingSpeak.writeFields(channelID_write, apiKey_write);
        if (response == 200) {
          Serial.println("Data sent to ThingSpeak successfully.");
          cyan();
          delay(5000);
        }
        else if (currentMillis - previousMillisUpload < intervalUpload) {
          Serial.println("Waiting for next upload");
          yellow();
          delay(5000);
        }
        else {
          Serial.println("Error sending data to ThingSpeak. HTTP error code: " + String(response));
          red();
          delay(5000);
        }
      }
      else {
        Serial.println("Temp read error (-196.60001)");
      }

      // Code for reading password updates from Thingspeak (commented out)
      // newCred = ThingSpeak.readFloatField(channelID_read, 1, apiKey_read);
      // if (newCred != password){
      //   EEPROM.writeString(0, newCred);
      //   EEPROM.commit();
      // }
    }
    else {
      errorFlash();
      ESP.restart();
    }
  }
}

void errorFlash() {
  red();
  delay(200);
  off();
  delay(100);
  red();
  delay(200);
  off();
  delay(100);
  red();
  delay(200);
  off();
  delay(100);
  red();
  delay(200);
  off();
  delay(1000);
}

void rainbowWipe() {

  red();
  delay(150);
  yellow();
  delay(150);
  green();
  delay(150);
  cyan();
  delay(150);
  blue();
  delay(150);
  purple();
  delay(150);
  off();
}

void red() {
  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
}
void yellow() {
  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(BLUE_LED, LOW);
}
void green() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(BLUE_LED, LOW);
}
void cyan() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(BLUE_LED, HIGH);
}
void blue() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, HIGH);
}
void purple() {
  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, HIGH);
}
void off() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
}
void bluetoothflash() {
  red();
  delay(150);
  blue();
  delay(150);
  red();
  delay(150);
  blue();
  delay(150);
  red();
  delay(150);
  blue();
  delay(150);
  purple();
}
void wifiDelay() {
  // 10s delay with indicator lighting
  yellow();
  delay(1000);
  off();
  delay(1000);
  yellow();
  delay(1000);
  off();
  delay(1000);
  yellow();
  delay(1000);
  off();
  delay(1000);
  yellow();
  delay(1000);
  off();
  delay(1000);
  yellow();
  delay(1000);
  off();
  delay(1000);
}

void bluetooth() {
  if (digitalRead(BUTTON_PIN) == HIGH) {

  bluetoothflash();

  // Initialize BLE server and characteristics
  NimBLEDevice::init("ESP32 BLE Test");
  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  // RX Characteristic (Write)
  NimBLECharacteristic* pRXCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE
      );
  pRXCharacteristic->setCallbacks(new MyCallbacks());

  // TX Characteristic (Notify)
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
                    );
  pCharacteristic->addDescriptor(new NimBLE2904());

  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");

  waiting = true;

  while (waiting) {
    if (temp != "") {
      newCred = temp;
      temp = "";
      waiting = false;
    }
  }

  
    Serial.println("New Credentials Received!");
    Serial.println(newCred);
    EEPROM.writeString(0, newCred);
    EEPROM.commit();

    blue(); delay(150); off(); delay(100); blue(); delay(150); off(); delay(100);
    blue(); delay(150); off(); delay(100);

    ESP.restart();
}
}