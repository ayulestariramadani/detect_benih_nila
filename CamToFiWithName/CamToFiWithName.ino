#include <FirebaseFS.h>
#include <Firebase_ESP_Client.h>
#include <Servo.h>
#include "WiFi.h"
#include "esp_camera.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <SPIFFS.h>
#include <FS.h>
#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include <addons/TokenHelper.h>
#include "NTPClient.h"
#include "WifiUdp.h"

Servo myservo;
//Replace with your network credentials
const char* ssid = "Kemang 8";
const char* password = "SriPurnamasari124";

unsigned long lastTimeCam = 0;
unsigned long lastTimeServo = 0;
unsigned long timerCam = 120000;
unsigned long timerServo = 30000;
int pos = 0;
int i = 1;
int j = 1;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;

// Insert Firebase project API Key
#define API_KEY "AIzaSyBasl3JebppNdfli-LZaOMbIbavALecAZo"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "ayulestariramadani@gmail.com"
#define USER_PASSWORD "123qwes"

// Insert Firebase storage bucket ID e.g bucket-name.appspot.com
#define STORAGE_BUCKET_ID "detect-benih-nila.appspot.com"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://detect-benih-nila-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Photo File Name to save in SPIFFS
//#define fileName "/images/photo.jpg"

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
  
  #ifndef ARDUINO_LOOP_STACK_SIZE
  #ifndef CONFIG_ARDUINO_LOOP_STACK_SIZE
  #define ARDUINO_LOOP_STACK_SIZE 8192
#else
#define ARDUINO_LOOP_STACK_SIZE CONFIG_ARDUINO_LOOP_STACK_SIZE
#endif
#endif

boolean takeNewPhoto = false;

String path = "/images/";
String fileName;

//Define Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;

int intValue;
bool taskCompleted = true;

// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( fileName );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

void deletePhoto( fs::FS &fs ) {
  fs.remove(fileName);
}

// Capture Photo and Save it to SPIFFS
void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly
  do {
    // Take a photo with the camera
    Serial.println("Taking a photo...");

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }
    // Photo file name
    Serial.printf("Picture file name: %s\n", fileName);
    File file = SPIFFS.open(fileName, FILE_WRITE);
    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print(fileName);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    // Close the file
    file.close();
    esp_camera_fb_return(fb);

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);
  } while ( !ok );
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}


void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }
}

void initCamera() {
  // OV2640 camera module
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
}

void sendCapture() {
  if (takeNewPhoto) {
    capturePhotoSaveSpiffs();
    takeNewPhoto = false;
  }
  delay(1);
  if (Firebase.ready() && !taskCompleted) {
    taskCompleted = true;
    Serial.print("Uploading picture... ");

    //MIME type should be valid to avoid the download problem.
    //The file systems for flash and SD/SDMMC can be changed in FirebaseFS.h.
    if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID /* Firebase Storage bucket id */, fileName /* path to local file */, mem_storage_type_flash /* memory storage type, mem_storage_type_flash and mem_storage_type_sd */, fileName /* path of remote file stored in the bucket */, "image/jpeg" /* mime type */)) {
      Serial.printf("\nDownload URL: %s\n", fbdo.downloadURL().c_str());
      deletePhoto(SPIFFS);
    }
    else {
      Serial.println(fbdo.errorReason());
    }
  }
}

void data_from_rtdb(){
  taskCompleted = false;
  if (Firebase.ready()&&!taskCompleted) {
    taskCompleted = true;
    Serial.print("Get the data RTDB... ");
    if (Firebase.RTDB.getInt(&fbdo, "data/jumlah")){
      if (fbdo.dataType() == "int"){
        intValue = fbdo.intData();
        Serial.println(intValue);
      }
      else {
        Serial.println(fbdo.errorReason());
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
  }
  else {
    Serial.println(fbdo.errorReason());
  }
  
}

void capture_send_loop() {
  //  tkCapture.once(86400, capture_send_loop);
  takeNewPhoto = true;
  taskCompleted = false;
  fileName = path + dayStamp + ".jpg";
  sendCapture();

}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  myservo.attach(13);  // attaches the servo on pin 13 to the servo object
  initWiFi();
  initSPIFFS();
  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  initCamera();

  //Firebase
  // Assign the api key
  configF.api_key = API_KEY;
  //Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  //Assign the RTDB URL
  configF.database_url = DATABASE_URL;
  //Assign the callback function for the long running token generation task
  configF.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(28800);
}

void loop() {
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  if ((millis() - lastTimeServo) > timerServo) {
    data_from_rtdb();
    for (i = 1; i <=intValue; i += 1){ 
      for (j = 1; j <=2; j += 1){
        myservo.attach(13);
        for (pos = 0; pos <= 151; pos += 1) { // goes from 0 degrees to 180 degrees
          // in steps of 1 degree
          myservo.write(pos);// tell servo to go to position in variable 'pos'                      // waits 15ms for the servo to reach the position
        }
        delay(1000);
        myservo.detach();
      }
    }
    lastTimeServo = millis();
  }
  if ((millis() - lastTimeCam) > timerCam) {
    // The formattedDate comes with the following format:
    // 2018-05-28T16:00:13Z
    // We need to extract date and time
    
    formattedDate = timeClient.getFormattedDate();
    Serial.println(formattedDate);

    // Extract date
    int splitT = formattedDate.indexOf("T");
    dayStamp = formattedDate.substring(0, splitT);
    Serial.print("DATE: ");
    Serial.println(dayStamp);
    //    Extract time
    //    timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
    //    Serial.print("HOUR: ");
    //    Serial.println(timeStamp);
    capture_send_loop();
    lastTimeCam = millis();
  }
}
