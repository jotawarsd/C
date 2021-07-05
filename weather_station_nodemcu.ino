#include <DHT.h>  // Including library for dht
#include <complex.h>
#include <ESP8266WiFi.h>
 
String apiKey = "";     //  Enter your Write API key from ThingSpeak
 
const char *ssid =  "";     // replace with your wifi ssid and wpa2 key
const char *pass =  "";     // replace with wifi password
const char* server = "api.thingspeak.com";
 
#define DHTPIN 0          //pin where the dht11 is connected
 
DHT dht(DHTPIN, DHT11);

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

int sensorPin = A0;    // input for LDR and rain sensor
int enable1 = 15;      // enable reading LDR
int enable2 = 13;      // enable reading Rain sensor


int sensorValue1 = 0;  // variable to store the value coming from sensor LDR
int sensorValue2 = 0;  // variable to store the value coming from sensor Rain sensor

 
WiFiClient client;
 
void setup() 
{
       pinMode(enable1, OUTPUT);
       pinMode(enable2, OUTPUT);
       
       Serial.begin(115200);
       delay(10);
       dht.begin();
 
       Serial.println("Connecting to ");
       Serial.println(ssid);
 
 
       WiFi.begin(ssid, pass);
 
      while (WiFi.status() != WL_CONNECTED) 
     {
            delay(500);
            Serial.print(".");
     }
      Serial.println("");
      Serial.println("WiFi connected");
 
}
 
void loop() 
{

//----------DHT11----------
      float h = dht.readHumidity();
      float t = dht.readTemperature();

      double gamma = log(h/100) + ((17.62*t) / (243.5+t));
      double dp = 243.5*gamma / (17.62-gamma);

      Serial.print("Dew point:        ");
      Serial.print(dp);
      Serial.print("degrees celcius");
      Serial.println();
      
      if (isnan(h) || isnan(t)) 
      {
          Serial.println("Failed to read from DHT sensor!");
          return;
      }
      Serial.print("Temperature:      ");
      Serial.print(t);
      Serial.println(" degrees Celcius");
      Serial.print("Humidity:         ");
      Serial.println(h);

//----------BMP180----------

      if(!bmp.begin()) 
      {
          Serial.print("Failed to read from BMP sensor!!");
          while(1);
      }

      sensors_event_t event;
      bmp.getEvent(&event);

      Serial.print("Pressure:         ");
      Serial.print(event.pressure);
      Serial.println(" hPa");

      float temperature;
      bmp.getTemperature(&temperature);
      Serial.print("Temperature:      ");
      Serial.print(temperature);
      Serial.println(" degrees Celcius ");

      //--- extra----you can measure the altitude with the temperature and the air pressure

      float seaLevelPressure = 1015;
      Serial.print("Altitude:         "); 
      Serial.print(bmp.pressureToAltitude(seaLevelPressure,event.pressure)); 
      Serial.println(" m");

//------------ldr--------------

      digitalWrite(enable1, HIGH); 
      sensorValue1 = analogRead(sensorPin);
      sensorValue1 = constrain(sensorValue1, 300, 850); 
      sensorValue1 = map(sensorValue1, 300, 850, 0, 1023); 
      Serial.print("Light intensity:  ");
      Serial.println(sensorValue1);
      digitalWrite(enable1, LOW);
      delay(100);

      //-----lux-----

      float ADC = sensorValue1; 

      float Vout = (ADC * 0.0048828125);// Vout = Output voltage from potential Divider. 
                                        //[Vout = ADC * (Vin / 1024)]

      float RLDR = (10.0 * (5 - Vout))/Vout; // Equation to calculate Resistance of LDR, 
      //[R-LDR =(R1 (Vin - Vout))/ Vout] R1 = 10,000 Ohms , Vin = 5.0 Vdc 

      float Lux = (500 / RLDR); 

      Serial.print("Lux:              ");
      Serial.println(Lux);

//----------Rain sensor----------

      digitalWrite(enable2, HIGH); 

      delay(500);
      sensorValue2 = analogRead(sensorPin);
      sensorValue2 = constrain(sensorValue2, 150, 440); 
      sensorValue2 = map(sensorValue2, 150, 440, 1023, 0); 
      
      Serial.print("Rain value:       ");
      Serial.println((sensorValue2/10));
      delay(100);
      
      digitalWrite(enable2, LOW);

//----------thingspeak-----------

      
      
if (client.connect(server,80))   //   "184.106.153.149" or api.thingspeak.com
{                           
        String postStr = apiKey;
        postStr +="&field1=";
        postStr += String(t);
        postStr +="&field2=";
        postStr += String(h);
        postStr +="&field3=";
        postStr += String(dp);
        postStr +="&field4=";
        postStr += String(event.pressure);
        postStr +="&field5=";
        postStr += String(temperature);
        postStr +="&field6=";
        postStr += String(bmp.pressureToAltitude(seaLevelPressure,event.pressure));
        postStr +="&field7=";
        postStr += String(Lux);
        postStr +="&field8=";
        postStr += String((sensorValue2/10));
        postStr += "\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n";

//--------------------------------
 
        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(postStr.length());
        client.print("\n\n\n\n\n\n\n\n\n");
        client.print(postStr);
        
        Serial.println("%. Send to Thingspeak.");
                        
}
                        
client.stop();
Serial.println("Waiting...");
  
// thingspeak needs minimum 15 sec delay between updates
delay(1500);
}
