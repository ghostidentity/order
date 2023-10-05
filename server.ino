#include <esp_now.h>
#include <WiFi.h>

typedef struct command {
    char state[10];
    uint32_t uniqueValue;
} command;


typedef struct request {
    char cmd[50];  
} request;


request input;
command client;

esp_now_peer_info_t peerInfo;
static const char* PMK_KEY_STR = "1234567890123456"; 
static const char* LMK_KEY_STR = "abcdefghabcdefgh"; 
bool sendOnce = false;
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    } 

    delay(1000);
    Serial.println("----------------------");
    Serial.println(WiFi.macAddress());
    Serial.println("----------------------");
    delay(1000);

    esp_err_t result = esp_now_set_pmk((uint8_t *)PMK_KEY_STR);
    
    if (result == ESP_OK) {
        Serial.println("PMK set with success");
    } else {
        Serial.println("Error setting the PMK");
    }

    memcpy(peerInfo.lmk, LMK_KEY_STR, 16 * sizeof(peerInfo.lmk[0]));
    peerInfo.encrypt = true;
    client.uniqueValue = 123456789;
    esp_now_register_send_cb(OnDataSent);
     esp_now_register_recv_cb(OnDataRecv); 
}
uint8_t mac[6];

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) 
{
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    memcpy(&input, incomingData , sizeof(input));
    Serial.println(String(macStr) + String("-") + String(input.cmd));
}

void loop() {
    if(Serial.available()) {
        String command = Serial.readStringUntil(',');
        String macStr = Serial.readStringUntil('\n');
        command.trim(); 
        macStr.trim();
        sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

          if (command == "REGISTER") {
              memcpy(peerInfo.peer_addr, mac, 6); 
              if (esp_now_add_peer(&peerInfo) != ESP_OK){
                  Serial.println("Failed to add peer");
              }
              else{
                  Serial.println("Peer added successfully");
              }
              return;
          }
        else if (command == "REMOVE") {
            if(esp_now_del_peer(mac) != ESP_OK){
                Serial.println("Failed to delete peer");
            }
            else{
                Serial.println("Peer deleted successfully");
            }
            return;
        } 
        // send status
        if ((command == "READY" || command == "STANDBY" || command == "PREPARING") && esp_now_is_peer_exist(mac)) {
            strncpy(client.state, command.c_str(), sizeof(client.state));
            esp_err_t result = esp_now_send(mac, (uint8_t *) &client, sizeof(client));
            if (result == ESP_OK) {
                Serial.println("Sent with success");
            } else {
                Serial.println("Error sending the data");
            }
        } else {
            Serial.println("Peer does not exist");
        }
    }
}
