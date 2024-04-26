#include "StatusLed.h"

StatusLed statusLed(PIN_NEOPIXEL, 1);

StatusLed::StatusLed(uint8_t pin, uint8_t numPixels) 
    : LED(numPixels, pin, NEO_GRB + NEO_KHZ800), currentStatus(Status::OK) {
        statusQueue = xQueueCreate(10, sizeof(Status));
        ledMutex = xSemaphoreCreateMutex();
    }

void StatusLed::begin() {
    LED.begin();
    LED.show();
}

void StatusLed::addStatus(Status status) {
    if (!isStatusInQueue(status)) { // Kontrola na duplikáty
        xQueueSendToFront(statusQueue, &status, portMAX_DELAY); // Prioritní vkládání
    }
}

void StatusLed::removeStatus(Status status) {
    Status current;
    QueueHandle_t tempQueue = xQueueCreate(10, sizeof(Status));

    while (xQueueReceive(statusQueue, &current, 0) == pdTRUE) {
        if (current != status) {
            xQueueSend(tempQueue, &current, portMAX_DELAY);
        }
    }

    while (xQueueReceive(tempQueue, &current, 0) == pdTRUE) {
        xQueueSend(statusQueue, &current, portMAX_DELAY);
    }

    vQueueDelete(tempQueue);
}

bool StatusLed::isStatusInQueue(Status status) {
    Status current;
    QueueHandle_t tempQueue = xQueueCreate(10, sizeof(Status));
    bool found = false;

    while (xQueueReceive(statusQueue, &current, 0) == pdTRUE) {
        if (current == status) {
            found = true; // Nalezen duplikátní stav
        }
        xQueueSend(tempQueue, &current, portMAX_DELAY); // Uložení stavů do dočasné fronty
    }

    // Při vkládání zpět kontrolujte, zda stav již ve frontě není
    while (xQueueReceive(tempQueue, &current, 0) == pdTRUE) {
        if (current != status || !found) {
            xQueueSend(statusQueue, &current, portMAX_DELAY); // Obnovení stavů v hlavní frontě pouze pokud nejsou duplikátní
        }
    }

    vQueueDelete(tempQueue);
    return found;
}

void StatusLed::updateLed() {
    Status highestStatus;
    bool isQueueNotEmpty = getHighestPriorityStatus(highestStatus);

    if (isQueueNotEmpty) {
        xSemaphoreTake(ledMutex, portMAX_DELAY); 
        switch (highestStatus) { 
            case Status::OK:
                blinkLed(LED.Color(0, 25, 0), 2000, 100);
                break;
            case Status::WARNING:
                blinkLed(LED.Color(25, 20, 0), 900, 900);
                break;
            case Status::ERROR:
                LED.setPixelColor(0, LED.Color(25, 0, 0));
                LED.show();
                break;
            case Status::WIFI_NOT_CONNECT:
                blinkLed(LED.Color(0, 0, 25), 250, 250);
                break;
            case Status::SD_CARD_DETECT:
                blinkLedCounted(LED.Color(255, 25, 217), 100, 3, Status::SD_CARD_DETECT);
                break;
            case Status::BLUETOOTH_CONNECT:
                blinkLedCounted(LED.Color(0, 0, 255), 100, 3, Status::BLUETOOTH_CONNECT);
                break;
            case Status::BLUETOOTH_COMUNICATION:
                blinkLedCounted(LED.Color(0, 255, 255), 70, 3, Status::BLUETOOTH_COMUNICATION);
                break;
            case Status::UPLOAD_FILE:
                blinkLed(LED.Color(25, 25, 0), 15, 15);
                break;
            case Status::FIRMWARE_DOWNLOAD:
                blinkLed(LED.Color(25, 0, 0), 15, 15);
                break;
        }
        xSemaphoreGive(ledMutex);
    }
}

bool StatusLed::getHighestPriorityStatus(Status& highestStatus) {
    Status currentStatus;
    bool isFound = false;
    int highestPriority = -1;

    QueueHandle_t tempQueue = xQueueCreate(10, sizeof(Status));

    while (xQueueReceive(statusQueue, &currentStatus, 0) == pdTRUE) {
        int priority = static_cast<int>(currentStatus);
        if (priority > highestPriority) {
            highestPriority = priority;
            highestStatus = currentStatus;
            isFound = true;
        }
        xQueueSend(tempQueue, &currentStatus, portMAX_DELAY);
    }

    while (xQueueReceive(tempQueue, &currentStatus, 0) == pdTRUE) {
        xQueueSend(statusQueue, &currentStatus, portMAX_DELAY);
    }

    vQueueDelete(tempQueue);
    return isFound;
}

void StatusLed::blinkLedCounted(uint32_t color, int delayTime, int count, Status status) {
    for (int i = 0; i < count; i++){
        LED.setPixelColor(0, color);
        LED.show();
        vTaskDelay(delayTime);
        LED.setPixelColor(0, 0, 0, 0); // vypnout LED
        LED.show();
        vTaskDelay(delayTime);
    }
    removeStatus(status);
}
 
void StatusLed::blinkLed(uint32_t color, int delayTime, int timeColor) {
    LED.setPixelColor(0, color);
    LED.show();
    vTaskDelay(timeColor);
    LED.setPixelColor(0, 0, 0, 0); // vypnout LED
    LED.show();
    vTaskDelay(delayTime);
}