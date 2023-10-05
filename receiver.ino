#include <esp_now.h>
#include <WiFi.h>

typedef struct command {
    char cmd[50];  
} command;


command myData;
esp_now_peer_info_t peerInfo;

const int ledPinReady = 12;
const int ledPinStandby = 13;
const int ledPinPreparing = 14;

const int triggerPin1 = 15;
const int triggerPin2 = 4;

uint8_t masterMacAddress[] = {0xE0, 0x5A, 0x1B, 0x6C, 0x41, 0x90};
static const char* PMK_KEY_STR = "1234567890123456"; 
static const char* LMK_KEY_STR = "abcdefghabcdefgh"; 

unsigned long lastTimeMessageSent = 0;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    memcpy(&myData, incomingData, sizeof(myData));
    Serial.print("Command: ");
    Serial.println(myData.cmd);
    
    if (strcmp(myData.cmd, "READY") == 0) {
        digitalWrite(ledPinReady, HIGH);
        digitalWrite(ledPinStandby, LOW);
        digitalWrite(ledPinPreparing, LOW);
    } 
    else if (strcmp(myData.cmd, "STANDBY") == 0) {
        digitalWrite(ledPinReady, LOW);
        digitalWrite(ledPinStandby, HIGH);
        digitalWrite(ledPinPreparing, LOW);
    } 
    else if (strcmp(myData.cmd, "PREPARING") == 0) {
        digitalWrite(ledPinReady, LOW);
        digitalWrite(ledPinStandby, LOW);
        digitalWrite(ledPinPreparing, HIGH);
    }
}

void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);

    delay(1000);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_err_t result = esp_now_set_pmk((uint8_t *)PMK_KEY_STR);

    if (result == ESP_OK) {
        Serial.println("PMK set with success");
    } else {
        Serial.println("Error setting the PMK");
    }

    memcpy(peerInfo.peer_addr, masterMacAddress, 6);
    memcpy(peerInfo.lmk, LMK_KEY_STR, 16 * sizeof(peerInfo.lmk[0]));
    peerInfo.encrypt = true;

    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("Failed to add peer");
        return;
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv); 

    pinMode(ledPinReady, OUTPUT);
    pinMode(ledPinStandby, OUTPUT);
    pinMode(ledPinPreparing, OUTPUT);

    pinMode(triggerPin1, INPUT);
    pinMode(triggerPin2, INPUT);
}

void loop() {
    unsigned long currentTime = millis();

    int trigger1State = digitalRead(triggerPin1);
    int trigger2State = digitalRead(triggerPin2);

    if((trigger1State == HIGH || trigger2State == HIGH) && currentTime - lastTimeMessageSent > 10000)
    {
        if (trigger1State == HIGH)
            strcpy(myData.cmd, "1");
        else if (trigger2State == HIGH)
            strcpy(myData.cmd, "2");

        esp_now_send(masterMacAddress, (uint8_t*)&myData, sizeof(myData));
        lastTimeMessageSent = currentTime;
    }

    delay(5000);
}
