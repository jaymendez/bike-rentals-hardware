
//Wifi Library
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

//Firebase
#include <ArduinoJson.h>
#include <FirebaseArduino.h>

//LCD
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

//GPS
#include <TinyGPS++.h> // library for GPS module
#include <SoftwareSerial.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
TinyGPSPlus gps;         // TinyGPS++ object
SoftwareSerial ss(4, 5); // GPS Serial connection 

ESP8266WebServer server;
uint8_t pin_led = 16;

char* ssid = "freewifi";
char* password = "12345678";

//Static IP address configuration
IPAddress staticIP(192, 168, 100, 69); //ESP8266 static ip
IPAddress gateway(192, 168, 100, 1); //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0); //Subnet mask
IPAddress dns(8, 8, 8, 8); //DNS

#define FIREBASE_HOST "bike-rentals-dlsu.firebaseio.com"
#define FIREBASE_AUTH "fUUKHLJFUS3OplIzugNVLbUfnNxRgcvWwiuQ30r5"

//Variable Declaration
float  gv_latitude, 
       gv_longitude;
String gv_lat_str, 
       gv_lng_str;
int H, M, S;
int isTimerReady = 0;
int isIP = 0;
int period = 1000;
unsigned long time_now = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  ss.begin(9600);
  
  pinMode(pin_led, OUTPUT);
  Wire.begin(D6, D5);
  lcd.begin();
  lcd.home();
  lcd.print("Waiting for");
  lcd.setCursor(0, 1);
  lcd.print("response");

  //Connect to Wifi
//  WiFi.config(staticIP, subnet, gateway, dns);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP Address is : ");
  Serial.println(WiFi.localIP());     

  //Initialize connection to firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  //Initialize server  
  server.on("/", []() {
    server.send(200, "text/plain", "Hello World!");
  });
  server.on("/print", requestRecieved );
  server.begin();
  sendIP();
}

void loop() {

  // put your main code here, to run repeatedly:
//  Serial.println("loop");
  getGps();
  server.handleClient();
  
  if (isTimerReady && !(ss.available() > 0) ) {
    timer();
  }
//  Serial.println("endLoop");
}

void setLcdMsg(String msg1, String msg2) {
  lcd.clear();
  lcd.print(msg1);
  lcd.setCursor(0, 1);
  lcd.print(msg2);
}

void sendFirebaseMsg(String msg1, String msg2) {       
  Serial.println("Sending");
  Firebase.setString("/bike1/coordinates/lat", msg1);
  Firebase.setString("/bike1/coordinates/lng", msg2);
  Firebase.setString("/bike1/localIP", WiFi.localIP().toString() );
  
  if (Firebase.failed()) {
    Serial.print("Push failed:");
    Serial.println(Firebase.error());
    return;
  }
  Serial.println("Sent to Firebase");
}

void requestRecieved() {
  Serial.println("received request");
  lcd.clear();
  String data = server.arg("plain");
  Serial.println(data);
  
  StaticJsonBuffer<200> jBuffer;
  JsonObject& jObject = jBuffer.parseObject(data);
  String msg1 = jObject["message"];
  String msg2 = jObject["message2"];
  H = jObject["hours"];
  M = jObject["minutes"];
  S = jObject["seconds"];
  setLcdMsg(msg1, msg2);
  
  lcd.clear();
  server.send(204, "success");
  Serial.println(msg2);
  if (msg2 == "start") {
//    isTimerReady = 1;
    lcd.clear();
    lcd.print("Device Start");
  } else if (msg2 == "stop") {
    isTimerReady = 0;
    Serial.println("STOP");
    lcd.clear();
    lcd.print("Device Stopped");
//    timeLoop(millis(), 2000);
    delay(2000);
    lcd.clear();
    lcd.print("Waiting for");
    lcd.setCursor(0, 1);
    lcd.print("response");
  } else if (msg2 == "overtime") {
//    Overtime
    lcd.clear();
    lcd.print ("  Overtime  ");
    lcd.setCursor(7, 1);
    lcd.print(":");
    lcd.setCursor(5, 1);
    if (H > 9) {
      lcd.print(H);
    } else {
      lcd.print("0");
      lcd.print(H);
    }
    lcd.setCursor(8, 1);
    if (M > 9) {
      lcd.print(M);
    } else {
      lcd.print("0");
      lcd.print(M);
    }
  } else if (msg2 == "time_left") {
//    Time Left
    lcd.clear();
    lcd.print ("  Time Left  ");
    lcd.setCursor(7, 1);
    lcd.print(":");
    lcd.setCursor(5, 1);
    if (H > 9) {
      lcd.print(H);
    } else {
      lcd.print("0");
      lcd.print(H);
    }
    lcd.setCursor(8, 1);
    if (M > 9) {
      lcd.print(M);
    } else {
      lcd.print("0");
      lcd.print(M);
    }
  } else if (msg2 == "outside_geofence") {
//    Outside Geofence
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Outside");
    lcd.print("Geofence");
    lcd.setCursor(1, 1);
  } 
  
//  sendFirebaseMsg(msg1, msg2);

  
  
}

void timer() {
  lcd.setCursor(1, 0);
//  lcd.print ("Tutorial45.com");
  lcd.print ("  Bike Timer  ");
  lcd.setCursor(6, 1);
  lcd.print(":");
  lcd.setCursor(9, 1);
  lcd.print(":");
  if (S == NULL && M == NULL && H == NULL)
  {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print ("TIME IS UP");
    isTimerReady = 0;
    return;
  }
  S--;
  timeLoop(millis(), 1000);
  
  if (S % 5) {
    getGps();
    
  }
  
  if (S < 0)
  {
    M--;
    S = 59;
  }
  if (M < 0)
  {
    H--;
    M = 59;
  }
  if (H < 0) {
    H = 23;
    M = 59;
    S = 59;
  } if (M > 9)
  {
    lcd.setCursor(7, 1);
    lcd.print(M);
  }
  else
  {
    lcd.setCursor(7, 1);
    lcd.print("0");
    lcd.setCursor(8, 1);
    lcd.print(M);
    lcd.setCursor(9, 1);
    lcd.print(":");
  }

  if (S > 9)
  {
    lcd.setCursor(10, 1);
    lcd.print(S);
  }
  else
  {
    lcd.setCursor(10, 1);
    lcd.print("0");
    lcd.setCursor(11, 1);
    lcd.print(S);
    lcd.setCursor(12, 1);
    lcd.print(" ");
  }

  if (H > 9)
  {
    lcd.setCursor(4, 1);
    lcd.print (H);
  }
  else
  {
    lcd.setCursor(4, 1);
    lcd.print("0");
    lcd.setCursor(5, 1);
    lcd.print(H);
    lcd.setCursor(6, 1);
    lcd.print(":");
  }
}

void timeLoop (long int startMillis, long int interval) { // the delay function
  // this loops until interval mS has passed
  while (millis() - startMillis < interval) {}
}

void getGps() {
  // if data is available  
//  Serial.println("this is gps");

  if ((ss.available() > 0)) 
  {
    if (gps.encode(ss.read())) //read gps data
    {
      if (gps.location.isValid()) //check whether gps location is valid
      {
        //Get GPS values
        gv_latitude  = gps.location.lat();
        gv_longitude = gps.location.lng();

        //Store GPS values on string for display
        gv_lng_str   = String(gv_longitude , 6); 
        gv_lat_str   = String(gv_latitude  , 6);  

        //Display Data
        Serial.println ("Location:");
        Serial.print   ("Latitude:");
        Serial.println (gv_lat_str);
        Serial.print   ("Longitude:");
        Serial.println (gv_lng_str);

        sendFirebaseMsg(gv_lat_str, gv_lng_str);
        timeLoop(millis(), 1000);
     }
    }
  }
  if (!isTimerReady) {
    sendIP();
  }
}

void sendIP() {
  if (!isIP) {
    Firebase.setString("/bike1/localIP", WiFi.localIP().toString() );
    if (Firebase.failed()) {
      Serial.print("IP Failed to Push:");
      Serial.println(Firebase.error());
      return;
    } else {
      Serial.println("IP SENT");
      isIP = 1;
    }
  }
}
