#include <WiFiClientSecure.h>
#include <MD_MAX72xx.h>
#include <ArduinoJson.h>
#include <Time.h>


#include <MD_Parola.h>
#include <MD_MAX72xx.h>


const char* ssid     = "ssid";
const char* password = "password";

const char* host = "opendata.lillemetropole.fr";
unsigned long delayBetweenChecks = 60000; //mean time between api requests
unsigned long whenDueToCheck = 0;


const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;



// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define CLK_PIN   25
#define DATA_PIN  27
#define CS_PIN    26

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

uint8_t scrollSpeed = 25;    // default frame delay value
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 1000; // in ms
char* pc[6];
uint8_t  curText = 0;
String line;

// opendata.lillemetropole.fr root certificate
const char* root_ca= \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n" \
"MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n" \
"DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\n" \
"PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\n" \
"Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n" \
"AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\n" \
"rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\n" \
"OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\n" \
"xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\n" \
"7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\n" \
"aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n" \
"HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\n" \
"SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\n" \
"ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\n" \
"AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\n" \
"R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\n" \
"JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\n" \
"Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n" \
"-----END CERTIFICATE-----\n";



// Time Functions

time_t getTime(){
  struct tm tm;
  if(!getLocalTime(&tm)){
    Serial.println("Failed to obtain time");
    return 0;
    
  }
  return mktime(&tm);
}

time_t str_to_time_t(const char* s_time){
  struct tm tm;
  strptime(s_time, "%Y-%m-%dT%T+00:00", &tm);
  time_t t = mktime(&tm);
  return t;
}

int minutes_diff(time_t expected_time){
  int minutes = (expected_time - getTime()) / 60;
  return minutes;
}

// ----

void getResponse()
{
    Serial.print("connecting to ");
    Serial.println(host);

    // Use WiFiClient class to create TCP connections
    WiFiClientSecure client;
    const int httpPort = 443;
    if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
    }
    client.setCACert(root_ca);

    String url = "/api/records/1.0/search/";
    url += "?dataset=ilevia-prochainspassages";
    url += "&q=(identifiantstation+%3D+TRANSPOLE%3AStopPointRef%3ABP%3A2677%3ALOC)";
    url += "+OR+";
    url += "(identifiantstation+%3D+TRANSPOLE%3AStopPointRef%3ABP%3A794%3ALOC)";
    url += "&rows=5";
    url += "&sort=-heureestimeedepart";
    
    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }
    
    // Read all the lines of the reply
    while(client.available()) {
        client.readStringUntil('\r');
        line = client.readStringUntil('\r'); // json in one line so this works
    }
    Serial.println("closing connection");
}



// I know nothing about pointers, will need to fix this
void update_timetable_array(const char* json){
  DynamicJsonDocument doc(10000);
  deserializeJson(doc, json);
  JsonArray records = doc["records"];
  pc[0] = "Prochains bus:";
  bool t = true;
  for(int i=0; i < records.size(); i++){
      char timetable[100];
      char *number = strdup(records[i]["fields"]["codeligne"]);
      int time = minutes_diff(str_to_time_t(records[i]["fields"]["heureestimeedepart"]));
      sprintf (timetable, "%s: %dmn", number, time);
      char *bar = strdup(timetable);
      pc[i+1] =  bar;      
      Serial.println(bar);
  }
}


void setup()
{
    Serial.begin(115200);
    delay(10);

    P.begin();  
    P.setIntensity(1);
    P.displayText("Prochains bus...", scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
   

    // Connect to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    }

void loop()
{
  unsigned long timeNow = millis();
  if ((timeNow > whenDueToCheck))
  {
    getResponse();
    update_timetable_array(&line[0]);
    whenDueToCheck = timeNow + delayBetweenChecks;
  }
  if (P.displayAnimate()) // If finished displaying message
  {
    //Set the display for the next string.
    curText = (curText + 1) % ARRAY_SIZE(pc);
    P.setTextBuffer(pc[curText]);
    
    P.displayReset();  // Reset and display it again
  }
}
