#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "WiFiEsp.h"

#define HASH "device_hash" //device hash(we are not using token)

char ssid[] = "Room-C22";
char pass[] = "hellonet123";

int status = WL_IDLE_STATUS;     // the Wifi radio's status

char server[] = "3.134.95.133";
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
  client.setServer( server, 1883 );
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


//this function simple tries to reconnect to the MQTT server
//topics are subscribe to 
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


    if ( client.connect(" Device", HASH, NULL) ) {
      Serial.println( "Sucessful connection with MQTT server" );
      //Subscribe to the required topics and the topics meant for callback
      subscribeToTopics();
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
//response is displayed on LCD if necessary
//or response is saved in EEPROM e.g device_id or comment or notification
void on_message(const char* topic, byte* payload, unsigned int length) {
  Serial.println("Response received");

  //it ashould be replaced with the expected number of array element for vita data
  int expectedNumberofElement = 2;


  char json[length + 1];
  strncpy (json, (char*)payload, length);
  json[length] = '\0';

  String localTopic = String(topic);
  String decodedTopic = localTopic.substring(0,6); 

  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print(" Decoded Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(json);

  if (decodedTopic.equals("/data/")){
      //extract payload as JSON array
      displayVitaData(json,expectedNumberofElement);
  }
  else {
      //any other case is normal JSON object
      respondToMQTT(json);
  }

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



/**************************MQTT Utils******************************************/

void transferDataOnline() {
  lcd.print("Sending Online...");

  //send vita
  sendVita();
}

void subscribeToTopics(){

  //subcribes to avaiable topics

  client.subscribe("/device/error");    ///device/error/{hash}
  client.subscribe("/device/register");   ///device/register/{hash}
  client.subscribe("/data/vita");     ///data/vita/{patient_id}
  client.subscribe("/device/comment");    //device/comment/{device_id}
  client.subscribe("/device/notification");    //device/notification/{device_id}
  client.subscribe("/device/vita-received");   ///device/vita-received/

  // Security protocol can follow


}



//this function sends patient vita to the website
//
void sendVita(){

  String payload; 

  const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4);
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& root = jsonBuffer.createObject();

  JsonObject& data = root.createNestedObject("data");
  data["resp_rate"] = info_data[3];
  data["pulse_rate"] = info_data[2];
  data["temperature"] = info_data[1];
  data["blood_pressure"] = info_data[4];
  root["hash"] = "12345";   //still thinking about a proper format


 //Just print to debug
  root.printTo(payload);
  root.printTo(Serial);

  //we still need device id, this one for testing purpose
  client.publish("/patient/vita/335f017e-8161-4e6a-b33e-6a627c8e36ef", payload.c_str());  ///patient/vita/{device_id}
}



//function to register device
// this function should be used the first time
void registerDevice(){

  String payload;

  const size_t capacity = JSON_OBJECT_SIZE(2);
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& root = jsonBuffer.createObject();


  //hospital id and admin are for testing purpose, we need a way to get it automatically
  root["hospital_id"] = "1";
  root["admin_id"] = "2";


  root.printTo(Serial);  // for debug purpose
  root.printTo(payload);


  ///device/register/{hash}  (how do we get hash when we haven't even registered?)
  client.publish("/device/register", payload.c_str()); 

}


//this functioin tries to decode any server response aside from vitas
//it extracts a particular string from the JsonDocument and responds accordingly 
void respondToMQTT(String json){
  // StaticJsonBuffer<200> jsonBuffer;
  const size_t capacity = JSON_OBJECT_SIZE(2) + 100;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& data = jsonBuffer.parseObject(String(json));

  if (!data.success())
  {
    Serial.println("parseObject() failed");
    return;
  }

  if (data["patient_id"] != NULL){
    if(data["comment"] != NULL){
      //respond to topic /device/comment/{device_id}
      //respond to comment received
      String patient_id = data["patient_id"];
      String comment = data["comment"];
      showComment(patient_id,comment);
      return;
    }


    //respond to the topic /device/vita-received/{hash}
    //show "vita has been sent succesfully"
    return;

  }
  else if(data["message"] != NULL){
    //respond topic /device/notification/{device_id}
    //display notification
    String message = data["message"];
    showNotication(message);

  }else if (data["device_id"] != NULL){

    //respond to topic /device/register/{hash}
    //save device id into the EEPROM
    saveToSD();

  }
}


void displayVitaData(String json, int expectedNumberofElement){
  // respond to topic /data/vita/{patient_id}

  const size_t capacity = JSON_ARRAY_SIZE(expectedNumberofElement) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 390;
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonArray& root = jsonBuffer.parseArray(json);
  
  if (!root.success())
  {
    Serial.println("parseObject() failed");
    return;
  }

  JsonObject& root_0 = root[0];
  char* root_0_id = root_0["id"]; // "f5efc201-9c6c-4934-a9ec-4063f0474237"
  char* root_0_patient_id = root_0["patient_id"]; // "5997286f-b603-44b7-88d4-fd82118e150b"
  char* root_0_device_id = root_0["device_id"]; // "9c842b3c-6239-4c7f-b4ec-424bc5e498d8"

  JsonObject& root_0_data = root_0["data"];
  int root_0_data_resp_rate = root_0_data["resp_rate"]; // 991
  int root_0_data_pulse_rate = root_0_data["pulse_rate"]; // 136
  char* root_0_data_location_id = root_0_data["location_id"]; // "14izeOzo"
  int root_0_data_temperature = root_0_data["temperature"]; // 30
  int root_0_data_blood_pressure = root_0_data["blood_pressure"]; // 835

  char* root_0_comment = root_0["comment"]; // "Loba atab"
  char* root_0_doctor_id = root_0["doctor_id"]; // "2dc89f7e-475a-4a1c-94fd-b5ceb52366bf"
  char* root_0_created_at = root_0["created_at"]; // "2019-10-25 12:10:12"
  char* root_0_updated_at = root_0["updated_at"]; // "2019-10-25 12:19:04"



  //Write a function to display the received vita on screen
  

}

//Function to Display comment on the screen
void showComment(String patient_id,String comment){

  //LCD commands to write to screen

}


//Function to  Displays notification
void showNotication(String comment){

    //LCD commands to write to screen

}
