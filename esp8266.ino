/**The MIT License (MIT)


  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*/

#include <ESP8266WiFi.h>
#include "DHT.h"
#include "settings.h"  


/***************************
   Begin Settings
 **************************/

 // Edit settings.h for appropriate WiFi-settings, Thingspeak API, Domoticz hostname/Sensor-ID and Weather Underground ID/password.

/***************************
   End Settings
 **************************/

// Initialize the temperature/ humidity sensor

// DHT Settings
#define DHTPIN D6         // what digital pin we're connected to. If you are not using NodeMCU change D6 to real pin
#define DHTTYPE DHT22     // DHT 22  (AM2302), AM2321

//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

const boolean IS_METRIC = true;

// Update every 600 seconds = 10 minutes. Change your interval here
const int UPDATE_INTERVAL_SECONDS = 300;

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("--------");
}


void loop() {
  // read values from the sensor
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(!IS_METRIC);
  Serial.print("DHT22 Temperature:");
  Serial.println(temperature);
  Serial.print("DHT22 Humidity:");
  Serial.println(humidity);

  // calculate temperature in Fahrenheit
  float temperaturef = (temperature * 1.8) + 32;

  // rough estimate on dewpoint calculation
  float dewpoint = temperature - ((100 - humidity) / 5);
  float dewpointf = (dewpoint * 1.8) + 32;


  // Use WiFiClient class to create TCP connections to Thingspeak

  WiFiClient client;

  if (client.connect(host_ts, 80) > 0)  {
    String urlts = "/update?api_key=";
    urlts += THINGSPEAK_API_KEY;
    urlts += "&field1=";
    urlts += String(temperature);
    urlts += "&field2=";
    urlts += String(humidity);

    // This will send the request to the server
    client.print(String("GET ") + urlts + " HTTP/1.1\r\n" +
                 "Host: " + host_ts + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(100);
    client.stop();
    Serial.println("-----------");
    Serial.println(urlts + " sent to " + host_ts);
  }



  // Connect to Domoticz and drop temperature + humidity to it

  if (client.connect(domoticz, port) > 0)  {
    String urldom = "/json.htm?type=command&param=udevice&idx=" ;
    urldom += idx;
    urldom += "&nvalue=0&svalue=";
    urldom += temperature;
    urldom += ";";
    urldom += humidity;
    urldom += ";0";

    // This will send the request to the server
    client.print(String("GET ") + urldom + " HTTP/1.1\r\n" +
                 "Host: " + domoticz + ":" + port + "\r\n" +
                 "User-Agent: ESP8266\r\n"
                 "Connection: close\r\n\r\n");
    delay(100);

    client.stop();
    Serial.println("-----------");
    Serial.println(urldom + " sent to " + domoticz);
  }


  // Connect to WU and drop temp + humidity to it (temperature must be in Fahrenheit)
  // see: http://help.wunderground.com/knowledgebase/articles/865575-pws-upload-protocol


  if (client.connect(hostwu, 80) > 0)   {
    String url2 = "/weatherstation/updateweatherstation.php?ID=";
    url2 += WUID;
    url2 += "&PASSWORD=";
    url2 += WUPASS;
    url2 += "&dateutc=now";
    url2 += "&tempf=";
    url2 += temperaturef;
    url2 += "&humidity=";
    url2 += humidity;
    url2 += "&dewptf=";
    url2 += dewpointf;
    url2 += "&softwaretype=ESP8266&action=updateraw";

    // This will send the request to the server
    client.print(String("GET ") + url2 + " HTTP/1.1\r\n" +
                 "Host: " + hostwu + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(100);

    client.stop();
    Serial.println("-----------");
    Serial.println(url2 + " sent to " + hostwu);
  }

  // Go to deep-sleep. To enable this see https://www.losant.com/blog/making-the-esp8266-low-powered-with-deep-sleep
  // For NodeMCU connect D0 (GPIO16) to RST

  Serial.print("ESP8266 in sleep mode for ");
  Serial.print(UPDATE_INTERVAL_SECONDS);
  Serial.println(" seconds");

  ESP.deepSleep(UPDATE_INTERVAL_SECONDS * 1000000);


}
