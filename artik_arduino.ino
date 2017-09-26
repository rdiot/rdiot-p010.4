/* Samsung ARTIK 5 ARTIK Cloud using the Arduino IDE (REST) [P010.3] : http://rdiot.tistory.com/20 [RDIoT Demo] */

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <DebugSerial.h>
 
LiquidCrystal_I2C lcd(0x27,20,4);  // LCD2004
int led = 13; // LED

// MQTT params  
char mqttServer[]     = "192.168.0.9";
int  mqttPort         = 1883;
char mqttClientName[] = "ARTIK-Arduino"; // or whatever you prefer
float insTemp, desTemp, outTemp;

WiFiClient ipStack;
MQTTClient mqttClient;

// Web REST API params
char server[] = "api.artik.cloud";  
int httpsPort = 443; //(port 443 is default for HTTPS)

String AuthorizationData = "Authorization: Bearer 3e7d63254fc64d6c945fc075cf0e4804";

//String PostData = "{\n  \"sdid\": \"_your_DEVICE_ID_goes_here_\",\n  \"type\":   \"message\",\n  \"data\": \{\"insTemp\":19.3,  \"desTemp\":19.1, \"outTemp\":23.7 \}}";  
char buf[200];

WiFiSSLClient client;
 
void setup()
{

  DebugSerial.begin(9600);  
  mqttClient.begin(mqttServer, mqttPort, ipStack);
  mqttClient.connect(mqttClientName);
  delay(500);
  mqttClient.subscribe("#");
  
  lcd.init();  // initialize the lcd 
  lcd.backlight();
  lcd.print("start LCD2004");
  pinMode(13, OUTPUT);

  delay(10000);
 
  lcd.clear();
}
 
void loop()
{
  
  mqttClient.loop();
  
  lcd.setCursor(0,0);
  lcd.print("Photon->MQTT->Cloud");
  
  //lcd.setCursor(0,1);
  //lcd.print("CDS=" + (String)temperatureC + "");

  delay(3000);
  
}

int loadBuffer(float insTemp, bool blndOpen, bool openBlndCtrl) {  
   StaticJsonBuffer<200> jsonBuffer; // reserve spot in memory

   JsonObject& root = jsonBuffer.createObject(); // create root objects
     root["sdid"] = "e9ab73324f9b460ea100268be9e79953"; // FIX 
     root["type"] = "message";

   JsonObject& dataPair = root.createNestedObject("data"); // create nested objects
   dataPair["insTemp"] = insTemp;  
   dataPair["desTemp"] = desTemp;  
   dataPair["outTemp"] = outTemp;  
  
   root.printTo(buf, sizeof(buf)); // JSON-print to buffer

   return (root.measureLength()); // also return length
 }

 void messageReceived(String topic, String payload, char * bytes, unsigned int length) {
  DebugSerial.println("--------------------------------");
  DebugSerial.print("Message arrived [");
  DebugSerial.print(topic);
  DebugSerial.println("] ");

  lcd.setCursor(0,1);
  lcd.print("topic=" + (String)topic + "");
  
  if (topic=="brightness") { // from Photon
    // convert payload to float
    String s = payload;
    float temp = s.toFloat();
    
  lcd.setCursor(0,2);
  lcd.print("cds=" + (String)temp + " ");    
  /*
    if (temp < 3000) {
      if (temp > 2000) { //expected range for photoresistor
        outTemp = temp;
      }  
    }
  */    
      outTemp = temp;
      DebugSerial.print("outTemp(Photon-CDS):");
      DebugSerial.println(outTemp);

    if(outTemp > 2000) 
    {
      digitalWrite(13, HIGH);
    }
    else 
      digitalWrite(13, LOW);

  } 
  if (topic=="color") { // from Node-RED slider
      String c = payload;
      DebugSerial.print("desTemp(Node-RED):");
      DebugSerial.println(c);
  }      
     
  int insTemp = analogRead(0);    
  float mVoltage = insTemp * 2 * 0.439453125; //ARTIK Voltage
  float temperatureC = (mVoltage - 500) / 10.0;  

  DebugSerial.print("insTemp(ARTIK-TMP36):");
  DebugSerial.println(temperatureC);
    
  // load current values into the buffer
  int bufLength = loadBuffer(insTemp,desTemp,outTemp);
  
  // send it out to ARTIK Cloud
  client.connect(server, httpsPort);
  delay(100);
  if (!client.connected()) { 
     DebugSerial.println("No connection to ARTIK Cloud!");
     ; // service the error 
   } 
   else {
     client.println("POST /v1.1/messages HTTP/1.1");
     client.println("Host: api.artik.cloud");
     client.println("Accept: */*");
     client.println("Content-Type: application/json");
     client.println(AuthorizationData);
     client.print("Content-Length: ");
    
     client.println(bufLength);
     client.println();
     client.println(buf);

     DebugSerial.println("Sent Data to ARTIK Cloud!");
     DebugSerial.println(buf);

   }
} 
