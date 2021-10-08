//Created by: Mogamat Nur Ally
//Date: 13/02/2021
//This sketch is intended for use in the Water Management Project

//Libraries needed for MQTT communication between node and modem (LG02)
#include <SPI.h>
#include <LoRa.h>

//Amount of free water required by the Western Cape Government to be provided to each household
const int freeWater = 6, nodeNum = 4567;
//variables representing the state of the node
bool paid = false, on = true;
//amount of water used
float litres = 0;
//String variables used in communication and logging
char tem_1[8] = {"\0"}, nodeStatus[8] = {"\0"}, cmd[8] = {"\0"}, cmdNode[8]={"\0"}, cmdString[8]={"\0"}, nodeName[8] = {"\0"}, accStatus[8] = {"\0"};
char *node_id = "<4567>";  //From LG01 via web Local Channel settings on MQTT.Please refer <> dataformat in here.
uint8_t datasend[72]; //Data packet sent to LG02
unsigned int count = 1, usage = 0; //counters for message logging and water used
unsigned long new_time, old_time = 0; //timer variables determining
int lastButtonState = 1; //integers for testing equality and changes in the input to the node

void setup()
{
  Serial.begin(9600);
  //configure pin 2 as an input and enable the internal pull-up resistor
  pinMode(3, INPUT_PULLUP);
  //configure pin 4 as an output node
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  while (!Serial);
  Serial.println(F("Start MQTT Example"));
  if (!LoRa.begin(868100000))   //868000000 is frequency
  {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  // Setup Spreading Factor (6 ~ 12)
  LoRa.setSpreadingFactor(7);

  // Setup BandWidth, option: 7800,10400,15600,20800,31250,41700,62500,125000,250000,500000
  //Lower BandWidth for longer distance.
  LoRa.setSignalBandwidth(125000);

  // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8)
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x34);
  Serial.println("LoRa init succeeded.");

  LoRa.onReceive(onReceive);
  LoRa.receive();



}

void dhtTem()
{
  //this function creates the log outputted to the node console
  litres = float(usage) * 0.5;
  Serial.println(F("flat number:"));
  Serial.print(float(nodeNum));
  Serial.print("has so far used:");
  Serial.print("[");
  Serial.print(litres);
  Serial.print("litres");
  Serial.print("]");
  Serial.print(" The water supply is:[");
  if (on) {
    Serial.print("ON");
  } else {
    Serial.print("OFF");
  }
  Serial.print("]");
  Serial.println("");
}

void dhtWrite()
{
  //this function creates the formatted data packet to be sent to the LG02
  char data[100] = "\0";
  for (int i = 0; i < 50; i++)
  {
    data[i] = node_id[i];
  }
  if (on) {
    dtostrf(1, 0, 1, nodeStatus);
  } else {
    dtostrf(0, 0, 1, nodeStatus);
  }

  dtostrf(litres, 0, 1, tem_1);


  if (paid) {
    dtostrf(1, 0, 1, accStatus);
  } else {
    dtostrf(0, 0, 1, accStatus);
  }

  // Serial.println(tem_1);
    // Serial.println(tem_1);
  strcat(data, "field1=");
  strcat(data, tem_1);
  strcat(data, "&field2=");
  strcat(data, nodeStatus);
  strcat(data, "&field3=");
  strcat(data, accStatus);
  strcpy((char *)datasend, data);

  //Serial.println((char *)datasend);
  //Serial.println(sizeof datasend);

}

void SendData()
{
  //This function sends the data paccket
  LoRa.beginPacket();
  LoRa.print((char *)datasend);
  LoRa.endPacket();
  Serial.println("Packet Sent");
}
void ControlValve ()
{
  //This function controls the state of the LED (valve)
  if (paid == false && litres > freeWater)
  {
    //close valve
    digitalWrite(4, LOW);
    digitalWrite(5, HIGH);
    on = false;
  } else if(on == true){
    //open valve
    digitalWrite(4, HIGH);
    digitalWrite(5, LOW);
  }else if (on == false){
    digitalWrite(4, LOW);
    digitalWrite(5, HIGH);
    }
}

void ReadUsage () {
  //read the pushbutton value into a variable
  int sensorVal = digitalRead(3);
  //print out the value of the pushbutton
  //Serial.println(sensorVal);
  // Keep in mind the pull-up means the pushbutton's logic is inverted. It goes
  // HIGH when it's open, and LOW when it's pressed.

  //check if the state of the input has changed
  if (sensorVal != lastButtonState) {
    if (sensorVal == HIGH) {
      // if the current state is LOW then the button went from on to off and increment the counter:
      usage++;
    }
  }
  lastButtonState = sensorVal;
}

void Update() {
  //send and recieve updates to and from the LG02 every minute
  new_time = millis();
  if (new_time - old_time >= 6000 || old_time == 0)
  {
    old_time = new_time;
    Serial.print("###########    ");
    Serial.print("UPLOAD COUNT=");
    Serial.print(count);
    Serial.println("    ###########");
    count++;
    dhtTem();
    dhtWrite();
    SendData();
    LoRa.receive();
  }
}

void loop()
{
  ControlValve();
  ReadUsage();
  Update();
}

void onReceive(int packetSize) {
  memset(cmd, 0, sizeof(cmdNode));
  memset(cmd, 0, sizeof(cmdString));
  memset(cmd, 0, sizeof(cmd));
  // received a packet
  Serial.print("Received packet : ");

  // read packet
  for (int i = 0; i < packetSize; i++) {
    //Serial.print((char)LoRa.read());
    //store command in variable
    cmd[i] = (char)LoRa.read();
  }
  Serial.print(cmd);
  Serial.print("\n\r");

  for (int j = 0; j < sizeof(cmd); j++) {
    if (j < 4) {
      cmdNode[j]=cmd[j];
    } else if (j > 4) {
      cmdString[j-5] = cmd[j];
    }
  }
  if (atoi(cmdNode) == nodeNum) {
    if (strcmp(cmdString,"CV")==0) {
      on = false;
    } else if (strcmp(cmdString,"OV")==0) {
      on = true;
    } else if (strcmp(cmdString,"CAS")==0) {
      paid = !paid;
    }
  }
}
