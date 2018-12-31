#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

int FSR_PIN = 36; // Analog pin input for FSR on ARduino
int fsrValue = 0; // FSR values
int isSeated = 0; // 0 stand up, 1 seated
bool changeState = false; //only notify when pass from seated to stand up or vice versa

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
        char charBuf[5];
        String value = String(isSeated);
        value.toCharArray(charBuf, 5);
        pCharacteristic->setValue(charBuf);
    }
};

void setup() { 
  Serial.begin (115200);
  
  // Create the BLE Device
  BLEDevice::init("ESP32 UART Test"); // Give it a name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                      
  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_READ
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void checkFSR() {
  fsrValue = analogRead(FSR_PIN);
  if (fsrValue > 2000) {
    if (isSeated != 1) {
      isSeated = 1;
      changeState = true;
      Serial.println("seated");
    }
  } 
  else {
    if (isSeated != 0) {
      isSeated = 0;
      changeState = true;
      Serial.println("stand up");
    }
  }
}

void loop() {
  // get value from FSR sensor and set is Seated variable
  checkFSR();

  // if there is a device connected and the state has change, then notify to the device
  if (deviceConnected & changeState) {
    // Let's convert the value to a char array:
    char charBuf[5]; // make sure this is big enuffz
    dtostrf(isSeated, 1, 0, charBuf); // float_val, min_width, digits_after_decimal, char_buffer
    pCharacteristic->setValue(charBuf);
    pCharacteristic->notify();
  }
  delay(500);
}
