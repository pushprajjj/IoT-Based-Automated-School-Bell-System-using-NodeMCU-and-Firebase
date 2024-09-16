#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266WebServer.h>
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Firebase API key and database URL
#define API_KEY "YOUR_FOREBASE_RTB_API_KEY"
#define DATABASE_URL "YOUR_RTB_URL"
const int ledPin = D1;

WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 19800; // UTC +5:30

NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

ESP8266WebServer server(80);

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Send SMS</h1>";
  html += "<form action='/send' method='post'>";
  html += "Phone Number: <input type='text' name='number'><br>";
  html += "Api Key: <input type='text' name='api'><br>";
  html += "Message: <textarea name='message'>00000000</textarea><br>";
  html += "<input type='submit' value='Send SMS'>";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSend() {
  String phoneNumber = server.arg("number");
  String message = server.arg("message");
  String apiKey = server.arg("api");

  if (phoneNumber.length() > 0 && message.length() > 0 && apiKey.equals("HomeServerUseApiForSMS_Gateway_763422")) {
    bool smsSent = true; // Placeholder for actual SMS sending logic

    if (smsSent) {
      server.send(200, "text/plain", "SMS sent successfully!");
    } else {
      server.send(500, "text/plain", "Failed to send SMS.");
    }
  } else {
    server.send(400, "text/plain", "Invalid request parameters");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize the NTPClient
  timeClient.begin();

  // Set up web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/send", HTTP_POST, handleSend);

  // Start the web server
  server.begin();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Firebase sign-up
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Sign-up successful");
    signupOK = true;
  } else {
    Serial.printf("Sign-up failed: %s\n", config.signer.signupError.message.c_str());
  }

  // Set token status callback
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  delay(50); // Adjust delay as needed for your application

  // Declare the variables here
  String BellHour;
  String BellMinute;
  String bellState;

  if (Firebase.ready() && signupOK) {
    if (Firebase.RTDB.getInt(&fbdo, "/SchoolBell/status")) {
      int status = fbdo.intData();
      if (status == 1) {
        timeClient.update(); // Update the time
        int currentHour = timeClient.getHours();
        int currentMinute = timeClient.getMinutes();
        int currentDayOfWeek = timeClient.getDay();
        Serial.print("Current Time: ");
        Serial.print(currentHour);
        Serial.print(":");
        Serial.println(currentMinute);
        //        Serial.println(currentDayOfWeek);

        bool bellRinging = false;

        for (int i = 1; i <= 9; i++) {
          String pathHour = "/SchoolBell/" + String(i) + "/h";
          String pathMinute = "/SchoolBell/" + String(i) + "/m";
          String pathbellState = "/SchoolBell/" + String(i) + "/state";


          // Retrieve the Bell Hour from Firebase
          if (Firebase.RTDB.getString(&fbdo, pathHour)) {
            BellHour = fbdo.stringData();
          } else {
            Serial.println("Failed to get Bell Hour: " + fbdo.errorReason());
            continue; // Skip to the next iteration
          }

          // Retrieve the Bell Minute from Firebase
          if (Firebase.RTDB.getString(&fbdo, pathMinute)) {
            BellMinute = fbdo.stringData();
          } else {
            Serial.println("Failed to get Bell Minute: " + fbdo.errorReason());
            continue; // Skip to the next iteration
          }

          // Retrieve the Bell Minute from Firebase
          if (Firebase.RTDB.getString(&fbdo, pathbellState)) {
            bellState = fbdo.stringData();
          } else {
            Serial.println("Failed to get Bell bellState: " + fbdo.errorReason());
            continue; // Skip to the next iteration
          }

          // Convert BellHour and BellMinute to integers
          int bellHourInt = BellHour.toInt();
          int bellMinuteInt = BellMinute.toInt();
          int bellStateint = bellState.toInt();


          // Compare current time with Bell time
          if ((currentHour == bellHourInt) && (currentMinute == bellMinuteInt)) {
            if (bellStateint == 0) {

              Serial.println("Bell ringing started at " + String(currentHour) + ":" + String(currentMinute));

              if (i == 1|| i == 6) {

                 digitalWrite(ledPin, HIGH); // Turn the LED on
                  delay(5000); // Wait 50 milliseconds
                  digitalWrite(ledPin, LOW); // Turn the LED off
                // Blink the LED 100 times quickly (50ms on, 50ms off)
                 delay(1000);
                
                  for (int j = 0; j < i; j++) {
                  digitalWrite(ledPin, HIGH); // Turn the LED on
                  delay(500); // Wait 500 milliseconds
                  digitalWrite(ledPin, LOW); // Turn the LED off
                  delay(500); // Wait 500 milliseconds
                }

              }
              else if (i == 5 || i == 9) {
                // Blink the LED 100 times quickly (50ms on, 50ms off)
              
                  digitalWrite(ledPin, HIGH); // Turn the LED on
                  delay(5000); // Wait 50 milliseconds
                  digitalWrite(ledPin, LOW); // Turn the LED off
                 
                
              }
              else {
                // Blink the LED i times with standard timing (500ms on, 500ms off)
                for (int j = 0; j < i; j++) {
                  digitalWrite(ledPin, HIGH); // Turn the LED on
                  delay(500); // Wait 500 milliseconds
                  digitalWrite(ledPin, LOW); // Turn the LED off
                  delay(500); // Wait 500 milliseconds
                }
              }


              // Update status to 1 in Firebase Realtime Database
              if (Firebase.RTDB.setInt(&fbdo, "/SchoolBell/" + String(i) + "/state", 1)) {
                Serial.println("Status updated to 1 successfully!");
              } else {
                Serial.println("Failed to update status to 1");
              }
              delay(60000); // Delay for a short period

              // Update status to 1 in Firebase Realtime Database
              if (Firebase.RTDB.setInt(&fbdo, "/SchoolBell/" + String(i) + "/state", 0)) {
                Serial.println("Status updated to 0 successfully!");
              } else {
                Serial.println("Failed to update status to 0");
              }

              bellRinging = true;
              break; // Stop checking further times once a match is found
            } else {
              Serial.println("Already done.");
            }
          } else {
            Serial.println("No Bell Scheduled");
          }


        }

        if (!bellRinging) {
          digitalWrite(ledPin, LOW); // Turn the LED off if no bell time matches
        }

      } else {
        Serial.println("Bell status is off");
        digitalWrite(ledPin, LOW); // Turn the LED off if status is not 1
      }
    } else {
      Serial.println("Failed to get Bell status from Firebase");
    }

    // Handle web server requests
    server.handleClient();
  } else {
    Serial.println("Firebase is not ready");
  }

  delay(100); // Delay for a short period
}
