#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <string.h>  //used for some string handling and processing.
#include <mpu6050_esp32.h>


TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
char network[] = "MIT GUEST";
char password[] = "";
uint8_t scanning = 0;
uint8_t channel = 1; //network channel on 2.4 GHz
byte bssid[] = {0x04, 0x95, 0xE6, 0xAE, 0xDB, 0x41}; //6 byte MAC address of AP you're targeting.

//Some constants and some resources:
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int GETTING_PERIOD = 2000; //periodicity of getting a number fact.
const int BUTTON_TIMEOUT = 1000; //button timeout in milliseconds
const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
const float ZOOM = 9.81; //for display (converts readings into m/s^2)...used for visualizing only

const int BUTTON = 45; //pin connected to button 
const uint8_t UNPRESSED = 1;  //change if you'd like
const uint8_t PRESSED = 0;  //change if you'd like
int modeState = 0;
int ALWAYS_ON = 0; //0 is always on mode
int CHECK_ON = 1;
int ACTIVATED = 1;
int DEACTIVATED = 0;
int screenState = ACTIVATED;
int mode2State = ALWAYS_ON;
int buttonState = UNPRESSED;
int button2State = UNPRESSED;
unsigned long minutetimer = 0; //
unsigned long fifteentimer = 0;
unsigned long secondtimer = 0;
unsigned long halfsecondtimer = 0;
char* resbuff;
int call = 1;
const int BUTTON2 = 39;

int PRESENT = 0;
int NOTPRESENT = 1;
int colonState = PRESENT;

int AM = 0;
int PM = 1;
int timeScale = AM;


char timet[30];
char hoursc[5];
char minsc[5];
char secsc[5];
int hours;
int mins;
int secs;

MPU6050 imu; //imu object called, appropriately, imu


float old_acc_mag;  //previous acc mag
float older_acc_mag;  //previous prevoius acc mag
//some suggested variables you can use or delete:

float acc_mag = 0;  //used for holding the magnitude of acceleration
float avg_acc_mag = 0; //used for holding the running average of acceleration magnitude
float x, y, z; //variables for grabbing x,y,and z values


void setup() {

  Wire.begin();
  delay(50); //pause to make sure comms get set up
  if (imu.setupIMU(1)) {
    Serial.println("IMU Connected!");
  } else {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }

  // put your setup code here, to run once:
  tft.init();  //init screen
  tft.setRotation(1); //adjust rotation
  tft.setTextSize(3); //default font size
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to green foreground, black background
  Serial.begin(115200); //begin serial comms


    if (scanning){
      int n = WiFi.scanNetworks();
      Serial.println("scan done");
    if (n == 0) {
      Serial.println("no networks found");
    } else {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {
        Serial.printf("%d: %s, Ch:%d (%ddBm) %s ", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "");
        uint8_t* cc = WiFi.BSSID(i);
        for (int k = 0; k < 6; k++) {
          Serial.print(*cc, HEX);
          if (k != 5) Serial.print(":");
          cc++;
        }
        Serial.println("");
      }
    }
  }
  delay(100); //wait a bit (100 ms)

  //if using regular connection use line below:
  WiFi.begin(network, password);
  //if using channel/mac specification for crowded bands use the following:
  //WiFi.begin(network, password, channel, bssid);
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count<6) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n",WiFi.localIP()[3],WiFi.localIP()[2],
                                            WiFi.localIP()[1],WiFi.localIP()[0], 
                                          WiFi.macAddress().c_str() ,WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  pinMode(BUTTON, INPUT_PULLUP); //set input pin as an input!
  pinMode(BUTTON2, INPUT_PULLUP);
  updateTime();
  minutetimer = millis();
  halfsecondtimer = millis();
}

void updateTime() {
    int i = 0; //start at 0
    do {
        request_buffer[i] = ("GET http://iesc-s3.mit.edu/esp32test/currenttime HTTP/1.1\r\n")[i]; //assign s[i] to the string literal index i
    } while(request_buffer[i++]); //continue the loop until the last char is null
    strcat(request_buffer,"Host: iesc-s3.mit.edu\r\n");
    strcat(request_buffer,"\r\n");
    do_http_GET("iesc-s3.mit.edu", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
    Serial.println("printing response buffer from update time");
    Serial.println(response_buffer);
    strncpy (hoursc, response_buffer+11, 2);
    strncpy (minsc, response_buffer+14, 3);
    strncpy (secsc, response_buffer+17, 3);
    secsc[4] = '\0';
    hours = atoi(hoursc);
    if (hours>12) {
      hours = hours - 12;
      timeScale = PM;
    }
    else if (hours==0) {
      hours = 12;
      timeScale = AM;
    }
    else if (hours==12) {
      timeScale = PM;
    }
    else {
      timeScale = AM;
    }
    mins = atoi(minsc);
    secs = atoi(secsc);
}

void loop() {

    uint8_t button = digitalRead(BUTTON);
    uint8_t button2 = digitalRead(BUTTON2);

    if (modeState==0) { //in hour minute mode
      if (colonState==PRESENT) {
        if (millis() - halfsecondtimer > 500) {
          Serial.println("inside halfsecond timer conditional");
          colonState = NOTPRESENT;
          //sprintf(timet, "%d %d PM  ", hours, mins);
            if (mins<10 && timeScale==AM) {
              sprintf(timet, "%d 0%d AM     ", hours, mins);
            }
            else if (mins<10 && timeScale==PM) {
              sprintf(timet, "%d 0%d PM     ", hours, mins);
            }
            else if (timeScale==AM) {
              sprintf(timet, "%d %d AM      ", hours, mins);     
            }
            else if (timeScale==PM) {
              sprintf(timet, "%d %d PM     ", hours, mins);               
            }
          tft.setCursor(0, 0, 1); //set cursor in upper right (upper right corner is 0,0 index...make font size 1)
          tft.println(timet);
          halfsecondtimer = millis();
        }
      }
      else if (colonState==NOTPRESENT) {
        if (millis() - halfsecondtimer > 500) {
          Serial.println("HEREEE");
          colonState = PRESENT;
            if (mins<10 && timeScale==AM) {
              sprintf(timet, "%d:0%d AM     ", hours, mins);
            }
            else if (mins<10 && timeScale==PM) {
              sprintf(timet, "%d:0%d PM     ", hours, mins);
            }
            else if (timeScale==AM) {
              sprintf(timet, "%d:%d AM     ", hours, mins);     
            }
            else if (timeScale==PM) {
              sprintf(timet, "%d:%d PM     ", hours, mins);               
            }
          tft.setCursor(0, 0, 1); //set cursor in upper right (upper right corner is 0,0 index...make font size 1)
          tft.println(timet);
          halfsecondtimer = millis();
        }
      }
    }    

    if (millis() - minutetimer > 60000) {
      updateTime();
      minutetimer = millis();
    }

    if (millis() - secondtimer >=1000) {
        secs+=1;
        if (secs>59) {
          secs = 0;
          mins +=1;
        }
        secondtimer = millis();
    }

    //checking if button is pressed
    if (button==0 && buttonState == UNPRESSED) {
      buttonState = PRESSED;
    }
    //checking if button is released right after a press
    else if (button == 1 && buttonState == PRESSED && screenState == ACTIVATED) {
      if (modeState==1) {
        modeState = 0;
        if (mins<10 && timeScale==AM) {
          sprintf(timet, "%d:0%d AM      ", hours, mins);
        }
        else if (mins<10 && timeScale==PM) {
          sprintf(timet, "%d:0%d PM      ", hours, mins);
        }
        else if (timeScale==AM) {
          sprintf(timet, "%d %d AM      ", hours, mins);     
        }
        else if (timeScale==PM) {
          sprintf(timet, "%d %d PM      ", hours, mins);               
        }
        tft.setCursor(0, 0, 1); //set cursor in upper right (upper right corner is 0,0 index...make font size 1)
        tft.println(timet);
      }
      else if (modeState==0 && screenState == ACTIVATED) {
        modeState = 1;
        tft.setCursor(0, 0, 1); //set cursor in upper right (upper right corner is 0,0 index...make font size 1)
        if (secs<10 && timeScale == AM) {
          sprintf(timet, "%d:%d:0%d AM", hours, mins, secs);
         }
        else if (secs<10 && timeScale == PM) {
          sprintf(timet, "%d:%d:0%d PM", hours, mins, secs);
         }
        else if (secs<10 && mins<10 && timeScale == AM) {
          sprintf(timet, "%d:0%d:0%d AM", hours, mins, secs);
        }
        else if (secs<10 && mins<10 && timeScale == PM) {
          sprintf(timet, "%d:0%d:0%d PM", hours, mins, secs);
        }
        else if (timeScale == AM) {
          sprintf(timet, "%d:%d:%d AM", hours, mins, secs);
        }
        else if (timeScale == PM) {
          sprintf(timet, "%d:%d:%d PM", hours, mins, secs);
        }
        tft.println(timet);
        
      }
      buttonState = UNPRESSED;
    }
    else if (modeState==1 && screenState == ACTIVATED) {
      tft.setCursor(0, 0, 1); //set cursor in upper right (upper right corner is 0,0 index...make font size 1)
      if (secs<10 && timeScale == AM) {
          sprintf(timet, "%d:%d:0%d AM", hours, mins, secs);
      }
      else if (secs<10 && timeScale == PM) {
        sprintf(timet, "%d:%d:0%d PM", hours, mins, secs);
      }
      else if (secs<10 && mins<10 && timeScale == AM) {
        sprintf(timet, "%d:0%d:0%d AM", hours, mins, secs);
      }
      else if (secs<10 && mins<10 && timeScale == PM) {
        sprintf(timet, "%d:0%d:0%d PM", hours, mins, secs);
      }
      else if (timeScale == AM) {
        sprintf(timet, "%d:%d:%d AM", hours, mins, secs);
      }
      else if (timeScale == PM) {
        sprintf(timet, "%d:%d:%d PM", hours, mins, secs);
      }     
      tft.println(timet);
      if (millis() - secondtimer >=1000) {
        secs+=1;
        if (secs>59) {
          secs = 0;
          mins +=1;
        }
        secondtimer = millis();
      }
    }


    imu.readAccelData(imu.accelCount);
    x = ZOOM * imu.accelCount[0] * imu.aRes;
    y = ZOOM * imu.accelCount[1] * imu.aRes;
    z = ZOOM * imu.accelCount[2] * imu.aRes;
    acc_mag = sqrt(x*x + y*y + z*z);
    //Serial.print("acc mag is:");
    //Serial.println(acc_mag);

    if (button2==0 && button2State == UNPRESSED) {
      button2State = PRESSED;
      //Serial.println("button2 has been pressed");
    }
    else if (button2 == 1 && button2State == PRESSED) {
      button2State = UNPRESSED;
      //Serial.println("button2 has been released after press");
      if (mode2State==ALWAYS_ON) {
        //Serial.println("changing to check time mode");
        mode2State = CHECK_ON;
        tft.fillScreen(TFT_BLACK); //fill background
        screenState = DEACTIVATED;        
      }
      else if (mode2State == CHECK_ON) {
        mode2State = ALWAYS_ON;
        screenState = ACTIVATED;
      }
    }

    if (mode2State==CHECK_ON && acc_mag>18 && modeState==1) {
        fifteentimer = millis();
        tft.setCursor(0, 0, 1); //set cursor in upper right (upper right corner is 0,0 index...make font size 1)
        if (secs<10 && timeScale == AM) {
          sprintf(timet, "%d:%d:0%d AM", hours, mins, secs);
         }
        else if (secs<10 && timeScale == PM) {
          sprintf(timet, "%d:%d:0%d PM", hours, mins, secs);
         }
        else if (secs<10 && mins<10 && timeScale == AM) {
          sprintf(timet, "%d:0%d:0%d AM", hours, mins, secs);
        }
        else if (secs<10 && mins<10 && timeScale == PM) {
          sprintf(timet, "%d:0%d:0%d PM", hours, mins, secs);
        }
        else if (timeScale == AM) {
          sprintf(timet, "%d:%d:%d AM", hours, mins, secs);
        }
        else if (timeScale == PM) {
          sprintf(timet, "%d:%d:%d PM", hours, mins, secs);
        }
        tft.println(timet);
        screenState = ACTIVATED;
    }
    else if (mode2State == CHECK_ON && acc_mag>18 && modeState==0) {
      fifteentimer = millis();
      if (mins<10 && timeScale==AM) {
          sprintf(timet, "%d:0%d AM  ", hours, mins);
        }
        else if (mins<10 && timeScale==PM) {
          sprintf(timet, "%d:0%d PM  ", hours, mins);
        }
        else if (timeScale==AM) {
          sprintf(timet, "%d %d AM  ", hours, mins);     
        }
        else if (timeScale==PM) {
          sprintf(timet, "%d %d PM  ", hours, mins);               
        }
      tft.setCursor(0, 0, 1); //set cursor in upper right (upper right corner is 0,0 index...make font size 1)
      tft.println(timet);
    }

    if (millis() - fifteentimer > 15000) {
      tft.fillScreen(TFT_BLACK); //fill background
      screenState = DEACTIVATED;
    }

}


void do_http_GET(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
  WiFiClient client; //instantiate a client object
  Serial.println(host);
  Serial.println(request_buffer);
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n',response,response_size);
      if (serial) Serial.println(response);
      if (strcmp(response,"\r")==0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis()-count>response_timeout) break;
    }
    memset(response, 0, response_size);  
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response,client.read(),OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");  
  }else{
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}  

/*----------------------------------
 * char_append Function:
 * Arguments:
 *    char* buff: pointer to character array which we will append a
 *    char c: 
 *    uint16_t buff_size: size of buffer buff
 *    
 * Return value: 
 *    boolean: True if character appended, False if not appended (indicating buffer full)
 */
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
        int len = strlen(buff);
        if (len>buff_size) return false;
        buff[len] = c;
        buff[len+1] = '\0';
        return true;
}
