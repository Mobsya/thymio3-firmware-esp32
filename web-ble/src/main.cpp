#include <map>

#include <Arduino.h>
#include <M5StickCPlus.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <NimBLEDevice.h>
#define SERVICE_UUID     "12345678-1234-1234-1234-1234567890ab"
#define WRITE_CHAR_UUID  "12345678-1234-1234-1234-1234567890ac"
#define READ_CHAR_UUID   "12345678-1234-1234-1234-1234567890ad"
#define ACK_CHAR_UUID    "12345678-1234-1234-1234-1234567890ae"

BLECharacteristic* readCharacteristic;
BLECharacteristic* ackCharacteristic;
std::map<int, std::string> receivedChunks;
bool messageComplete = false;
uint32_t receivedChecksum = 0;
bool bluetoothConnected = false;

#define TASK_STACK_SIZE 3000
#define AVAILABLE = 0;
#define BUSY = 1;

unsigned long startMillis;
unsigned long currentMillis;
const unsigned long period = 10;

TaskHandle_t codeExecutionHandle = NULL;
void createAndStartTask(const char* stringToDisplay);
void stopTask();

uint32_t crc32(const std::string& data) {
  uint32_t crc = 0xFFFFFFFF;
  for (unsigned char c : data) {
    crc ^= c;
    for (int i = 0; i < 8; ++i)
      crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
  }
  return ~crc;
}

void processCompleteMessage() {
  std::string fullMessage;
  for (auto const& entry : receivedChunks) {
    fullMessage += entry.second;
  }

  uint32_t calcCrc = crc32(fullMessage);

  Serial.println("===== FULL MESSAGE RECEIVED =====");
  Serial.println(fullMessage.c_str());
  Serial.printf("Received CRC: %08X, Calculated CRC: %08X\n", receivedChecksum, calcCrc);

  if (calcCrc == receivedChecksum) {
    Serial.println("✅ CRC MATCH: Message is valid.");
    //std::string notify = "MSG_RECEIVED:" + String(calcCrc, HEX).c_str();
    std::string notify = "MSG_RECEIVED:" + std::to_string(calcCrc);
    Serial.println(notify.c_str());
    ackCharacteristic->notify(notify);

    //readCharacteristic->setValue(fullMessage.c_str());
  } else {
    Serial.println("❌ CRC MISMATCH: Message corrupted.");
    ackCharacteristic->setValue("MSG_CORRUPTED");
    ackCharacteristic->notify();
  }

  receivedChunks.clear();
  messageComplete = false;
}

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer, BLEConnInfo& connInfo) override {
    Serial.println("🔗 Client connected");
    bluetoothConnected = true;
  }

  void onDisconnect(BLEServer* pServer, BLEConnInfo& connInfo, int reason) override {
    Serial.println("❌ Client disconnected. Restarting advertising...");
    bluetoothConnected = false;

    // Restart advertising
    pServer->startAdvertising();
  }
};

class WriteCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    std::string value = pCharacteristic->getValue();
    Serial.println("Received chunk: " + String(value.c_str()));

    int sepIndex = value.find('|');
    if (sepIndex > 0) {
      int seq = atoi(value.substr(0, sepIndex).c_str());
      std::string data = value.substr(sepIndex + 1);

      bool isFinal = data.find("<END>:") != std::string::npos;
      if (isFinal) {
        size_t delimiterPos = data.find("<END>:");
        std::string dataPart = data.substr(0, delimiterPos);
        std::string crcStr = data.substr(delimiterPos + 6);
        receivedChunks[seq] = dataPart;

        receivedChecksum = strtoul(crcStr.c_str(), NULL, 16);
        messageComplete = true;
      } else {
        receivedChunks[seq] = data;
      }

      std::string ack = "ACK:" + std::to_string(seq);
      Serial.println(ack.c_str());
      ackCharacteristic->notify(ack);

      if (messageComplete) {
        processCompleteMessage();
      }
    }
  }
};

class ReadValueCallback : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    std::string value = String(rand()).c_str();
    readCharacteristic->setValue(value);
  }
};

void setup() {
  M5.begin();
  Serial.begin(115200);
  delay(10);

  startMillis = millis();

  BLEDevice::init("ESP32-BLE-Server");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *writeCharacteristic = pService->createCharacteristic(
    WRITE_CHAR_UUID,
    NIMBLE_PROPERTY::WRITE
  );
  writeCharacteristic->setCallbacks(new WriteCallbacks());

  readCharacteristic = pService->createCharacteristic(
    READ_CHAR_UUID,
    NIMBLE_PROPERTY::READ
  );
  readCharacteristic->setValue("ESP32 ready");
  readCharacteristic->setCallbacks(new ReadValueCallback());

  ackCharacteristic = pService->createCharacteristic(
    ACK_CHAR_UUID,
    NIMBLE_PROPERTY::NOTIFY
  );

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setName("ESP32-BLE-Server");
  pAdvertising->start();
  Serial.println("Advertising...");
}

void loop() {
  currentMillis = millis();
}

void print_word_task(void* arg) {
  const char* paramString = static_cast<const char*>(arg);

  while(true) {
    printf("\n %s running...", paramString);
    vTaskDelay(1000/portTICK_RATE_MS);
  }
}

void createAndStartTask(const char* stringToExecute) {
  M5.Lcd.println("Creating task...");
  printf("Allocating...");

  const char* allocatedStringToExecute = (const char*) malloc(1000);
  if(allocatedStringToExecute != NULL) {
    printf("Copying...");
    strcpy((char*)allocatedStringToExecute, stringToExecute);

    if(codeExecutionHandle != NULL && eTaskGetState(codeExecutionHandle) != eDeleted) {
      stopTask();
    }

    printf("Creating task...");
    xTaskCreatePinnedToCore(print_word_task, "print_word_task", TASK_STACK_SIZE, (void*)allocatedStringToExecute, 1, &codeExecutionHandle, 1);
  } else {
    printf("Memory allocation failed !");
  }
}

void stopTask() {
  if(codeExecutionHandle != NULL) {
    M5.Lcd.println("Stopping execution...");
    vTaskSuspend(codeExecutionHandle);
    vTaskDelete(codeExecutionHandle);
    codeExecutionHandle = NULL;
  }
}

