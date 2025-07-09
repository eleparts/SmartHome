/*
  ArduinoMqttClient - WiFi Simple Receive Callback

  This example connects to a MQTT broker and subscribes to a single topic.
  When a message is received it prints the message to the Serial Monitor,
  it uses the callback functionality of the library.

  The circuit:
  - Arduino MKR 1000, MKR 1010 or Uno WiFi Rev2 board

  This example code is in the public domain.
*/

#define EN1 4
#define EN2 5

#include <DHT11.h>                  // 온습도 센서 DHT11 라이브러리 

DHT11 dht11(6);

int tem, hum; 

#include <ArduinoMqttClient.h>
#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_AVR_UNO_WIFI_REV2)
  #include <WiFiNINA.h>
#elif defined(ARDUINO_SAMD_MKR1000)
  #include <WiFi101.h>
#elif defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_NICLA_VISION) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_GIGA) || defined(ARDUINO_OPTA)
  #include <WiFi.h>
#elif defined(ARDUINO_PORTENTA_C33)
  #include <WiFiC3.h>
#elif defined(ARDUINO_UNOR4_WIFI)
  #include <WiFiS3.h>
#endif

// arduino 보드와 HomeAssistant를 통신할 때 사용할 와이파이를 세팅합니다.
#include "arduino_secrets.h"  
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;    // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

// To connect with SSL/TLS:
// 1) Change WiFiClient to WiFiSSLClient.
// 2) Change port value from 1883 to 8883.
// 3) Change broker value to a server with a known SSL/TLS root certificate 
//    flashed in the WiFi module.

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "192.168.51.114";
int        port     = 1883;
const char topic[]  = "ArduinoR4";

int fan_timer_state = 0, fan_century_state, fan_century = 0;
int target_hhour = 0, target_hminute = 0, target_hsecond = 0;
int humi_hour_timer = 0, humi_minute_timer = 0, EN = 0, humi_EN = 0; 
int humi_timer, humi_humi;
int humi_min = 0, humi_max = 0;
boolean TIMER_HUMI = LOW, humi_flag = LOW, PIR_10m = LOW;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  pinMode(EN1, OUTPUT);   // 사용하는 두 제품의 입출력 모드를 설정합니다.
  pinMode(EN2, OUTPUT);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // attempt to connect to WiFi network:
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }

  Serial.println("You're connected to the network");
  Serial.println();

  // You can provide a unique client ID, if not set the library uses Arduino-millis()
  // Each client must have a unique client ID
  // mqttClient.setId("clientId");

  // You can provide a username and password for authentication
  mqttClient.setUsernamePassword("smarthome", "smarthome");
  //mqttClient.setUsernamePassword("eleparts", "eleparts");

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

//  if (!mqttClient.connect(broker, port)) {
//    Serial.print("MQTT connection failed! Error code = ");
//    Serial.println(mqttClient.connectError());
//
//    while (1);
//  }

  while (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    delay(100);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  // set the message receive callback
  mqttClient.onMessage(onMqttMessage);

  Serial.print("Subscribing to topic: ");
  Serial.println(topic);
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe(topic);

  // topics can be unsubscribed using:
  // mqttClient.unsubscribe(topic);

  Serial.print("Waiting for messages on topic: ");
  Serial.println(topic);
  Serial.println();
}

void loop() {
  // PIR센서 입력 값을 boolean형식(참/거짓)의 detect 변수에 저장
  int result = dht11.readTemperatureHumidity(tem, hum);

  Serial.print("humi_flag : ");
  Serial.println(humi_flag);
  Serial.print("TIMER/HUMI : ");
  Serial.println(TIMER_HUMI);

  Serial.print("EN : ");
  Serial.println(EN);
    
  if(humi_timer >= 60){
    humi_hour_timer = humi_timer / 60;
    humi_minute_timer = humi_timer % 60;
  }else{
    humi_hour_timer = 0;
    humi_minute_timer = humi_timer;
  }

  if(humi_timer != 0 || TIMER_HUMI == 1){
    humi_flag = 1;
  }else if(humi_timer == 0){
    humi_flag = 0;
  }
  
  Serial.print("humi_hour_timer : ");
  Serial.println(humi_hour_timer);
  Serial.print("humi_minute_timer : ");
  Serial.println(humi_minute_timer);

  target_hhour = humi_hour_timer;
  target_hminute = humi_minute_timer;

  humi_min = humi_humi / 1000;
  humi_max = humi_humi % 1000;

  Serial.print("humi_min : ");
  Serial.println(humi_min);
  Serial.print("humi_max : ");
  Serial.println(humi_max);

  if(humi_flag == 1){
    if(TIMER_HUMI == LOW){
      if(EN == 1){
        digitalWrite(EN1, LOW);
        digitalWrite(EN2, HIGH);
      }else if(EN == 2){
        digitalWrite(EN1, HIGH);
        digitalWrite(EN2, LOW);
      }else if(EN == 3){
        digitalWrite(EN1, LOW);
        digitalWrite(EN2, LOW);
      }
    }else if(TIMER_HUMI == HIGH){
      Serial.println("여기는 동작 됨?");
      if(hum < humi_min){
        digitalWrite(EN1, LOW);
        digitalWrite(EN2, LOW);
        humi_EN = 1;
      }else if(hum >= humi_min && hum < humi_max){
        digitalWrite(EN1, LOW);
        digitalWrite(EN2, HIGH);
        humi_EN = 2;
      }else if(hum >= humi_max){
        digitalWrite(EN1, HIGH);
        digitalWrite(EN2, HIGH);
        humi_EN = 3;
      }
    }
  }else if(humi_flag == 0){
    digitalWrite(EN1, HIGH);
    digitalWrite(EN2, HIGH);
  }
  
  // call poll() regularly to allow the library to receive MQTT messages and
  // send MQTT keep alives which avoids being disconnected by the broker
  mqttClient.poll();

  mqttClient.beginMessage("Arduinohumidifier");
  if(TIMER_HUMI == false){
    mqttClient.print(target_hhour);
    mqttClient.print("시간 ");
    mqttClient.print(target_hminute);
    mqttClient.println("분 타이머");
    if(EN == 0){
      mqttClient.print("가습기 동작 미설정");
    }else if(EN == 1){
      mqttClient.print("1번 가습기 동작");
    }else if(EN == 2){
      mqttClient.print("2번 가습기 동작");
    }else if(EN == 3){
      mqttClient.print("가습기 모두 동작");
    }
  }else if(TIMER_HUMI == true){
    mqttClient.print("현재 습도값 : ");
    mqttClient.print(hum);
    mqttClient.print("% / 습도 최솟값 : ");
    mqttClient.print(humi_min);
    mqttClient.print("% / 습도 최댓값 : ");
    mqttClient.print(humi_max);
    mqttClient.println("%");
    
    if(humi_EN == 1){
      mqttClient.print("가습기 모두 동작");
    }else if(humi_EN == 2){
      mqttClient.print("1번 가습기 동작");
    }else if(humi_EN == 3){
      mqttClient.print("가습기 모두 중지");
    }
  }
  mqttClient.endMessage();

  Serial.println();
}

void onMqttMessage(int messageSize) {                         // HomeAssistant에서 메시지를 송신 받을 때,
  // we received a message, print out the topic and contents
  Serial.println("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  //String mqttpayload = (String)mqttClient.read();
  String mqttpayload = mqttClient.readString();               // mqttClient.readString() 이라는 함수를 써 
                                                              // 수신한 메시지를 mqttpayload에 저장합니다.
  Serial.println(mqttpayload);                                                        
  if(mqttpayload == "HumiTimerEnd"){
    humi_timer = 0;
  }else if(mqttpayload == "TIMER_humi"){
    TIMER_HUMI = false;
  }else if(mqttpayload == "HUMI"){
    TIMER_HUMI = true;
    humi_timer = 0;
  }else if(mqttpayload == "EN1"){
    EN = 1;
  }else if(mqttpayload == "EN2"){
    EN = 2;
  }else if(mqttpayload == "EN3"){
    EN = 3;
  }else if(messageSize >= 10 && TIMER_HUMI == false){ 
    humi_timer = mqttpayload.toInt();
    Serial.println(humi_timer);
  }
  else if(messageSize >= 10 && TIMER_HUMI == true){
    humi_humi = mqttpayload.toInt();
    Serial.println(humi_humi);
  }
  
  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    Serial.print((char)mqttClient.read());
    
  }
  Serial.println();
}
