#include "config.h"

// Check if Bluetooth configs are enabled
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

//OWN ADDINGS!!!-------------
uint32_t last = 0;
uint32_t updateTimeout = 0;

uint32_t sessionStartTime = 0;
uint32_t sessionDuration = 0;
//---------------------------

// Bluetooth Serial object
BluetoothSerial SerialBT;

// Watch objects
TTGOClass *watch;
TFT_eSPI *tft;
BMA *sensor;

uint32_t sessionId = 30;

volatile uint8_t state;
volatile bool irqBMA = false;
volatile bool irqButton = false;

bool sessionStored = false;
bool sessionSent = false;

void initHikeWatch()
{
    // LittleFS
    if(!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LITTLEFS Mount Failed");
        return;
    }

    // Stepcounter
    // Configure IMU
    // Enable BMA423 step count feature
    // Reset steps
    // Turn on step interrupt
    if (!watch->bma->begin()) {
    Serial.println("BMA423 init failed");
    return;
    }
    watch->bma->enableAccel();
    watch->bma->enableFeature(BMA423_STEP_CNTR, true);
    watch->bma->resetStepCounter();
    //watch->bma->enableIrq(BMA423_STEP_CNTR_INT);
    watch->bma->enableStepCountInterrupt();


    // Side button
    pinMode(AXP202_INT, INPUT_PULLUP);
    attachInterrupt(AXP202_INT, [] {
        irqButton = true;
    }, FALLING);

    //!Clear IRQ unprocessed first
    watch->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);
    watch->power->clearIRQ();

    return;
}

void sendDataBT(fs::FS &fs, const char * path)
{
    /* Sends data via SerialBT */
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }
    Serial.println("- read from file:");
    while(file.available()){
        SerialBT.write(file.read());
    }
    file.close();
}

void sendSessionBT()
{
    // Read session and send it via SerialBT
    watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
    watch->tft->drawString("Sending session", 20, 80);
    watch->tft->drawString("to Hub", 80, 110);

    // Sending session id
    sendDataBT(LITTLEFS, "/id.txt");
    SerialBT.write(';');
    // Sending steps
    sendDataBT(LITTLEFS, "/steps.txt");
    SerialBT.write(';');
    // Sending distance
    sendDataBT(LITTLEFS, "/distance.txt");
    SerialBT.write(';');
    // Sending duration
    sendDataBT(LITTLEFS, "/time.txt");
    SerialBT.write(';');
    // Send connection termination char
    SerialBT.write('\n');
}


void saveIdToFile(uint16_t id) //UNUSED
{
    char buffer[10];
    itoa(id, buffer, 10);
    writeFile(LITTLEFS, "/id.txt", buffer);
}

void saveStepsToFile(uint32_t step_count) //UNUSED
{
    char buffer[10];
    itoa(step_count, buffer, 10);
    writeFile(LITTLEFS, "/steps.txt", buffer);
}

void saveDistanceToFile(float distance) //UNUSED ????
{
    char buffer[10];
    itoa(distance, buffer, 10);
    writeFile(LITTLEFS, "/distance.txt", buffer);
}

//OWN ADDING
//void saveTimeToFile(int hours, int minutes, int seconds)
// {
//     char buffer[20];
//     snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hours, minutes, seconds);
//     writeFile(LITTLEFS, "/time.txt", buffer);
// }
void saveTimeToFile(uint32_t seconds)
{
    char buffer[20];
    itoa(seconds, buffer, 20);
    writeFile(LITTLEFS, "/time.txt", buffer);
}
//OWN ADDING END

void deleteSession()
{
    deleteFile(LITTLEFS, "/id.txt");
    deleteFile(LITTLEFS, "/distance.txt");
    deleteFile(LITTLEFS, "/steps.txt");
    deleteFile(LITTLEFS, "/coord.txt");
    deleteFile(LITTLEFS, "/time.txt");

}

void setup()
{
    Serial.begin(115200);
    watch = TTGOClass::getWatch();
    watch->begin();
    watch->openBL();

    //Receive objects for easy writing
    tft = watch->tft;
    sensor = watch->bma;
    
    initHikeWatch();

    state = 1;

    SerialBT.begin("Hiking Watch");
}

void loop()
{
    switch (state)
    {
    case 1:
    {
        /* Initial stage */
        //Basic interface
        watch->tft->fillScreen(TFT_BLACK);
        watch->tft->setTextFont(4);
        watch->tft->setTextColor(TFT_WHITE, TFT_BLACK);
        watch->tft->drawString("Hiking Watch",  45, 25, 4);
        watch->tft->drawString("Press button", 50, 80);
        watch->tft->drawString("to start session", 40, 110);

        bool exitSync = false;

        //Bluetooth discovery
        while (1)
        {
            /* Bluetooth sync */
            if (SerialBT.available())
            {
                char incomingChar = SerialBT.read();
                if (incomingChar == 'c' and sessionStored and not sessionSent)
                {
                    sendSessionBT();
                    sessionSent = true;
                }

                if (sessionSent && sessionStored) {
                    // Update timeout before blocking while
                    updateTimeout = 0;
                    last = millis();
                    while(1)
                    {
                        updateTimeout = millis();

                        if (SerialBT.available())
                            incomingChar = SerialBT.read();
                        if (incomingChar == 'r')
                        {
                            Serial.println("Got an R");
                            // Delete session
                            deleteSession();
                            sessionStored = false;
                            sessionSent = false;
                            incomingChar = 'q';
                            exitSync = true;
                            break;
                        }
                        else if ((millis() - updateTimeout > 2000))
                        {
                            Serial.println("Waiting for timeout to expire");
                            updateTimeout = millis();
                            sessionSent = false;
                            exitSync = true;
                            break;
                        }
                    }
                }
            }
            if (exitSync)
            {
                delay(1000);
                watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
                watch->tft->drawString("Hiking Watch",  45, 25, 4);
                watch->tft->drawString("Press button", 50, 80);
                watch->tft->drawString("to start session", 40, 110);
                exitSync = false;
            }

            /*      IRQ     */
            if (irqButton) {
                irqButton = false;
                watch->power->readIRQ();
                if (state == 1)
                {
                    state = 2;
                }
                watch->power->clearIRQ();
            }
            if (state == 2) {
                if (sessionStored)
                {
                    watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
                    watch->tft->drawString("Overwriting",  55, 100, 4);
                    watch->tft->drawString("session", 70, 130);
                    delay(1000);
                }
                break;
            }
        }
        break;
    }
    case 2:
    {
        /* Hiking session initalisation */ 
        
        //reset step-counter
        sensor->resetStepCounter();

        watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
        watch->tft->drawString("Starting hike", 45, 100);
        sessionStartTime = millis();
        delay(1000);
        watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
        state = 3;
        break;
    }
    case 3:
    {
        /* Hiking session ongoing */
        delay(1000);
        watch->tft->setCursor(25, 70);
        watch->tft->print(String("Steps: ") + sensor->getCounter());

        watch->tft->setCursor(25, 100);
        watch->tft->print(String("Dist: ") + sensor->getCounter() * 0.0008 + " km");

        last = millis();
        updateTimeout = 0;

        sessionDuration = millis() - sessionStartTime;
        uint32_t totalSeconds = sessionDuration / 1000;
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;
        watch->tft->setCursor(25, 130);
        //watch->tft->print(String("Duration: ") + hours + ":" + minutes + ":" + seconds);
        watch->tft->print(
            String("Duration: ") +
            (hours < 10 ? "0" : "") + String(hours) + ":" +
            (minutes < 10 ? "0" : "") + String(minutes) + ":" +
            (seconds < 10 ? "0" : "") + String(seconds)
            );

        if (irqButton) {
                irqButton = false;
                state = 4;
                watch->power->readIRQ();
                watch->power->clearIRQ();


        }
        break;
    }
    case 4:
    {
        //Save hiking session data
        delay(1000);

        // OWN ADDINGS-------------
        saveIdToFile(sessionId);
        saveStepsToFile(sensor->getCounter());
        saveDistanceToFile(sensor->getCounter() * 0.0008); // /(km)

        //time
        sessionDuration = millis() - sessionStartTime;
        uint32_t seconds = sessionDuration / 1000;
        saveTimeToFile(seconds);

        sessionStored = true;
        //OWN ADDINGS END----------
        state = 1;  
        break;
    }
    default:
        // Restart watch
        ESP.restart();
        break;
    }
}