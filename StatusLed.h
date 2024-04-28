#ifndef StatusLed_h
#define StatusLed_h

//připojení knihoven
#include <Arduino.h> 
#include "Adafruit_NeoPixel.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "Logger.h"

class StatusLed {
public:
    enum class Status {
        OK = 0,                         // nejnižší priorita
        WIFI_NOT_CONNECT = 1, 
        WARNING = 2,            
        ERROR = 3,                  // vyšší priorita 
        SD_CARD_DETECT = 4,
        BLUETOOTH_COMUNICATION = 5,
        BLUETOOTH_CONNECT = 6,
        UPLOAD_FILE = 7,     
        FIRMWARE_DOWNLOAD = 8,      // nejvyšší priorita
    };

    StatusLed(uint8_t pin, uint8_t numPixels);
    void begin();
    void setStatus(Status status);
    void updateLed();
    void addStatus(Status status); // Přidá nový status do fronty
    void removeStatus(Status status); // Odstraní status z fronty

private:
    Adafruit_NeoPixel LED;
    Status currentStatus;
    void blinkLedCounted(uint32_t color, int delayTime, int count, Status status);
    void blinkLed(uint32_t color, int delayTime, int timeColor);
    SemaphoreHandle_t ledMutex;
    QueueHandle_t statusQueue; // Fronta pro uchování statusů
    bool isStatusInQueue(Status status); // Kontroluje, zda je status již ve frontě
    bool getHighestPriorityStatus(Status& highestStatus);
};

#endif // StatusLed_h
