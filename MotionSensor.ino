// ESP32 Touch Test
// Just test touch pin - Touch0 is T0 which is on GPIO 4.

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WebServer.h>
#include <Wire.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#include "ESP32_MailClient.h"

#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex
#define MS2_TO_GS 9.80665

Adafruit_MMA8451 mma = Adafruit_MMA8451();
WebServer server(80);

const int BUTTON_PIN = 27;
const int VIBRATION_PIN =  15;
const int ACCEL_PWR_PIN =  21;


int ACCEL_SAMPLES_MAX = 10;
int ACCEL_SAMPLES_DELAY = 10;
int ACCEL_THRESHHOLD_GS = 2;

//admin password
const char* www_username = "admin";
const char* www_password = "esp32";

//wifi settings
char* ssid     = "";
char* password = "";

//Email Settings
char* sms_email = "";

// To send Email using Gmail use port 465 (SSL) and SMTP Server smtp.gmail.com
// YOU MUST ENABLE less secure app option https://myaccount.google.com/lesssecureapps?pli=1
#define emailSenderAccount     "highnote.notifier@gmail.com"
#define emailSenderPassword    "zgdzvgvgtrriwrbq"
#define smtpServer             "smtp.gmail.com"
#define smtpServerPort          465


/*
  AT&T: number@txt.att.net (SMS), number@mms.att.net (MMS)
  T-Mobile: number@tmomail.net (SMS & MMS)
  Verizon: number@vtext.com (SMS), number@vzwpix.com (MMS)
  Sprint: number@messaging.sprintpcs.com (SMS), number@pm.sprint.com (MMS)
  XFinity Mobile: number@vtext.com (SMS), number@mypixmessages.com (MMS)
  Virgin Mobile: number@vmobl.com (SMS), number@vmpix.com (MMS)
  Tracfone: number@mmst5.tracfone.com (MMS)
  Metro PCS: number@mymetropcs.com (SMS & MMS)
  Boost Mobile: number@sms.myboostmobile.com (SMS), number@myboostmobile.com (MMS)
  Cricket: number@sms.cricketwireless.net (SMS), number@mms.cricketwireless.net (MMS)
  Republic Wireless: number@text.republicwireless.com (SMS)
  Google Fi (Project Fi): number@msg.fi.google.com (SMS & MMS)
  U.S. Cellular: number@email.uscc.net (SMS), number@mms.uscc.net (MMS)
  Ting: number@message.ting.com
  Consumer Cellular: number@mailmymobile.net
  C-Spire: number@cspire1.com
  Page Plus: number@vtext.com
*/



void setup()
{
  Serial.begin(115200);
  delay(500); //delay to bring up serial monitor

  pinMode(ACCEL_PWR_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  digitalWrite(LED_BUILTIN, HIGH);

  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }

  unpackConfig();

  /*
    First we configure the wake up source
    We set our ESP32 to wake up for an external trigger.
    There are two types for ESP32, ext0 and ext1 .
    ext0 uses RTC_IO to wakeup thus requires RTC peripherals
    to be on while ext1 uses RTC Controller so doesnt need
    peripherals to be powered on.
    Note that using internal pullups/pulldowns also requires
    RTC peripherals to be turned on.
  */
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_15, 0); //1 = High, 0 = Low




  if (touchRead(BUTTON_PIN) != 0) {

    WiFi.softAP("MotionSensor");
    IPAddress myIP = WiFi.softAPIP();
    Serial.println(myIP);
    startServer();
    
  } else {


    bool movementTriggered = sampleAcceleromter();
    if (movementTriggered) {
      //connect to wifi
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      sendNotification();
    }

    //Go to sleep
    Serial.println(F("Sleeping"));
    //esp_deep_sleep_start();
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void unpackConfig() {
  StaticJsonDocument<200> doc;

  //GetConfig data
  String configData = readFile(SPIFFS, "/config.json");
  Serial.print("ConfigData: ");
  Serial.println(configData);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, configData);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }



  const char* config_ssid             = doc[F("ssid")];
  const char* config_password            = doc[F("password")];
  int         config_ACCEL_SAMPLES_MAX   = doc[F("accel_samples_max")];
  int         config_ACCEL_SAMPLES_DELAY = doc[F("accel_samples_delay_s")];
  int         config_ACCEL_THRESHHOLD_GS = doc[F("accel_threshold_gs")];
  const char* config_sms_email           = doc[F("sms_email")];

  ACCEL_SAMPLES_MAX = config_ACCEL_SAMPLES_MAX;
  ACCEL_SAMPLES_DELAY = config_ACCEL_SAMPLES_DELAY;
  ACCEL_THRESHHOLD_GS = config_ACCEL_THRESHHOLD_GS;

  ssid = copy(config_ssid);
  password = copy(config_password);
  sms_email = copy(config_sms_email);


/*
              // Print values.
              Serial.println("UNPACKING CONFIG");
  Serial.printf("SSID: %s\r\n", ssid);
  Serial.printf("ACCEL_SAMPLES_MAX: %s\r\n", ACCEL_SAMPLES_MAX);
  Serial.printf("ACCEL_SAMPLES_DELAY: %s\r\n", ACCEL_SAMPLES_DELAY);
  Serial.printf("ACCEL_THRESHHOLD_GS: %s\r\n", ACCEL_THRESHHOLD_GS);
  Serial.printf("sms_email: %s\r\n", sms_email);
  */
}

char* copy(const char* orig) {
  char *res = new char[strlen(orig) + 1];
  strcpy(res, orig);
  return res;
}
///////////////////SERVER

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? F("GET") : F("POST");
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, F("text/plain"), message);

}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(F(".html"))) return F("text/html");
  else if (filename.endsWith(F(".css"))) return F("text/css");
  else if (filename.endsWith(F(".js"))) return F("application/javascript");
  else if (filename.endsWith(F(".ico"))) return F("image/x-icon");
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}


void startServer() {
  Serial.println("Starting Server....");


  server.on("/", []() {
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    handleFileRead("/index.html");
    //server.send(200, "text/plain", "Login OK");
  });

//  
//  server.on("/", []() {
//    
//  });

  server.on("/getconfig", []() {
    String configData = readFile(SPIFFS, "/config.json");
    server.send(200, F("text/json"), configData);
  });

  server.on("/setconfig", []() {
    String newConfigData = server.arg(0);
    writeFile(SPIFFS, "/config.json", newConfigData.c_str() );
    server.send(200, F("text/plain"), F("Settings Saved"));
  });


  server.onNotFound(handleNotFound);
  server.begin();
}


String readFile(fs::FS &fs, const char * path) {
 
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    return "";
  }

  String fileData = "";

  while (file.available()) {
    fileData += file.readString();
  }
  return fileData;
}


void writeFile(fs::FS &fs, const char * path, const char * message) {
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println(F("- file written"));
  } else {
    Serial.println(F("- frite failed"));
  }
}


bool sampleAcceleromter() {

  //Power On Accelerometer
  digitalWrite(ACCEL_PWR_PIN, HIGH);
  delay(100);
  //Connect to accelerometer
  if (! mma.begin()) {
    Serial.println(F("Couldnt start Accelerometer"));
    while (1);
  }

  mma.setRange(MMA8451_RANGE_2_G);

  bool isActivated = false;

  for (int sample = 0; sample <= ACCEL_SAMPLES_MAX; sample++) {
    /* Get a new sensor event */
    sensors_event_t event;
    mma.getEvent(&event);

    float xGs = abs(event.acceleration.x / MS2_TO_GS);
    float yGs = abs(event.acceleration.y / MS2_TO_GS);
    float zGs = abs(event.acceleration.z / MS2_TO_GS);

    /* Display the results (acceleration is measured in m/s^2) 
    Serial.print("X: \t"); Serial.print(xGs); Serial.print("\t");
    Serial.print("Y: \t"); Serial.print(yGs); Serial.print("\t");
    Serial.print("Z: \t"); Serial.print(zGs); Serial.print("\t");
    Serial.println();
    */

    if ( xGs >= ACCEL_THRESHHOLD_GS ||
         yGs >= ACCEL_THRESHHOLD_GS ||
         zGs >= ACCEL_THRESHHOLD_GS ) {
      isActivated = true;
      break;
    }
    delay(250);
  }

  digitalWrite(ACCEL_PWR_PIN, LOW);
  return isActivated;
}



void sendNotification( ) {
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  // Preparing email
  Serial.println(F("Preparing Email"));
  SMTPData smtpData;
  // Set the SMTP Server Email host, port, account and password
  smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);

  // Set the sender name and Email
  smtpData.setSender("Motion", emailSenderAccount);

  // Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
  smtpData.setPriority(F("High"));

  // Set the subject
  smtpData.setSubject(F("Motion"));

  // Set the email message in HTML format
  //smtpData.setMessage("<h2>Photo captured with ESP32-CAM and attached in this email.</h2>", true);
  // Set the email message in text format
  smtpData.setMessage(F("Motion Detected"), false);

  // Add recipients, can add more than one recipient
  smtpData.addRecipient(sms_email);


  //  // Add attach files from SPIFFS
  //  smtpData.addAttachFile(FILE_PHOTO, "image/jpg");
  //  // Set the storage type to attach files in your email (SPIFFS)
  //  smtpData.setFileStorageType(MailClientStorageType::SPIFFS);
  //
  smtpData.setSendCallback(sendCallback);

  Serial.println(F("Sending Email"));
  // Start sending Email, can be set callback function to track the status
  if (!MailClient.sendMail(smtpData)) {
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
  }
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  // Clear all data from Email object to free memory
  smtpData.empty();
}

// Callback function to get the Email sending status
void sendCallback(SendStatus msg) {
  //Print the current status
  Serial.println(msg.info());
}




void loop()
{
  server.handleClient();
}
