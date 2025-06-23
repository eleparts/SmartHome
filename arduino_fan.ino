/*
  ArduinoMqttClient - WiFi Simple Receive Callback

  This example connects to a MQTT broker and subscribes to a single topic.
  When a message is received it prints the message to the Serial Monitor,
  it uses the callback functionality of the library.

  The circuit:
  - Arduino MKR 1000, MKR 1010 or Uno WiFi Rev2 board

  This example code is in the public domain.
*/

#define fan 2           // 사용 제품에 연결된 핀에 각 제품명을 지정합니다.
#define PIR 3

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
boolean TIMER_PIR = false, fan_flag = false, PIR_10m = false;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  pinMode(fan, OUTPUT);   // 사용하는 두 제품의 입출력 모드를 설정합니다.
  pinMode(PIR, INPUT);
  
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
  mqttClient.setUsernamePassword("Your HomeAssistant SSID", "Your HomeAssistant PASSWORD");

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
  bool detect = digitalRead(PIR);

  Serial.print("PIR_10m : ");
  Serial.println(PIR_10m);
  Serial.print("detect : ");
  Serial.println(detect);
  Serial.print("century : ");
  Serial.println(fan_century_state);
  Serial.print("fan_flag : ");
  Serial.println(fan_flag);
  Serial.print("TIMER_PIR : ");
  Serial.println(TIMER_PIR);
  Serial.println("");

  // TIMER_PIR 변수를 통해 타이머 동작인지, PIR센서 동작인지 구분합니다.
  if (TIMER_PIR == false){                    // 타이머 동작을 선택했을 시,
    if (fan_flag == true) {                   // 선풍기 동작 O
      analogWrite(fan, fan_century);
    } else if (fan_flag == false) {           // 선풍기 동작 X
      analogWrite(fan, 0);
    }
  }else if (TIMER_PIR == true){               // PIR센서 동작을 선택했을 시,
    if (detect == true || PIR_10m == true) {  // 감지되지 않았을 시
      analogWrite(fan, fan_century);
    } else if (detect == false) {             // 감지되었을 시
      analogWrite(fan, 0);
    }
  }
  
  // call poll() regularly to allow the library to receive MQTT messages and
  // send MQTT keep alives which avoids being disconnected by the broker
  mqttClient.poll();

  mqttClient.beginMessage("ArduinoFan");      // "ArduinoFan"이라는 이름을 토픽으로 데이터 송신을 시작합니다.
  mqttClient.print("선풍기 세기 : ");         // 선풍기 세기, 타이머, 움직임 감지 등 현재 동작 상태를 나타냅니다.
  if (fan_century_state == 0) {
    mqttClient.println("OFF");
  } else if (fan_century_state == 1) {
    mqttClient.println("미풍");
  } else if (fan_century_state == 2) {
    mqttClient.println("약풍");
  } else if (fan_century_state == 3) {
    mqttClient.println("강풍");
  }

  if(TIMER_PIR == false){
    mqttClient.print("선풍기 타이머 : ");
    if (fan_timer_state == 0) {
      mqttClient.println("30분 타이머");
    } else if (fan_timer_state == 1) {
      mqttClient.println("1시간 타이머");
    } else if (fan_timer_state == 2) {
      mqttClient.println("1시간 30분 타이머");
    } else if (fan_timer_state == 3) {
      mqttClient.println("2시간 타이머");
    } else if (fan_timer_state == 4) {
      mqttClient.println("연속");
    }
  }else if(TIMER_PIR == true){
    mqttClient.print("움직임 감지 여부 : ");
    if(detect == false){
      mqttClient.println("X");
    }else if(detect = true){
      mqttClient.println("O");
    }
  }
  mqttClient.endMessage();                    // "ArduinoFan" 이름의 토픽으로 하는 데이터 송신을 끝냅니다.

  mqttClient.beginMessage("ArduinoFanPir");   // "ArduinoFanPir"이라는 이름을 토픽으로 데이터 송신을 시작합니다.
  if(detect == false){
    mqttClient.print("X");
  }else if(detect = true){
    mqttClient.print("O");
  }
  mqttClient.endMessage();                    // "ArduinoFanPir" 이름의 토픽으로 하는 데이터 송신을 끝냅니다.
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
  if (mqttpayload == "TIMER"){
    TIMER_PIR = false;
  } else if (mqttpayload == "PIR"){
    TIMER_PIR = true;
  } else if (mqttpayload == "OFF") {
    fan_century_state = 0;
    fan_century = 0;
    fan_flag = false;
  } else if (mqttpayload == "미풍") {
    fan_century_state = 1;
    fan_century = 85;
  } else if (mqttpayload == "약풍") {
    fan_century_state = 2;
    fan_century = 170;
  } else if (mqttpayload == "강풍") {
    fan_century_state = 3;
    fan_century = 255;
  } else if (mqttpayload == "10m") {
    PIR_10m = true;
  } else if (mqttpayload == "30m") {
    fan_timer_state = 0;
    fan_flag = true;
  } else if (mqttpayload == "60m") {
    fan_timer_state = 1;
    fan_flag = true;
  } else if (mqttpayload == "90m") {
    fan_timer_state = 2;
    fan_flag = true;
  } else if (mqttpayload == "120m") {
    fan_timer_state = 3;
    fan_flag = true;
  } else if (mqttpayload == "연속") {
    fan_timer_state = 4;
    fan_flag = true;
  } else if (mqttpayload == "FanTimerEnd") {
    fan_flag = false;
    PIR_10m = false;
  }
  
  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    Serial.print((char)mqttClient.read());
    
  }
  Serial.println();

  Serial.println();
}
