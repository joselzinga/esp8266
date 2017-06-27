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
#include <U8x8lib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "settings.h"

/***************************
  Edit settings.h for appropriate WiFi-settings, update interval, Thingspeak API,
  Domoticz hostname/Sensor-ID and Weather Underground ID/password.
**************************/

// We define the type of screen here
// I use: https://www.tinytronics.nl/shop/nl/display/0.96-inch-oled-display-128*64-pixels-blauw-i2c?search=I2C0.96OLEDBLUE
// With SCL to D2, SCA to D1 (software I2C protocol)
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(D2, D1, U8X8_PIN_NONE);

// Initialize the temperature/ humidity sensor (DHT22)

// DHT Settings
#define DHTPIN D6         // what digital pin the DHT22 is connected to.
#define DHTTYPE DHT22     // DHT 22  (AM2302), AM2321

const boolean IS_METRIC = true;
DHT dht(DHTPIN, DHTTYPE);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "0.nl.pool.ntp.org", 7200);

void setup() {
  // Enabling serialport (115.200 baud-rate)
  Serial.begin(115200);
  delay(10);

  // Enabling display - set font to something small
  u8x8.begin();
  u8x8.setPowerSave(0);
  u8x8.setFont(u8x8_font_chroma48medium8_r);

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
  u8x8.setCursor(0, 6);
  u8x8.print("SSID:");
  u8x8.print(ssid);
  u8x8.setCursor(0, 7);
  u8x8.print(WiFi.localIP());

}


void loop() {
  // read values from the sensor
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(!IS_METRIC);
  Serial.print("DHT22 Temperature:");
  Serial.println(temperature);
  Serial.print("DHT22 Humidity:");
  Serial.println(humidity);
  u8x8.setCursor(0, 0);
  u8x8.print("Temp:");
  u8x8.print(temperature);
  u8x8.setCursor(0, 1);
  u8x8.print("Hum.:");
  u8x8.print(humidity);


  // calculate temperature in Fahrenheit
  float temperaturef = (temperature * 1.8) + 32;

  // rough estimate on dewpoint calculation
  float dewpoint = temperature - ((100 - humidity) / 5);
  u8x8.setCursor(0, 2);
  u8x8.print("Dewp:");
  u8x8.print(dewpoint);
  float dewpointf = (dewpoint * 1.8) + 32;

  // Use WiFiClient class to create TCP connections to various hosts
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



  timeClient.begin();
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
  u8x8.setCursor(0, 5);
  u8x8.print("Time:");
  u8x8.print(timeClient.getFormattedTime());

  // Go to deep-sleep. To enable this see https://www.losant.com/blog/making-the-esp8266-low-powered-with-deep-sleep
  // For NodeMCU connect D0 (GPIO16) to RST

  Serial.print("ESP8266 in sleep mode for ");
  Serial.print(UPDATE_INTERVAL_SECONDS);
  Serial.println(" seconds");
  ESP.deepSleep(UPDATE_INTERVAL_SECONDS * 1000000);


}
