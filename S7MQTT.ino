#include "Platform.h"
#include "Settimino.h"
#include <PubSubClient.h>
#define DO_IT_SMALL

IPAddress Local(XXX, XXX, XXX, XXX); // Local Address

IPAddress PLC(XXX, XXX, XXX, XXX); // PLC Address
int DBNum = 111; // This DB must be present in your PLC
byte Buffer[4];
S7Client Clients;

char ssid[] = "XXXXXXXXXXXX";    // Your network SSID (name)
char pass[] = "XXXXXXXXXXXX";  // Your network password (if any)

const char* mqtt_server = "XXXX";
const char* topicIn = "XXX";
const char* topicOut = "XXX";

const char* user = "XXXX";
const char* pw = "XXX";
int port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;


unsigned long Elapsed; // To calc the execution time

//----------------------------------------------------------------------
// Setup : Init Ethernet and Serial port
//----------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  //WiFi.config(Local, Gateway, Subnet);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Local IP address : ");
  Serial.println(WiFi.localIP());
  client.setServer(mqtt_server, port);
  client.setCallback(callback);
}

//----------------------------------------------------------------------
// Main Loop
//----------------------------------------------------------------------
void loop()
{
  int Size, Result;
  void *Target;

#ifdef DO_IT_SMALL
  Size = 4;
  Target = NULL; // Uses the internal Buffer (PDU.DATA[])
#else
  Size = 1024;
  Target = &Buffer; // Uses a larger buffer
#endif

  // Connection
  while (!Clients.Connected)
  {
    if (!Connect())
      delay(500);
  }

  Serial.print("Reading "); Serial.print(Size); Serial.print(" bytes from DB"); Serial.println(DBNum);
  // Get the current tick
  MarkTime();
  Result = Clients.ReadArea(S7AreaDB, // We are requesting DB access
                            DBNum,    // DB Number
                            772,        // Start from byte N.0
                            Size,     // We need "Size" bytes
                            Target);  // Put them into our target (Buffer or PDU)


  if (Result == 0)
  {
    ShowTime();
    //Dump(Target, Size);
    pbyte buf;
    buf = pbyte(&PDU.DATA[0]);

    float temp = S7.FloatAt(buf, 0);
    Serial.println(temp);
    SendMQTT(temp);
  }
  else
    CheckError(Result);

  delay(500);

}
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.println((char)payload[i]);
  }
  value = ((char)payload[0]) - '0';
  
  bool state = true;
  int Result = 0;
  
  if (value == 4){
  Result = Clients.WriteBit(S7AreaDB, 421, 0, 0,state);
  Result = Clients.WriteBit(S7AreaDB, 421, 4, 0,state);
  }
  if (value == 3){
  Result = Clients.WriteBit(S7AreaDB, 420, 0, 0,state);
  Result = Clients.WriteBit(S7AreaDB, 420, 4, 0,state);
  }
  if (value == 2){
  Result = Clients.WriteBit(S7AreaDB, 122, 4, 1,state);
  }
  if (value == 1){
  Result = Clients.WriteBit(S7AreaDB, 122, 4, 0,state);
  }
  Serial.println(Result);

}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), user, pw)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe(topicIn);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}
//----------------------------------------------------------------------
// Connects to the PLC
//----------------------------------------------------------------------
bool Connect()
{
  int Result = Clients.ConnectTo(PLC, 0,  3); // Rack (see the doc.),// Slot (see the doc.)
  Serial.print("Connecting to "); Serial.println(PLC);
  if (Result == 0)
  {
    Serial.print("Connected ! PDU Length = "); Serial.println(Clients.GetPDULength());
  }
  else
    Serial.println("Connection error");
  return Result == 0;
}
//----------------------------------------------------------------------
// Dumps a buffer (a very rough routine)
//----------------------------------------------------------------------
void Dump(void *Buffer, int Length)
{
  int i, cnt = 0;
  pbyte buf;

  if (Buffer != NULL)
    buf = pbyte(Buffer);
  else
    buf = pbyte(&PDU.DATA[0]);

  Serial.print("[ Dumping "); Serial.print(Length);
  Serial.println(" bytes ]===========================");
  for (i = 0; i < Length; i++)
  {
    cnt++;
    if (buf[i] < 0x10)
      Serial.print("0");
    Serial.print(buf[i], HEX);
    //Serial.print(buf[i]);
    Serial.print(" ");
    if (cnt == 16)
    {
      cnt = 0;
      Serial.println();
    }
  }
  Serial.println("===============================================");

}
//MQTT
void SendMQTT(float buf)
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    //snprintf (msg, 50, "%d", buf);
    snprintf (msg, 50, "%f", buf);
    Serial.print("Publishing...: ");
    Serial.println(msg);
    client.publish(topicOut, msg);
    ++value;
  }
}
//----------------------------------------------------------------------
// Prints the Error number
//----------------------------------------------------------------------
void CheckError(int ErrNo)
{
  Serial.print("Error No. 0x");
  Serial.println(ErrNo, HEX);

  // Checks if it's a Severe Error => we need to disconnect
  if (ErrNo & 0x00FF)
  {
    Serial.println("SEVERE ERROR, disconnecting.");
    Clients.Disconnect();
  }
}
//----------------------------------------------------------------------
// Profiling routines
//----------------------------------------------------------------------
void MarkTime()
{
  Elapsed = millis();
}
//----------------------------------------------------------------------
void ShowTime()
{
  // Calcs the time
  Elapsed = millis() - Elapsed;
  Serial.print("Job time (ms) : ");
  Serial.println(Elapsed);
}
