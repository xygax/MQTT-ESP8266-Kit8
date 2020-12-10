#include <dummy.h>

/*
 * Heltec kit-8 (ESP8266) with intergrated display 
 * connection to DHCP and display.
 * connection to MQTT service
 * Connection to DHT22 display Humidity and temprature
 * Publish to MQTT topic
 * User is prefix + mac address
 * output is an identyand readings as CSV [IIIIIII,TTTTT,HHHHH]
 * By Steve dec 20
*/


// Dependancies
#include "heltec.h"
#include "ESP8266WiFi.h"
#include "PubSubClient.h"
#include "DHT.h"


// Hardware
#define DHTTYPE DHT22   // sensor type
#define DHTPIN 14       // sensor pin


// Constants 
#define wifi_ssid "**********"              //SSID
#define wifi_password "**********"         //PWD

#define mqtt_server "node02.myqtthub.com" // server location
#define mqtt_user "**********"        // identiy
#define mqtt_password "**********"      // password

#define prefix "T-"                       // appended to mac address as mqtt identity
#define Identiy "IOT-001"                 // UID
#define input_topic "*****.Temp"          // inbound topic
#define output_topic "*****.Temp"         // outbound topic


WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE, 22);


void setup(){

  Heltec.display->init();                    // seperate init doent do msg
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_16); // big text to start
  
  Serial.begin(115200);
  Serial.flush();
  delay(50);
  
  dht.begin();
  
  setup_wifi();
  client.setServer(mqtt_server,1883);
  client.setCallback(callback);
}

void setup_wifi(){
  delay(100);
  Heltec.display->clear();
  WiFi.disconnect(true); // Set WiFi to station mode and disconnect from an AP if it was previously connected
  delay(100);
  
  Serial.println();
  Serial.print ("Connecting to ");
  Serial.println (wifi_ssid);
  
	WiFi.begin(wifi_ssid,wifi_password);
  delay(500);

   byte count = 0;
	while(WiFi.status() != WL_CONNECTED && count < 10){
		count ++;
    Heltec.display->invertDisplay();
		Heltec.display->drawString(20, 6, "Connecting...");
		Heltec.display->display();
    delay(500);
	}
 
  String mac=WiFi.macAddress();
 
	if(WiFi.status() == WL_CONNECTED){
    Serial.println("WiFi: " + WiFi.SSID ());
    Serial.println("IP:   " + WiFi.localIP ().toString());
    Serial.println("MAC:  " + mac);
    Serial.println(" ");
      
    Heltec.display->clear();
		Heltec.display->drawString(0, 0, "WIFI    " + WiFi.SSID ());
    Heltec.display->setFont(ArialMT_Plain_10);
		Heltec.display->drawString(0, 21, "DHCP      " + WiFi.localIP ().toString());
		Heltec.display->display();
		delay(5000);
		Heltec.display->clear();
    Heltec.display->display();
    
	}else{
    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_10);
		Heltec.display->drawString(0, 20, "Failed");
		Heltec.display->display();
		delay(1000);
		
	}
}



void callback(char* topic, byte* payload, unsigned int length){

  //char msg(50);
  
  Serial.print("Message [ ");
  Serial.print(topic);
  Serial.println (" ]");
  
  String msg;
  for (int i = 0; i <length; i++){
    Serial.print((char)payload[i]);
    msg += (char)payload[i]; 
  }
  Serial.println(" ");
  Serial.println(" ");
  
  Heltec.display->drawString(0,22, "                        ");
  Heltec.display->drawString(0, 22,"Msg:- " + msg);
  Heltec.display->display();
  
}

void reconnect(){

String mqtt_clientID;       // determines faull name from mac
  mqtt_clientID = prefix;    // Identity prefix
  mqtt_clientID += WiFi.macAddress();

  while (!client.connected()){
    Serial.print("MQTT Broker: ");
    Serial.println (mqtt_server);
        
    Heltec.display->normalDisplay();
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, "Bkr:-");
    Heltec.display->drawString(24, 0, mqtt_server);
    Heltec.display->display();
    
    if (client.connect (mqtt_clientID.c_str(), mqtt_user, mqtt_password)){
      delay(20);
      Serial.print (mqtt_user);
      Serial.println(" connected");
      Serial.println(" ");
      
      Heltec.display->drawString(0, 11, "Usr:-");
      Heltec.display->drawString(24, 11, mqtt_user);
      Heltec.display->drawString(0, 22, "Uid:-");
      Heltec.display->drawString(24, 22, mqtt_clientID);
      Heltec.display->display();
      delay(5000);
      Heltec.display->clear();
      client.subscribe(input_topic);
      
      // Wait 2 sec before retrying
      delay(2000);
    }else{
      Serial.print("failed,rc=");
      Serial.println(client.state());
      Serial.println(" try again in 10 seconds");
      
      // Wait 10 seconds before retrying
      delay(10000);
    }
  }
}

bool checkBound(float newValue, float prevValue, float maxDiff){
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

long lastMsg =0;
float temp = 0.0;
float hum = 0.0;
float diff = 0.2;


void loop(){
  if (!client.connected()){
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg >300000){   
    lastMsg = now;            // 300000 = 5 mins interval

    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();

    if (checkBound(newTemp, temp, diff)){
      temp = newTemp;
      Serial.print("New temprature: ");
      Serial.print(String(temp).c_str());
      Serial.println (" C");
      //client.publish("Dauis.Temp", String(temp).c_str(), true);

      //if (checkBound(newHum, hum, diff)){   // used to send on change
      hum = newHum;
      Serial.print("New Humidity: ");
      Serial.print(String(hum).c_str());
      Serial.println (" %");
      //client.publish("Dauis.Temp", String(hum).c_str(), true);

      Heltec.display->clear();      
      Heltec.display->drawString(0, 0, "Tmp:- " + String(temp) + " C");
      Heltec.display->drawString(88, 0, String(Identiy));
      Heltec.display->drawString(0, 11, "Hum:- " + String (hum) +" %");
      Heltec.display->display();

      // build output string
      String payload = Identiy; // build payload as csv
      payload = String(payload + "," + String(temp).c_str());
      payload = String(payload + "," + String(hum).c_str());
      
      Serial.println (payload);
      Serial.println (" ");
      
      client.publish(output_topic, String(payload).c_str(), true);
      

      
    }
    
    //delay(600000);  //wait 10 mins
  }
}
