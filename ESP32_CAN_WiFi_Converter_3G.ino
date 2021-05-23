
// CAN to WiFi Converter for connecting the CAN devices with the HOST   //
// This Program is developed for connecting to CAR and communicating with different ECU's.  //
// For reference visit to https://github.com/miwagner/ESP32-Arduino-CAN  //
// Sketch developed by 3G & IoT Team  //

#include <Arduino.h>
#include <ESP32CAN.h>
#include <CAN_config.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#define LED_INDICATIONS

WiFiMulti wifiMulti;

//how many clients should be able to telnet to this ESP32
#define MAX_SRV_CLIENTS 1
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWARD";

IPAddress local_IP(192, 168, 1, 50); //Ip address assigned to ESP32 to access
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8); //optional DNS
IPAddress secondaryDNS(8, 8, 4, 4); //optional DNS

WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];

CAN_device_t CAN_cfg;               // CAN Config
const int rx_queue_size = 10;       // Receive Queue size

uint8_t i;
uint16_t reqId = 0;
uint16_t resId = 0;
uint8_t DLC = 0;
uint8_t SID = 0;
uint8_t resLen = 0;
uint8_t resLenRecv = 0;
uint8_t resp[255];

//Indication leds on ESP32
#ifdef LED_INDICATIONS
#define WIFI_COMM_OK_LED              32    // Indicates if ESP32 is connected to WiFi or not. 
#define CLIENT_CONN_LED               22    // Indicates if client connected with system or not.
#define CAN_RX_LED                    23    // Indicates CAN communication
#endif

void setup()
{
#ifdef LED_INDICATIONS
  pinMode(CAN_RX_LED, OUTPUT);
  pinMode(WIFI_COMM_OK_LED, OUTPUT);
  pinMode(CLIENT_CONN_LED, OUTPUT);
#endif

  Serial.begin(115200);

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  wifiMulti.addAP(ssid, password);

  Serial.println("Connecting Wifi ");
  for (int loops = 10; loops > 0; loops--) {
    if (wifiMulti.run() == WL_CONNECTED) {
      Serial.println("");
      Serial.print("WiFi connected. ");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      break;
    }
    else {
      Serial.println(loops);
      delay(1000);
    }
  }

  if (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.println("WiFi connect failed");
    delay(1000);
    ESP.restart();
  }

  server.begin();
  server.setNoDelay(true);

  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");

  CAN_cfg.speed = CAN_SPEED_500KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_5;
  CAN_cfg.rx_pin_id = GPIO_NUM_4;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));

  // Init CAN Module
  ESP32Can.CANInit();

  Serial.println("CAN Comm. Ready.");
}

void loop() {

  CAN_frame_t rx_frame;

  if (wifiMulti.run() == WL_CONNECTED) {

    //check if there are any new clients

#ifdef LED_INDICATIONS
    digitalWrite(WIFI_COMM_OK_LED, HIGH);     //WiFi connected so turn ON led
#endif

    if (server.hasClient()) {

      for (i = 0; i < MAX_SRV_CLIENTS; i++) {
        //find free/disconnected spot
        if (!serverClients[i] || !serverClients[i].connected()) {
          if (serverClients[i]) serverClients[i].stop();

          serverClients[i] = server.available();
          if (!serverClients[i]) Serial.println("Available broken");
          Serial.print("New client: ");
          Serial.print(i);
          Serial.print(' ');
          Serial.println(serverClients[i].remoteIP());
          break;
        }
      }
      if (i >= MAX_SRV_CLIENTS) {
        //no free/disconnected spot so reject
        server.available().stop();
      }
    }

    //check clients for data
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (serverClients[i] && serverClients[i].connected()) {

#ifdef LED_INDICATIONS
        digitalWrite(CLIENT_CONN_LED, HIGH);
#endif

        if (serverClients[i].available()) {

          //printf("\n");
          //Serial.println("WiFi Data Received");

          size_t len = serverClients[i].available();
          uint8_t sbuf[len];
          serverClients[i].readBytes(sbuf, len);

          // Send CAN Message
          if (len == 13)
          {
            CAN_frame_t tx_frame;

            tx_frame.FIR.B.FF = CAN_frame_std;
            tx_frame.MsgID = sbuf[0] << 8 | sbuf[1]; //0x07E5;
            reqId = tx_frame.MsgID;
            resId = sbuf[2] << 8 | sbuf[3];
            tx_frame.FIR.B.DLC = sbuf[4]; //8;
            DLC = sbuf[4];
            SID = sbuf[6];
            tx_frame.data.u8[0] = sbuf[5]; //0x03;
            tx_frame.data.u8[1] = sbuf[6]; //0x22;
            tx_frame.data.u8[2] = sbuf[7]; //0xF1;
            tx_frame.data.u8[3] = sbuf[8]; //0x81;
            tx_frame.data.u8[4] = sbuf[9]; //0x00;
            tx_frame.data.u8[5] = sbuf[10]; //0x00;
            tx_frame.data.u8[6] = sbuf[11]; //0x00;
            tx_frame.data.u8[7] = sbuf[12]; //0x00;
            ESP32Can.CANWriteFrame(&tx_frame);

            printf(" Request to 0x%08X, DLC %d, Data ", tx_frame.MsgID,  tx_frame.FIR.B.DLC);
            for (int i = 0; i < tx_frame.FIR.B.DLC; i++) {
              printf("0x%02X ", tx_frame.data.u8[i]);
            }
            printf("\n");
            Serial.println("Data Sent.");
          }
        }
      }
      else {
#ifdef LED_INDICATIONS
        digitalWrite(CLIENT_CONN_LED, LOW);
#endif

        if (serverClients[i]) {
          serverClients[i].stop();
        }
      }
    }

    // Receive next CAN frame from queue
    if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {

#ifdef LED_INDICATIONS
      digitalWrite(CAN_RX_LED, HIGH);     //received some frame on CAN so blink led
#endif

      if ( rx_frame.MsgID == resId ) {
        if (rx_frame.FIR.B.FF == CAN_frame_std) {
          printf("New standard frame");
        }
        else {
          printf("New extended frame");
        }

        if (rx_frame.FIR.B.RTR == CAN_RTR) {
          printf(" RTR from 0x%08X, DLC %d\r\n", rx_frame.MsgID,  rx_frame.FIR.B.DLC);
        }
        else {
          printf(" Response from 0x%08X, DLC %d, Data ", rx_frame.MsgID,  rx_frame.FIR.B.DLC);
          for (int i = 0; i < rx_frame.FIR.B.DLC; i++) {
            printf("0x%02X ", rx_frame.data.u8[i]);
          }
          printf("\n");

          if (rx_frame.data.u8[0] == 0x10) {
            //Serial.println("In equal to 0x10");
            //First frame of multi-part response received.
            resLen = rx_frame.data.u8[1];

            for (int i = 1; i < rx_frame.FIR.B.DLC; i++) {
              resp[resLenRecv] = rx_frame.data.u8[i];
              resLenRecv++;
            }

            //Send flow control message
            CAN_frame_t tx_frame;

            tx_frame.FIR.B.FF = CAN_frame_std;
            tx_frame.MsgID = reqId;
            tx_frame.FIR.B.DLC = DLC;
            tx_frame.data.u8[0] = 0x30;
            tx_frame.data.u8[1] = 0x00;
            tx_frame.data.u8[2] = 0x00;
            tx_frame.data.u8[3] = 0x00;
            tx_frame.data.u8[4] = 0x00;
            tx_frame.data.u8[5] = 0x00;
            tx_frame.data.u8[6] = 0x00;
            tx_frame.data.u8[7] = 0x00;
            ESP32Can.CANWriteFrame(&tx_frame);

            printf(" Request to 0x%08X, DLC %d, Data ", tx_frame.MsgID,  tx_frame.FIR.B.DLC);
            for (int i = 0; i < tx_frame.FIR.B.DLC; i++) {
              printf("0x%02X ", tx_frame.data.u8[i]);
            }
            printf("\n");
            Serial.println("Data Sent.");
          }
          else if (rx_frame.data.u8[0] < 0x10) {
            //Serial.println("In less than 0x10");
            resLen = rx_frame.data.u8[0];

            if (rx_frame.data.u8[1] == 0x7F) { 
              //NACK received
              if ((rx_frame.data.u8[2] == SID) && (rx_frame.data.u8[3] == 0x78)) {
                //Serial.println("NRC received. Waiting for positive response.");
              }
              else {
                for (int i = 1; i < rx_frame.FIR.B.DLC; i++) {
                  resp[resLenRecv] = rx_frame.data.u8[i];
                  resLenRecv++;
                }
              }
            }
            else {
              for (int i = 1; i < rx_frame.FIR.B.DLC; i++) {
                resp[resLenRecv] = rx_frame.data.u8[i];
                resLenRecv++;
              }
            }

//            for (int i = 1; i < rx_frame.FIR.B.DLC; i++) {
//              resp[resLenRecv] = rx_frame.data.u8[i];
//              resLenRecv++;
//            }
          }
          else if (rx_frame.data.u8[0] > 0x20) {
            //Serial.println("In greater than 0x20");
            for (int i = 1; i < rx_frame.FIR.B.DLC; i++) {
              resp[resLenRecv] = rx_frame.data.u8[i];
              resLenRecv++;

              if (resLenRecv > resLen) {
                break;
              }
            }
          }

          if (resLenRecv > resLen) {
            //push UART data to all connected telnet clients

            for (i = 0; i < MAX_SRV_CLIENTS; i++) {
              if (serverClients[i] && serverClients[i].connected()) {
                //              byte MsgID_DLC[3] = { highByte(rx_frame.MsgID), lowByte(rx_frame.MsgID), rx_frame.FIR.B.DLC };
                //              serverClients[i].write(MsgID_DLC, 3);
                //serverClients[i].write(rx_frame.data.u8, rx_frame.FIR.B.DLC);

                //serverClients[i].write(resp, resLenRecv);
                serverClients[i].write(resp, resLenRecv);   //SID
                delay(1);
              }
            }

            resLenRecv = 0;
            resLen = 0;
          }
        }
      }
#ifdef LED_INDICATIONS
      digitalWrite(CAN_RX_LED, LOW);     //received some frame on CAN so blink led
#endif
    }
  }
  else {

#ifdef LED_INDICATIONS
    digitalWrite(WIFI_COMM_OK_LED, LOW);     //WiFi not connected so turn OFF led
#endif

    Serial.println("WiFi not connected!");
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (serverClients[i]) serverClients[i].stop();
    }
    delay(1000);
  }
}
