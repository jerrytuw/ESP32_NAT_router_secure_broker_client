extern "C" void mainstart();
extern "C" void mainloop();
extern bool ap_connect;

#include <stdio.h>
#include <Arduino.h>
#include "WiFi.h"
#include "TinyMqtt.h"
#include "cert.h"
#include "private_key.h"
#include <PubSubClient.h>

// We will use wifi
#include <WiFi.h>

// Includes for the server
// Note: We include HTTPServer and HTTPSServer
#include <SSLCert.hpp>

// The HTTPS Server comes in a separate namespace. For easier use, include it here.
using namespace httpsserver;

// Create an SSL certificate object from the files included above
SSLCert cert = SSLCert(
                 example_crt_DER, example_crt_DER_len,
                 example_key_DER, example_key_DER_len
               );

// First, we create the HTTPSServer with the certificate created above
//HTTPServer secureServer = HTTPServer();

#define PORT 8883

MqttBroker broker(&cert, PORT);
/* for T4ME2 by PM @TUW 05/22
    based on ESP32LR20 board
   r2d2 simple estimation:
   max down move time = 10s
   up move to sit down in 3-8s
*/

#define MYAPP "r2d2"
#define VERSIONDATE "v220512"

const char* sketch_version = VERSIONDATE MYAPP " @";
pthread_t t2;

#define USETLS

// below settings control the blink LED
#define BLINKTIME 1*1000L // time motor is opening
#define INCREMENT 1000L
#define INCREMENTMAX 10000L

#define MQTTdebug(x) MQTTpublish("r2d2/event/state",x, false)

#define UP digitalWrite(R1, HIGH);digitalWrite(R2, LOW)
#define DOWN digitalWrite(R2, HIGH);digitalWrite(R1, LOW)
#define STOP digitalWrite(R2, LOW);digitalWrite(R1, LOW)

unsigned long movetime = 0L;
unsigned long starttime;

unsigned long LEDtime = 0L;

#define LED         23
#define R1          33
#define R2          25          // Configurable, see typical pin layout above
// also IR output - 22 is IR in
// end flush settings

String MQTTserver; // gets the alternatives

#ifdef USETLS
  #include <WiFiClientSecure.h>
  WiFiClientSecure WifiClient;
  #define mqttPort 8883
#else
  WiFiClient WifiClient;
  #define mqttPort 1883
#endif

const char *mqttUser = "tuw";
const char *mqttPassword = "project2017";
PubSubClient MQTTclient(WifiClient);

bool hadMQTT = false; // for MQTT re-connection tries
byte present = 0; // current presence status

bool ontoilet = false;

bool MQTTpublish(const char* topic, const char* msg, bool retain = false)
{
  return MQTTclient.publish(topic, msg, retain);
}

void LEDblink()
{
  digitalWrite(LED, LOW);
  LEDtime = millis();
}

void estimateSettings(int bodySize, bool wheelchair)
{
  float h_sitdown = 0;
  float t_sitdown = 0;
  float h_standup = 0;
  float t_standup = 0;

  if (bodySize == 0)
  {
    movetime = INCREMENTMAX;
    starttime = millis();
    DOWN;
    printf("estimation reset\n");
    return;
  }
  if (!wheelchair)
  {
    bodySize = _max(_min(bodySize, 200), 150) - 150; // 150-200cm only
    printf("size %d\n", bodySize);
    h_sitdown = 50 + ((float)bodySize / 8.); // start with ~3 up to ~8
    t_sitdown = 3;
    h_standup = 53 + ((float)bodySize / 8.);
    t_standup = 6;
  }
  else
  {
    h_sitdown = 48;
    t_sitdown = 0;
    h_standup = 50;
    t_standup = 0;
  }
  h_sitdown = _min(h_sitdown, 55);
  h_standup = _min(h_standup, 65);

  h_sitdown -= 47;
  h_standup -= 47;

  movetime = (unsigned long) ((float)INCREMENT * h_sitdown);
  starttime = millis();
  UP;
  printf("### estimation results %f and %f, %lu\n", h_sitdown, h_standup, movetime);
  MQTTdebug((String("### estimated hs=") + String(h_sitdown) + String(" hu=") + String(h_standup)+ String(" t=") + String(movetime)).c_str());
  /*char msg[100];
    sprintf(msg, "{\"height\": %3.1f, \"tilt\": %3.1f}", h_sitdown, t_sitdown);
    MQTTpublish("lift/command/move", msg, false);*/

  //endheight = h_standup;

  /*sprintf(msg, "{\"height\": %3.1f, \"tilt\": %3.1f}", h_standup, t_standup);
    MQTTpublish("lift/config/end_position", msg, false);*/

  MQTTpublish("tts/speak", "ich stelle mich auf sie ein.", false);
}

// look up value position in message
// TODO: check ArduinoJSON use?
char *getJSONval(char *data, const char *member, unsigned int length)
{
  if (length == 0) return NULL;

  char *val = strstr(data, member);
  if (val)
  {
    val = strstr(val, ":");
    if (val)
    {
      while ((*val) && (((*val) == ':') || ((*val) == ' ') || ((*val) == '\'') || ((*val) == '"')))
      {
        //printf(*val);
        val++;
      }
      //printf("%s\n",);
      return val;
    }
  }
  return NULL;
}

// MQTT presence data change
void processstatus(char* msg, char*topic, unsigned int length)
{
  char *val = NULL;
  float h = -1;

  printf("@presence status = %d\n", present);

#ifdef SOMEPRESENCE
  if (present == 0)
  {
    present = 1;
    MQTTpublish("presence/event/somepresent", "1", false);
  }
  lastpresence = millis(); // remember last presence data
#endif

  if (present == 2)
  {
    val = getJSONval(msg, "height", length);
    if (val)
    {
      if (sscanf(val, "%f", &h) || sscanf(val + 1, "%f", &h))
      {
        val = getJSONval(msg, "on_toilet", length);
        if (val)
        {
          if (*val == '1') ontoilet = true;
        }
        val = getJSONval(msg, "wheelchair", length);
        if (val)
        {
          int height = h;
          bool wheelchair = (*val == '1');
          //MQTTdebug((String("estimate=") + String(estimation)).c_str());
          //MQTTdebug((String("adjust h=") + String(height) + String(" w=") + String(wheelchair)).c_str());
          printf("try estimate %d, %d\n", height, wheelchair);
          estimateSettings(height, wheelchair);
          present = 3;
        }
      }
    }
  }
  else
  {
    if (present < 2) present++;
    //MQTTdebug((String("p=") + String(present)).c_str());
    printf("%s\n","else presence");
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int len)
{
  char msg[300];

  printf("Message arrived in topic: ");
  printf("%s\n",topic);

  printf("Message:");
  for (int i = 0; i < min(len, sizeof(msg)); i++)
  {
    msg[i] = (char)(payload[i]);
    printf("%c",(payload[i]));
  }
  msg[len] = 0;

  printf("\n");

  if (strstr(topic, "r2d2/command"))
  {
    if (strstr((const char*)msg, "R1ON"))
    {
      digitalWrite(R1, HIGH);
      LEDblink();
    }
    if (strstr((const char*)msg, "R2ON"))
    {
      digitalWrite(R2, HIGH);
      LEDblink();
    }
    if (strstr((const char*)msg, "R1OFF"))
    {
      digitalWrite(R1, LOW);
      LEDblink();
    }
    if (strstr((const char*)msg, "R2OFF"))
    {
      digitalWrite(R2, LOW);
      LEDblink();
    }
  }
  else if (strstr(topic, "query"))
  {
    char msg[150];
    sprintf(msg, "%s@%s", sketch_version, WiFi.localIP().toString().c_str());
    MQTTclient.publish("r2d2/info", msg, false);
  }
  // watch presence
  if (strstr(topic, "presence/event/present"))
  {
    printf("presence user ");
    if (msg[0] == '1')
    {
      if (present == 0) present = 1; // avoid reset on 2nd person entering at the same time
    }
    else
    {
      ontoilet = false;
      present = 0;
      movetime = INCREMENTMAX;
      starttime = millis();
      DOWN;
      MQTTdebug("Go back to default");
      printf("Go back to default\n");
    }
    printf("%d\n", present);
  }
  // watch presence data
  if (strstr(topic, "presence/event/status"))
  {
    processstatus(msg, topic, len);
  }
  if (strstr(topic, "r2d2/estimate"))
  {
    char* val;
    int w = 0;
    float h;

    val = getJSONval(msg, "height", len);
    if (val)
    {
      if (sscanf(val, "%f", &h) || sscanf(val + 1, "%f", &h))
      {
        val = getJSONval(msg, "wheelchair", len);
        if (val)
        {
          if (sscanf(val, "%d", &w) || sscanf(val + 1, "%d", &w))
            estimateSettings((int)h, w == 1);
        }
      }
    }
  }
}


// try to connect with MQTT server
void connectMqtt(void)
{
  int conn_count = 0;
  String clientId = "R2D2lift" + WiFi.macAddress();

   {
#ifdef USETLS
    WifiClient.setInsecure();
#endif
    printf("\n%ld - Connecting to MQTT...\n", millis());
    MQTTclient.setCallback(mqttCallback);

    IPAddress result;
 
    if (!MQTTclient.connected())
    {
      MQTTserver = "127.0.0.1"; // only localhost seems to work for internal IP?
      printf("%ld Trying local broker address '%s' ...\n", millis(), MQTTserver.c_str());
      MQTTclient.setServer(MQTTserver.c_str(), mqttPort); // finally try test server

      while (!MQTTclient.connected())
      {
        if (conn_count++ > 5)
          break;
        printf(".");

        if (MQTTclient.connect(clientId.c_str(), mqttUser, mqttPassword, "r2d2/connected", MQTTQOS1, true, "0"))
        {
          printf("\nconnected: %d, state: %d\n", MQTTclient.connected(), MQTTclient.state());
        }
        else
        {
          delay(500);
        }
      }
    }
    if (MQTTclient.connected())
    {
      String msg = "initialized ";
      if (hadMQTT) msg = "reconnected ";
      msg += sketch_version;
      msg += WiFi.localIP().toString();

      hadMQTT = true; // remember we had connection
      MQTTclient.subscribe("presence/event/present");
      MQTTclient.subscribe("r2d2/query");
      MQTTclient.subscribe("r2d2/command");
      MQTTclient.subscribe("r2d2/estimate");
      MQTTclient.subscribe("presence/event/status");
      MQTTclient.publish(MYAPP "/connected", "1", true);
      printf("- Subscribed to MQTT on ");
      printf("%s\n",MQTTserver.c_str());
      MQTTclient.publish(MYAPP "/event/state", msg.c_str(), false);
    }
    else
    {
      printf("%ld",millis());
      printf("- Could not connect to MQTT, state: %d\n", MQTTclient.state());
    }
  }
}

/* this needs to run in background*/
void * broker_thread(void * p)
{
    while (true)
    {
      broker.loop();
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

//*****************************************************************************************//
void setupclient() {
  digitalWrite(R1, LOW);
  digitalWrite(R2, LOW);
  digitalWrite(LED, HIGH);

  pinMode(R1, OUTPUT);
  pinMode(R2, OUTPUT);
  pinMode(LED, OUTPUT);

  printf("%s\n","NAT Router/secure Broker Relay Module");    //shows in serial that it is starting

  pthread_create(&t2, NULL,  broker_thread, NULL);    //spawn broker loop process

  connectMqtt();  //start local client
  printf("\nupstream: %s\n",ap_connect?"yes":"no");
}

//*****************************************************************************************//
void loopclient() {
  {
    if (!MQTTclient.connected())
    {
      printf("%s\n","_  MQTT not connected - retry");
      printf("%d",MQTTclient.state());
      connectMqtt();
    }
    else MQTTclient.loop(); //MQTT check loop
  }
  if ((LEDtime != 0L) && ((millis() - LEDtime) > BLINKTIME))
  {
    LEDtime = 0L;
    digitalWrite(LED, HIGH);
  }
  if (movetime != 0L)
  {
    //printf("%lu\n", millis() - starttime);
    if (((millis() - starttime) > movetime))
    {
      STOP;
      movetime = 0L;
      MQTTdebug("stopped");
    }
  }
  delay(10);
}
//*****************************************************************************************//


extern "C" void app_main()
{
     //initArduino();
        ESP_LOGI("first", "###### arduino init");

     //Serial.begin(115200);
     //Serial.printf("\n##################hello\n");

  mainstart();
  ESP_LOGI("first", "###### arduino broker");
  while(!ap_connect) delay(1000);
  broker.begin();
  ESP_LOGI("first", "###### arduino broker end");
  setupclient();
  while(true) loop();
  
}

void loop()
{
  delay(1);
  loopclient();
  //mainloop();
}