#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
const char *ssid = "********"; 
const char *password = "**********";  
#define BUTTON 4 //D2(gpio4)
int BtnValue = 0; // actual read value from pin4
int lightState = 0; // is the switch on = 1 or off = 0


//I've included some commenting markups from the LIFX LAN protocol website

// An instance of WifiUdp 
WiFiUDP Udp;

// The LIFX Header structure
#pragma pack(push, 1)
typedef struct {
  /* frame */
  uint16_t size; //unsigned integer to store the size value (in bytes) of the header and body
  uint16_t protocol:12; //The protocol value must be set to 1024 for LIFX packets and as a Uint16 turns into 00000000 00000100. The LIFX protocol only uses the first 12 bits of that Uint16, so we leave the 4 upper bits for the next fields.
  uint8_t  addressable:1; //Next is addressable and that must always be true, and tagged should be false.
  uint8_t  tagged:1; //Note that if we were broadcasting this packet to multiple lights then we'd want tagged to be true. And so we set the next two bits to be 01
  uint8_t  origin:2; //Finally our origin should be left as 0s, giving us 00. This means the 2 bytes that consist of protocol, addressable, tagged and origin become 00000000 00010100.
  uint32_t source; //Next up is source. The values 0 and 1 are reserved. So we'll set 2 here. The source is an arbitrary number and can be used to check whether the replies have come from a source you know about.
  /*                 So 2 as a Uint32 becomes 00000010 00000000 00000000 00000000. */

  /* frame address */
  uint8_t  target[8];  //Next up is our target. This field allows 8 bytes of data, but currently we only use the first 6. If we want to send this packet to multiple devices and have all of the respond then we leave this as all 0s and ensure tagged is set to true. In this example we only want to target one device and so we'll set a specific target.
  uint8_t  reserved[6];
  uint8_t  res_required:1;
  uint8_t  ack_required:1;
  uint8_t  :6;
  uint8_t  sequence; //Now we have a Uint8 for the sequence. Replies from a device will copy the triplet of (source, sequence, target) from the request packet and so we can use this to determine which request resulted in which reply. You should increment sequence for each message you send and then wrap to 0 after it reaches 255. Let's use 1 here, which as a Uint8 becomes 00000001.
  
  /* protocol header */
  /* This is specific to the 21SetPower type */
  uint64_t :64;  // 8 reserved bytes
  uint16_t type; //Finally we have a couple reserved fields and then type. Each packet has a different type that tells the device what kind of payload will follow. We're creating a SetColor (102) which has a type of 102. This becomes 01100110 00000000 when we pack it as a Uint16.
  uint16_t :16;  // 2 more reserved bytes
  /* variable length payload follows */
} lifx_header;
#pragma pack(pop)

// Device::SetColor Payload
#pragma pack(push, 1)
typedef struct {
  uint16_t level;
} lifx_payload_device_set_power;
#pragma pack(pop)

// Payload types
#define LIFX_DEVICE_SETPOWER 21 

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
/* not really required as we are going to broadcast the SetPower Packet to the whole network, but I'll keep it here anyway incase I work out how to send SetColour to specific Lights */
byte mac[] = { 0xD0, 0x73, 0xD5, 0x62, 0xB1, 0xA7 }; // D0:73:D5:62:B1:A7 ??? Doesn't seem to do anything, addresses all lights

unsigned int localPort = 8888;  // Port to listen for responses on
unsigned int lifxPort  = 56700; // Port used to send to send to LIFX devices

// Remote IP (In this case we broadcast to the entire subnet)
IPAddress broadcast_ip(255, 255, 255, 255);

// Packet buffer size
#define LIFX_INCOMING_PACKET_BUFFER_LEN 300



void setup() {
  Serial.begin(19200);
  pinMode(BUTTON, INPUT); // push button 
  pinMode(LED_BUILTIN, OUTPUT); // anything you want to control using a switch e.g. a Led
  WiFi.hostname("ESPSwitchTest");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
   delay(800);
   Serial.println("Connecting to WiFi..");
    
  }
}


void loop() {
    lifx_header header;
    lifx_payload_device_set_power payload;
    // Initialise both structures
    memset(&header, 0, sizeof(header));
    memset(&payload, 0, sizeof(payload));
    // Setup the header
    header.size = sizeof(lifx_header) + sizeof(payload); // Size of header + payload
    header.addressable = 1; //always set to True (1)
    header.tagged = 1; // set to True (1) as we are broadcasting
    header.protocol = 1024;  //Required for LIFX Packet Header
    header.source = 123; // Has something to do with replies? Unsure, and is fairy arbitrary
    header.ack_required = 1; // buffer byte
    header.sequence = 100;  //Used if you're doing waveform things so the UDP packets arrive in order
    header.type = LIFX_DEVICE_SETPOWER; //Header type 21 for Setpower, read more here https://lan.developer.lifx.com/docs/changing-a-device
    
    
    BtnValue = digitalRead(BUTTON); // read the pushButton State
      if(BtnValue == HIGH){
        if(lightState == 1){
          digitalWrite(LED_BUILTIN, HIGH);
          payload.level = 0; // If you specify 0 the light will turn off and if you specify 65535 the device will turn on.  
          // Send a packet on startup
          Udp.beginPacket(broadcast_ip, 56700); // broadcasts to addresses on network on the LIFX Port - 56700
          Udp.write((char *) &header, sizeof(lifx_header)); // Treat the structures like byte arrays
          Udp.write((char *) &payload, sizeof(payload));    // Which makes the data on wire correct
          Udp.endPacket();
          Serial.println("Sent packet with following payload:");
          Serial.println(payload.level);
          lightState = 0;
        }
        else { 
          digitalWrite(LED_BUILTIN, LOW);
          payload.level = 65535; // If you specify 0 the light will turn off and if you specify 65535 the device will turn on.  
          // Send a packet on startup
          Udp.beginPacket(broadcast_ip, 56700); // broadcasts to addresses on network on the LIFX Port - 56700
          Udp.write((char *) &header, sizeof(lifx_header)); // Treat the structures like byte arrays
          Udp.write((char *) &payload, sizeof(payload));    // Which makes the data on wire correct
          Udp.endPacket();
          Serial.println("Sent packet with following payload:");
          Serial.println(payload.level);
          lightState = 1;
        }
      }
      delay(300); // bad version of a button debouncer
  
}
