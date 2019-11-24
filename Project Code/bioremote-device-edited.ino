#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "WiFiEsp.h"

#define HASH "device_hash" //device hash(we are not using token)

char ssid[] = "Room-C22";
char pass[] = "hellonet123";

int status = WL_IDLE_STATUS;     // the Wifi radio's status

char server[] = "http://18.216.149.144/nova";
WiFiEspClient Espclient;
PubSubClient client(Espclient);

// char thingsboardServer[] = "";


#include <LiquidCrystal.h>
#include <Keypad.h>
#include <EEPROM.h>
LiquidCrystal lcd(19, 18, 17, 16, 15, 14);

//Declaring keypad pins
const byte rows = 4;
const byte cols = 4;
char hexaKeys[rows][cols] = {
  {'D', 'C', 'B', 'A'},
  {'#', '9', '6', '3'},
  {'0', '8', '5', '2'},
  {'*', '7', '4', '1'}
};
byte rowPins[rows] = {51, 49, 47, 45}; //connect to the row pinouts of the keypad
byte colPins[cols] = {43, 41, 39, 37}; //connect to the column pinouts of the keypad
Keypad key = Keypad(makeKeymap(hexaKeys), rowPins, colPins, rows, cols);



/****************Declaring of Heart Beat Variables***************************/

#define heart_beat_sens A0
int UpperThreshold = 518;
int LowerThreshold = 490;
int reading = 0;
float BPM = 0.0;
bool IgnoreReading = false;
bool FirstPulseDetected = false;
unsigned long FirstPulseTime = 0;
unsigned long SecondPulseTime = 0;
unsigned long PulseInterval = 0;
const unsigned long delayTime = 10;
const unsigned long delayTime2 = 1000;
const unsigned long baudRate = 9600;
unsigned long previousMillis = 0;
unsigned long previousMillis2 = 0;

/*************End of HeartBeat variable Declaration ****************/

/*********************Declaring of body temperature variables*************/

#include <Adafruit_MAX31865.h>
// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 max = Adafruit_MAX31865(10, 12, 11, 13);
// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF 430.0
#define RNOMINAL  100.0 // 100.0 for PT100, 1000.0 for PT1000

/*******End of declaration****************************************/

String info_data[6];// 0: patient id, 1: heartbeat data,2: body temp 3: resp rate 4 blood pressur


void setup() {
  pinMode(heart_beat_sens, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(9600);
  max.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
  lcd.begin(20, 4);
  lcd.print(" BIO-REMOTE HEALTH  ");
  lcd.setCursor(0, 1);
  lcd.print("       DEVICE       ");
  lcd.setCursor(0, 2);
  lcd.print("Powered By:NaijaHack");
  WiFi.init(&Serial1);

  //checks if module is mounted
  if (WiFi.status() == WL_NO_SHIELD) {
    lcd.setCursor(0, 3);
    lcd.print("WiFi Shield NotFound");
    Serial.println("WiFi shield not present");
    // don't continu
    while (true);
  }


  // attempt to connect to WiFi network
  //Blocking connection(you can put it in the loop for non-blocking connection)
  clearScreen();
  while ( status != WL_CONNECTED) {
    Serial.print("debug message to show its connnecting to :  ");

    lcd.setCursor(0, 2);
    lcd.print(" WiFi Connecting To ");
    lcd.setCursor(2, 3);
    Serial.println(ssid);
    lcd.print(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }
  clearScreen();
  lcd.setCursor(0, 2);
  lcd.print("Connection Successful");
  Serial.println("Connection succesful");

  //connect to MQTT server
  client.setServer( thingsboardServer, 1883 );
  client.setCallback(on_message);
  delay(3000);

}

void loop() {
    if ( !client.connected() ) {
    reconnect();
  }

  client.loop();
  doOtherStuffs();
 

}

void doOtherStuffs(){
     lcd.clear();
  lcd.print(" BIO-REMOTE HEALTH  ");
  lcd.setCursor(0, 1);
  lcd.print(F("       DEVICE       "));
  lcd.setCursor(0, 2);
  lcd.print(F("   Press Any Key    "));
  lcd.setCursor(0, 3);
  lcd.print(F("To Start Taking Test"));
  bool start_flg = false;
  long current_time, previous_time = 0;
  byte blink_state = 0;
  while (!start_flg) {
    current_time = millis();
    if ((current_time - previous_time) > 800) {
      blink_state = !blink_state;

      if (blink_state == 0) {
        clearScreen();
      }
      if (blink_state == 1) {
        lcd.setCursor(0, 2);
        lcd.print(F("   Press Any Key    "));
        lcd.setCursor(0, 3);
        lcd.print(F("To Start Taking Test"));
      }
      previous_time = current_time;
    }
    //waiting for key press
    char key_pressed = key.getKey();
    if (key_pressed) {
      clearScreen(); //clear screen
      start_flg = true; //and move out of the loop
    }
  }

  enterPatientId();
  startTest();
}

void reconnect() {
  // Loop until  reconnected
  while (!client.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(ssid, pass);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }

    Serial.print("Connecting to Bioremote MQTT server ...");


    //Note before connecting with username and password, the device will have to do a one-time registration
    //After registration, then we can re-connect using the username and password
    //
    //check the PubSub documentation for the proper constructor for connect()
    //check the one with the (username,password)     parameters
    if ( client.connect(" Device", HASH, NULL) ) {
      Serial.println( "Sucessful connection with MQTT server" );
      // Subscribing to receive pub message 
      //Subscribe to the required topics and the topics meant for callback
      client.subscribe("v1/devices/xx");
      // For pusblishing
      client.publish("v1/devices/xx", (value to send));
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 1000 );
    }
  }
}

// The callback for when a PUBLISH message is received from the server.
void on_message(const char* topic, byte* payload, unsigned int length) {

  Serial.println("Response received");

  char json[length + 1];
  strncpy (json, (char*)payload, length);
  json[length] = '\0';

  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(json);

  // Decode JSON request
  //IT CAN ONLY WORK FOR VERSION 5.xx of ArduinoJson Library
  //If you use a newer version, use the DynamicJSONBuffer(I think), or check the documentation for newer code
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.parseObject((char*)json);

  if (!data.success())
  {
    Serial.println("parseObject() failed");
    return;
  }

  // Check request method
  //Check the received JSON object for the fields contained in it
  String attributeName = String((const char*)data["json-attribute"]);

  if (methodName.equals("expected-response-topic")) {
    // Reply if you need to respond back to the same topic
    String responseTopic = String(topic);
    // responseTopic.replace("request", "response");//[not required]
    client.publish(responseTopic.c_str(), getResponseMessage.c_str());
  } else if (methodName.equals("another-response-topic")) {


    displayOnHW(data["params"]["pin"], data["params"]["enabled"]);  //that's another way to extract values from the JSON

    String responseTopic = String(topic);
    // responseTopic.replace("request", "response");[not required]
    client.publish(responseTopic.c_str(), getAnotherResponseMessage(str,str).c_str());
    client.publish("v1/devices/xx-response", getAnotherResponseMessage(str,str).c_str());
  }
}

String getResponseMessage(){
    //process any information you want to send
    //and send as String
}

String getAnotherResponseMessage(String str,String anotherStr){
    //do another procesing for the second  topic expected

}

String displayOnHW(String str, int value){
    //this can be used to trigger something from the callback function
}





void clearScreen() {
  lcd.setCursor(0, 2);
  lcd.print(F("                    "));
  lcd.setCursor(0, 3);
  lcd.print(F("                    "));
}
void startTest() {
  lcd.clear();
  lcd.print(F(" BIO-REMOTE HEALTH  "));
  lcd.setCursor(0, 1);
  lcd.print(F("       DEVICE       "));
  lcd.setCursor(0, 2);
  lcd.print(F("1.Body Temperature <"));
  lcd.setCursor(0, 3);
  lcd.print(F("2.Pulse Rate        "));
  bool done_with_test = false;
  int counter = 0;
  while (!done_with_test) {
    lcd.setCursor(0, 0);
    lcd.print(F(" BIO-REMOTE HEALTH  "));
    lcd.setCursor(0, 1);
    lcd.print(F("       DEVICE       "));

    char key_pressed = key.getKey();
    if (key_pressed) {
      char c = key_pressed;
      if (c == 'A') counter --;
      if (c == 'D') counter ++;
      if (counter > 6) counter = 0;
      if (counter < 0) counter = 6;
      switch (counter) {
        case 0:
          lcd.setCursor(0, 2);
          lcd.print(F("1.Body Temperature <"));
          lcd.setCursor(0, 3);
          lcd.print(F("2.Pulse Rate        "));
          break;
        case 1:

          lcd.setCursor(0, 2);
          lcd.print(F("1.Body Temperature  "));
          lcd.setCursor(0, 3);
          lcd.print(F("2.Pulse Rate    <   "));
          break;
        case 2:

          lcd.setCursor(0, 2);
          lcd.print(F("2.Pulse Rate         "));
          lcd.setCursor(0, 3);
          lcd.print(F("3.Respiratory Rate < "));
          break;
        case 3:

          lcd.setCursor(0, 2);
          lcd.print(F("3.Respiratory Rate   "));
          lcd.setCursor(0, 3);
          lcd.print(F("4.Blood Pressure   < "));
          break;
        case 4:
          lcd.setCursor(0, 2);
          lcd.print(F("4.Blood Pressure     "));
          lcd.setCursor(0, 3);
          lcd.print(F("5.Save To SD       < "));
          break;
        case 5:

          lcd.setCursor(0, 2);
          lcd.print(F("5.Save To SD         "));
          lcd.setCursor(0, 3);
          lcd.print(F("6.Transfer Data   <  "));
          break;
        case 6:

          lcd.setCursor(0, 2);
          lcd.print(F("6.Transfer Data      "));
          lcd.setCursor(0, 3);
          lcd.print(F("7.GoTo Homepage    < "));
          break;
      }
      if (c == '#') {
        switch (counter) {
          case 0:
            checkBodyTemp();
            break;
          case 1:
            checkHeartBeat();
            break;
          case 2:
            checkRespRate();
            break;
          case 3:
            checkBloodPres();
            break;
          case 4:
            saveToSD();
            break;
          case 5:
            transferDataOnline();
            break;
          case 6:
            done_with_test = true;
            break;
        }
      }
    }
  }

}

void enterPatientId() {
  String patient_id = "";
  bool done_taking_id = false;
  lcd.setCursor(0, 2);
  lcd.print(F("  Enter Patient ID  "));
  lcd.setCursor(0, 3);
  lcd.print(F("Patient ID:         "));
  while (!done_taking_id) {
    char key_pressed = key.getKey();
    if (key_pressed) {
      char c = key_pressed;
      if (c == '*') {
        String val = deleteVal(patient_id);
        lcd.setCursor(0, 2);
        lcd.print(F("  Enter Patient ID  "));
        lcd.setCursor(0, 3);
        lcd.print(F("Patient ID:         "));
        lcd.setCursor(12, 3);
        patient_id = val;
        lcd.print(patient_id);
      }
      else if (c == '#') {
        //store patient id in info_data buffer
        info_data[0] = patient_id;
        lcd.setCursor(0, 2);
        lcd.print(F(" Patient ID Stored !"));
        lcd.setCursor(0, 3);
        lcd.print(F("Patient ID:         "));
        lcd.setCursor(12, 3);
        lcd.print(patient_id);
        delay(2000);
        done_taking_id = true;
      }
      else {
        lcd.setCursor(0, 2);
        lcd.print(F("  Enter Patient ID  "));
        lcd.setCursor(0, 3);
        lcd.print(F("Patient ID:         "));
        lcd.setCursor(12, 3);
        patient_id += c;
        lcd.print(patient_id);
      }
    }
  }
}

String deleteVal(String val) {
  uint8_t len = val.length();
  uint8_t del_count = len - 1;
  String ret;
  for (int x = 0; x < del_count; x++ ) {
    ret += val[x];
  }
  return ret;
}
/*********************************Heart Beat Function codes***************************/

void checkHeartBeat() {
  bool done_taking_bpm = false;
  clearScreen();
  lcd.setCursor(0, 2);
  lcd.print(F("< Pulse Rate (BPM) >"));

  while (!done_taking_bpm) {
    //GET Current time
    unsigned long currentMillis = millis();
    //First event
    if (myTimer1(delayTime, currentMillis) == 1) {
      reading = analogRead(0);

      //Heart beat leading edge detected.

      if (reading > UpperThreshold && IgnoreReading == false) {
        if (FirstPulseDetected == false) {
          FirstPulseTime = millis();
          FirstPulseDetected = true;
        }
        else {
          SecondPulseTime = millis();
          PulseInterval = SecondPulseTime - FirstPulseTime;
          FirstPulseTime = SecondPulseTime;
        }
        IgnoreReading = true;
        digitalWrite(LED_BUILTIN, HIGH);
      }
      //Heart beat trailing edge detected.
      if (reading < LowerThreshold && IgnoreReading == true) {
        IgnoreReading = false;
        digitalWrite(LED_BUILTIN, LOW);
      }
      //calculate beats per minute.
      BPM = (1.0 / PulseInterval) * 60.0 * 1000;
    }
    // second event
    if (myTimer2(delayTime2, currentMillis) == 1) {
      lcd.setCursor(9, 3);
      lcd.print(String(BPM));
      Serial.flush();
    }
    char key_pressed = key.getKey();
    if (key_pressed) {
      char c = key_pressed;
      if (c == '#' or c == 'C') {
        info_data[2] = String(BPM);
        clearScreen();
        lcd.setCursor(0, 2);
        lcd.print("  Pulse Rate Value  ");
        lcd.setCursor(0, 3);
        lcd.print("Stored Successfully!");
        delay(2000);
        done_taking_bpm = true;
      }
    }


  }
}

int myTimer1(long delayTime, long currentMillis) {
  if (currentMillis - previousMillis >= delayTime) {
    previousMillis = currentMillis;
    return 1;
  }
  else {
    return 0;
  }
}

int myTimer2(long delayTime2, long currentMillis) {
  if (currentMillis - previousMillis2 >= delayTime2) {
    previousMillis2 = currentMillis;
    return 1;
  }
  else {
    return 0;
  }
}

/************************End of HeartBeat Code***********************************/

/***********************Writing of function for body temperature**********/

void checkBodyTemp() {
  bool done_taking_temp = false;
  clearScreen();
  lcd.setCursor(0, 2);
  lcd.print(F("Body Temperature (C)"));
  lcd.setCursor(9, 3);
  while (!done_taking_temp) {
    float temp = max.temperature(RNOMINAL, RREF);
    lcd.setCursor(9, 3);
    lcd.print(String(temp));
    checkFault();
    delay(200);
    char key_pressed = key.getKey();
    if (key_pressed) {
      char c = key_pressed;
      if (c == '#' or c == 'C') {
        info_data[1] = String(temp);
        clearScreen();
        lcd.setCursor(0, 2);
        lcd.print("   Body Temp data   ");
        lcd.setCursor(0, 3);
        lcd.print("Stored Successfully!");
        delay(2000);
        done_taking_temp = true;
      }
    }
  }
}

void checkFault() {
  // Check and print any faults
  uint8_t fault = max.readFault();
  if (fault) {
    Serial.print("Fault 0x"); Serial.println(fault, HEX);
    if (fault & MAX31865_FAULT_HIGHTHRESH) {
      Serial.println("RTD High Threshold");
    }
    if (fault & MAX31865_FAULT_LOWTHRESH) {
      Serial.println("RTD Low Threshold");
    }
    if (fault & MAX31865_FAULT_REFINLOW) {
      Serial.println("REFIN- > 0.85 x Bias");
    }
    if (fault & MAX31865_FAULT_REFINHIGH) {
      Serial.println("REFIN- < 0.85 x Bias - FORCE- open");
    }
    if (fault & MAX31865_FAULT_RTDINLOW) {
      Serial.println("RTDIN- < 0.85 x Bias - FORCE- open");
    }
    if (fault & MAX31865_FAULT_OVUV) {
      Serial.println("Under/Over voltage");
    }
    max.clearFault();
  }
}

/*********************End of Body Temperature function*****************/
void checkRespRate() {
  lcd.print("Respiratory Rate");
}
void checkBloodPres() {
  lcd.print("Blood Pressure");
}
void saveToSD() {
  lcd.print("Saving data to SD");
}
void transferDataOnline() {
  lcd.print("Sending Online...");


  info_data[6]
  // 0: patient id, 1: heartbeat data,2: body temp 3: resp rate 4 blood pressur
  //

}
