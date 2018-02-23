#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define CNY70 A0
//------------------------------------------
// Domoticz
 const char * domoticz_server = "###.###.###.###"; //Domoticz server IP
 int port = 8080; //Domoticz port
 int kWh_idx = 6; //IDX for this virtual sensor, found in Setup -> Devices
 int impulse_count = 0;
 float kWh_value = 0.3*1000; //Domoticz gets Wh as unit so every impulse is 300Wh (line detected 36 times so 36*(1/120)kWh)
//------------------------------------------
// WIFI
WiFiClient client;
const char* ssid = "";
const char* password = "";
//------------------------------------------
// Other
bool impulse_finished = true;  //Is impulse finished
int impulse_threshold = 0; //impulse is finished after we have more than x "not detected" measurements. Set in loop if statement.
//------------------------------------------
void sendDomoticzkWh(float kWh_value)
{
    // Domoticz format /json.htm?type=command&param=udevice&idx=IDX&svalue=COUNTER

    if (client.connect(domoticz_server,port)) {
        
        client.print("GET /json.htm?type=command&param=udevice&idx=");
        client.print(kWh_idx);
        client.print("&svalue=");
        client.print(kWh_value);
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.print(domoticz_server);
        client.print(":");
        client.println(port);
        client.println("User-Agent: Arduino-ethernet");
        client.println("Connection: close");
        client.println();      
        client.stop();
     }
}
//------------------------------------------
void setup() 
{
  //Setup Serial port speed
  Serial.begin(115200);
  pinMode(CNY70, INPUT);
  Serial.println("Booting temperature meter by vvnuczek");

  //Setup WIFI
  WiFi.begin(ssid, password);
  Serial.println("");

  //Wait for WIFI connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
//------------------------------------------
void loop() {
  
  //Getting CNY70 value
  int CNY70_value = analogRead(CNY70);
  Serial.print("CNY70: ");
  Serial.println(CNY70_value);
  
  //Black line detected
  if(CNY70_value <= 50 && impulse_threshold == 0 && impulse_finished == true)
  {
    //Wait for next measurements to tell if impulse is finished (black line passed the sensor). 
    //Solves possible errors if the black line was to stop in front of the sensor when there in no electricity usage
    impulse_finished = false; 
    
    //Set threshold of negative measurements - line not detected at least x times
    //Solves possible errors if we get wrong measurement
    impulse_threshold = 20; 
    
    //How many line detections happened since sending data. 
    //Sending data after 36 detections - several minutes time difference. 
    //Sending data everytime gave poor accuracy - possibly due to network packet loss.
    if(impulse_count>=35){
        Serial.print( "Sending energy consumption. " );
        sendDomoticzkWh(kWh_value);
        impulse_count=0; //Reset impulse counter 
    }
    else{
      impulse_count++; //Increment impulse counter
    }
  }
  //Black line not detected
  else 
  {
    //Count measurements with line not detected and decrement impulse threshold
    if(CNY70_value >= 80 && impulse_finished == false && impulse_threshold != 0){
      impulse_threshold--;  
    }
    //Impulse is finished. We got 20 negative measurements.
    else if(impulse_finished == false && impulse_threshold == 0){
      impulse_finished = true;
    }
  }
  delay(50); //Wait 50ms before next measurement
}