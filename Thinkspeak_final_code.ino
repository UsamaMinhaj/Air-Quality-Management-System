
 
#include <SimpleTimer.h> 
#include <ESP8266WiFi.h>
//#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

float mq9 = A0; //Pin of for mq9 gas sensor
int gasSensorData = 20; 
int pm25 = D1;
int pm10 = D2;
float DustSensorValue10, DustSensorValue25;

/////////////////////////////////////////
//Dust Sensor variables
unsigned long   duration;
unsigned long   starttime;
unsigned long   endtime;
unsigned long   lowpulseoccupancy = 0;
float           ratio = 0;
unsigned long   SLEEP_TIME    = 2 * 1000;       // Sleep time between reads (in milliseconds)
unsigned long   sampletime_ms =  5*1000;  // sample time (ms)
/////////////////////////////////////////


String apiKey = "GRC6TKM1274M7X3Q";     //  Enter your Write API key from ThingSpeak

const char *ssid =  "DESKTOP-T8HCETS 9224";     // replace with your wifi ssid and wpa2 key
const char *pass =  "osama123";
const char* server = "api.thingspeak.com";

#define DHTPIN D4          // D4
float humidity;
float temperature; 

#define DHTTYPE DHT11     // DHT 11 
DHT dht(DHTPIN, DHTTYPE);

WiFiClient client;
 
void HumitureSensor()
{
   humidity = dht.readHumidity();
   temperature = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit
 
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.print("\nTemperature: ");
  Serial.print(temperature);
  Serial.print("\nHumidity: ");
  Serial.print(humidity);
 
}
 
void setup()
{
  // Debug console
  Serial.begin(9600);
  delay(10);
  dht.begin();
  Serial.println("Connecting to ");
 Serial.println(ssid);
 WiFi.begin(ssid, pass);
 
  dht.begin();
  while (WiFi.status() != WL_CONNECTED) 
     {
            delay(500);
            Serial.print(".");
     }
      Serial.println("");
      Serial.println("WiFi connected");

  pinMode(D5,OUTPUT);
  pinMode(D6,OUTPUT);
  pinMode(D7,OUTPUT);
  pinMode(D8,OUTPUT);
     
}

void Decision()
{
  if(DustSensorValue25 < 15.5 && gasSensorData < 3000 && DustSensorValue10 < 54)
  { 
   
    digitalWrite(D5,HIGH);
    digitalWrite(D6,LOW);
    digitalWrite(D7,LOW);
    digitalWrite(D8,LOW);
    }
    else if(DustSensorValue25 < 65.5 && gasSensorData < 8500 && DustSensorValue10 < 154)
  {
    digitalWrite(D5,LOW);
    digitalWrite(D6,HIGH);
    digitalWrite(D7,LOW);
    digitalWrite(D8,LOW);
    }
     else if(DustSensorValue25 < 150.5 && gasSensorData < 11000 && DustSensorValue10 < 254 )
  {
    digitalWrite(D5,LOW);
    digitalWrite(D6,LOW);
    digitalWrite(D7,HIGH);
    digitalWrite(D8,LOW);
    }
     else if(DustSensorValue25 > 150.5 || gasSensorData > 11000 || DustSensorValue10 > 254)
  {
    digitalWrite(D5,LOW);
    digitalWrite(D6,LOW);
    digitalWrite(D7,LOW);
    digitalWrite(D8,HIGH);
    }
}




void GasSensor()
{
  GasSensorCallibrator();
  Serial.print("\nConcentration: ");
  Serial.print(gasSensorData);
}


void GasSensorCallibrator()
{
   float vrl = analogRead(mq9);
   vrl = ((vrl)/1024)*5;
   Serial.print("\nVRL:");
   Serial.print(vrl);
   float ratio = ( 5 - vrl ) / vrl;
   float logppm = (log10(ratio) * -2.6) + 2.7;
   float temp = pow(10,logppm); 
  gasSensorData = temp - 7000;
  if(gasSensorData < 0)
   gasSensorData = 50*vrl*vrl;
}

/////////////////////////////////////////
// Dust Sensor Calculations
long getPM(int DUST_SENSOR_DIGITAL_PIN) {
  float duration,ratio;
  starttime = millis();
  while (1) {
    duration = pulseIn(DUST_SENSOR_DIGITAL_PIN, LOW);
    lowpulseoccupancy += duration;
    endtime = millis();
    if ((endtime-starttime) >= sampletime_ms) {
      ratio = lowpulseoccupancy / (sampletime_ms*10.0);
         long concentration = 1.1 * pow( ratio, 3) - 3.8 *pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve
      lowpulseoccupancy = 0;
      return(concentration);    
    }
  }  
}

void DustSensor10()
{
  float temp = getPM(pm10);
  DustSensorValue10 = conversion25(temp);
  Serial.print("\n Dust Concentration PM1.0: ");
  Serial.print(DustSensorValue10);
}

void DustSensor25()
{
 
  float temp = getPM(pm25);
  DustSensorValue25 = conversion25(temp);
  Serial.print("\n Dust Concentration PM2.5: ");
  Serial.print(DustSensorValue25);
}


void SensorData()
{
  DustSensor25(); 
  DustSensor10(); 
  GasSensor();
  HumitureSensor();

   if (client.connect(server,80))   //   "184.106.153.149" or api.thingspeak.com
                      {  
                            
                             String postStr = apiKey;
                             postStr +="&field1=";
                             postStr += String(temperature);
                             postStr +="&field2=";
                             postStr += String(humidity);
                             postStr +="&field3=";
                             postStr += String(DustSensorValue10);
                             postStr +="&field4=";
                             postStr += String(DustSensorValue25);
                             postStr +="&field5=";
                             postStr += String(gasSensorData);
                             postStr += "\r\n\r\n";
 
                             client.print("POST /update HTTP/1.1\n");
                             client.print("Host: api.thingspeak.com\n");
                             client.print("Connection: close\n");
                             client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
                             client.print("Content-Type: application/x-www-form-urlencoded\n");
                             client.print("Content-Length: ");
                             client.print(postStr.length());
                             client.print("\n\n");
                             client.print(postStr);

                             Serial.println("%. Send to Thingspeak.");
                        }
          client.stop();
 
          Serial.println("Waiting...");
  
}

float conversion25(long concentrationPM25) {
  double pi = 3.14159;
  double density = 1.65 * pow (10, 12);
  double r25 = 0.44 * pow (10, -6);
  double vol25 = (4/3) * pi * pow (r25, 3);
  double mass25 = density * vol25;
  double K = 3531.5;
  return (concentrationPM25) * K * mass25;
}
 
void loop()
{
  Decision();
  SensorData();
  //timer.run();
  delay(10000);
}
