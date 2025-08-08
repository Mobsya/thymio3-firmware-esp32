#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <map>

/*
#define SERVICE_UUID     "12345678-1234-1234-1234-1234567890ab"
#define WRITE_CHAR_UUID  "12345678-1234-1234-1234-1234567890ac"
#define READ_CHAR_UUID   "12345678-1234-1234-1234-1234567890ad"
#define ACK_CHAR_UUID    "12345678-1234-1234-1234-1234567890ae"

BLECharacteristic* readCharacteristic;
BLECharacteristic* ackCharacteristic;
std::map<int, std::string> receivedChunks;
bool messageComplete = false;
uint32_t receivedChecksum = 0;

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
    std::string notify = "MSG_RECEIVED:" + String(calcCrc, HEX).c_str();
    ackCharacteristic->setValue(notify);
    ackCharacteristic->notify();

    readCharacteristic->setValue(fullMessage.c_str());
  } else {
    Serial.println("❌ CRC MISMATCH: Message corrupted.");
    ackCharacteristic->setValue("MSG_CORRUPTED");
    ackCharacteristic->notify();
  }

  receivedChunks.clear();
  messageComplete = false;
}

class WriteCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
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
      ackCharacteristic->setValue(ack);
      ackCharacteristic->notify();

      if (messageComplete) {
        processCompleteMessage();
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("ESP32-BLE-Server");
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *writeCharacteristic = pService->createCharacteristic(
    WRITE_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  writeCharacteristic->setCallbacks(new WriteCallbacks());

  readCharacteristic = pService->createCharacteristic(
    READ_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ
  );
  readCharacteristic->setValue("ESP32 ready");

  ackCharacteristic = pService->createCharacteristic(
    ACK_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pService->start();
  BLEDevice::getAdvertising()->start();
}

void loop() {
  // Optionally update the read characteristic periodically here
}
*/
