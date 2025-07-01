#include <Arduino.h>

#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_bt.h"
#include <sys/time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

bool DEBUG_MASTER_ENABLE = true;
bool DEBUG_ANIMATOR = true;
bool DEBUG_GAME_STATS = true;
bool DEBUG_DIALOG_BOX = true;
bool DEBUG_PATH_GENERATOR = true;
bool DEBUG_CHARACTER_MANAGER = true;
bool DEBUG_SCENES = true;
bool DEBUG_WIFI_MANAGER = true;
bool DEBUG_BLUETOOTH = true;
bool DEBUG_SLEEP_CONTROLLER = true;
bool DEBUG_HARDWARE_INPUT = true;
bool DEBUG_SYSTEM = true;
bool DEBUG_EFFECTS_MANAGER = true;
bool DEBUG_WEATHER = true;
bool DEBUG_PARTICLE_SYSTEM = true;
bool DEBUG_TASK = true;
bool DEBUG_DEEP_SLEEP = true;
bool DEBUG_U8G2_WEBSTREAM = true;

#include "DebugUtils.h"


#include "EDGE.h"
#include "System/GameContext.h"


#include "Scenes/SceneBoot/BootScene.h"
#include "Scenes/SceneMain/MainScene.h"
#include "Scenes/SceneAction/SceneAction.h"
#include "Scenes/SceneSleeping/SleepingScene.h"
#include "Scenes/SceneStats/StatsScene.h"
#include "Scenes/SceneMenuParameters/MenuParametersScene.h"
#include "Scenes/Games/GameScene.h"
#include "Scenes/Games/FlappyTuck/FlappyTuckScene.h"
#include "Scenes/ScenePlayMenu/PlayMenuScene.h"
#include "Scenes/Prequel/_0LanguageSelectScene.h"
#include "Scenes/Prequel/_1AwakeningSparkScene.h"
#include "Scenes/Prequel/_2CellularConglomerationScene.h"
#include "Scenes/Prequel/_3JourneyWithinScene.h"
#include "Scenes/Prequel/_4ShellWeavingScene.h"


#include "DialogBox/DialogBox.h"
#include "System/DeepSleepController.h"
#include "Scenes/Prequel/PrequelManager.h"
#include "System/PeriodicTaskManager.h"
#include "HardwareInputController.h"

#include "DisplayConfig.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "espasyncbutton.hpp"
#include "WiFiManager.h"
#include <WiFi.h>
#include "BluetoothManager.h"
#include "SerialCommandHandler.h"
#include <Bluepad32.h>
#include "GEM_u8g2.h"
#include "MycilaWebSerial.h"
#include <SerialForwarder.h>
#include "System/ScreenStreamer.h"


#include "GameStats.h"
#include "Weather/WeatherManager.h"
#include "character/CharacterManager.h"
#include "GlobalMappings.h"
#include "Helper/PathGenerator.h"

Language currentLanguage = Language::ENGLISH;
GameContext gameContext;

AsyncWebServer *server_ptr = nullptr;
WebSerial *webSerial_ptr = nullptr;
Preferences *preferences_ptr = nullptr;
GameStats *gameStats_ptr = nullptr;
WiFiManager *wifiManager_ptr = nullptr;
BluetoothManager *bluetoothManager_ptr = nullptr;
SerialCommandHandler *commandHandler_ptr = nullptr;
SerialForwarder *forwardedSerial_ptr = nullptr;
EDGE *engine = nullptr;
GPIOButton<ESPEventPolicy> *globalButtonOk_ptr = nullptr;
GPIOButton<ESPEventPolicy> *physicalButtonUp_ptr = nullptr;
GPIOButton<ESPEventPolicy> *physicalButtonDown_ptr = nullptr;
WeatherManager *weatherManager_ptr = nullptr;
CharacterManager *characterManager_ptr = nullptr;
DeepSleepController *deepSleepController_ptr = nullptr;
PrequelManager *prequelManager_ptr = nullptr;
PeriodicTaskManager *periodicTaskManager_ptr = nullptr;
HardwareInputController *hardwareInputController_ptr = nullptr;
PathGenerator *pathGenerator_ptr = nullptr;
ScreenStreamer* screenStreamer_ptr = nullptr;

extern Bluepad32 BP32;

const gpio_num_t VIRTUAL_BTN_OK = GPIO_NUM_0;
const gpio_num_t VIRTUAL_BTN_LEFT = (gpio_num_t)1;
const gpio_num_t VIRTUAL_BTN_RIGHT = (gpio_num_t)2;
const gpio_num_t WAKEUP_BUTTON_PIN = GPIO_NUM_0;

const gpio_num_t PHYSICAL_UP_BUTTON_PIN = (gpio_num_t)34;
const gpio_num_t PHYSICAL_DOWN_BUTTON_PIN = (gpio_num_t)35;

const char *WIFI_SSID = "testSSID";
const char *WIFI_PASSWORD = "testPW";
const char *OTA_PASSWORD = ""; // Set your OTA password here. Use "" or nullptr for no password.

#define I2C_SDA 5
#define I2C_SCL 4
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

using MyDisplayDriver = U8G2_SSD1306_128X64_NONAME_F_HW_I2C;
MyDisplayDriver* u8g2 = nullptr;

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

unsigned long loopCounter = 0;
unsigned long startTime = 0;
const int TICK_SPEED = 30;
const unsigned long TICK_INTERVAL_MICROS = 1000000 / TICK_SPEED;
unsigned long previousTickTime = 0;
unsigned long tickCounter = 0;
unsigned long lastActivityTime = 0;

const unsigned long ONE_MINUTE_MILLIS = 60000UL;
const unsigned long INACTIVITY_TIMEOUT_MILLIS = 2 * ONE_MINUTE_MILLIS;

RTC_DATA_ATTR uint32_t rtc_sleep_entry_epoch_sec = 0;
RTC_DATA_ATTR bool wokeFromDeepSleep = false;
RTC_DATA_ATTR int rtc_forced_sleep_duration_minutes = 0;
RTC_DATA_ATTR bool rtc_was_forced_sleep_with_duration = false;

#define KEY_QUEUE_LENGTH 10
#define KEY_QUEUE_ITEM_SIZE sizeof(uint8_t)
QueueHandle_t keyQueue = NULL;
void updateLastActivityTime();
void setup();
void loop();

void updateLastActivityTime()
{
    lastActivityTime = millis();
}

Scene *createBootScene_Factory(GameContext &context, void *configData)
{
    BootScene *scene = new BootScene();
    if (scene) scene->init(context);
    if (scene && configData)
    {
        scene->setConfig(*(static_cast<BootSceneConfig *>(configData)));
    }
    else if (scene)
    {
        BootSceneConfig defaultConfig;
        scene->setConfig(defaultConfig);
    }
    return scene;
}
Scene *createMainScene_Factory(GameContext &context, void *configData) { 
    MainScene* scene = new MainScene();
    if (scene) scene->init(context);
    return scene;
}
Scene *createStatsScene_Factory(GameContext &context, void *configData) { 
    StatsScene* scene = new StatsScene();
    if (scene) scene->init(context);
    return scene;
}
Scene *createParamsScene_Factory(GameContext &context, void *configData)
{
    MenuParametersScene *scene = new MenuParametersScene();
    if (scene) scene->init(context);
    return scene;
}
Scene *createFlappyGameScene_Factory(GameContext &context, void *configData) { 
    FlappyTuckScene* scene = new FlappyTuckScene();
    if (scene) scene->init(context);
    return scene;
}
Scene *createActionScene_Factory(GameContext &context, void *configData) { 
    SceneAction* scene = new SceneAction();
    if (scene) scene->init(context);
    return scene;
}
Scene *createSleepingScene_Factory(GameContext &context, void *configData) { 
    SleepingScene* scene = new SleepingScene();
    if (scene) scene->init(context);
    return scene;
}
Scene *createPlayMenuScene_Factory(GameContext &context, void *configData) { 
    PlayMenuScene* scene = new PlayMenuScene();
    if (scene) scene->init(context);
    return scene;
}
Scene *createLangSelectPrequelScene_Factory(GameContext &context, void *configData) { 
    _0LanguageSelectScene* scene = new _0LanguageSelectScene();
    if (scene) scene->init(context);
    return scene;
}
Scene *createPrequel1Scene_Factory(GameContext &context, void *configData) { 
    _1AwakeningSparkScene* scene = new _1AwakeningSparkScene();
    if (scene) scene->init(context);
    return scene;
}
Scene *createPrequel2Scene_Factory(GameContext &context, void *configData) { 
    _2CellularConglomerationScene* scene = new _2CellularConglomerationScene();
    if (scene) scene->init(context);
    return scene;
}
Scene *createPrequel3Scene_Factory(GameContext &context, void *configData) { 
    _3JourneyWithinScene* scene = new _3JourneyWithinScene();
    if (scene) scene->init(context);
    return scene;
}
Scene *createPrequel4Scene_Factory(GameContext &context, void *configData) { 
    _4ShellWeavingScene* scene = new _4ShellWeavingScene();
    if (scene) scene->init(context);
    return scene;
}

void setup()
{


    Serial.begin(115200UL, 134217756U, 3, 1);
    delay(100);
    Serial.println("\nStarting TamaBouchi! (HW Serial)");
    Serial.println("\nFree heap : " + String(ESP.getFreeHeap()));
    Serial.println("Free min heap : " + String(ESP.getMinFreeHeap()));

    setCpuFrequencyMhz(80);

    Serial.println("\nLoading preferences ...");
    preferences_ptr = new Preferences();
    gameContext.preferences = preferences_ptr;

    Serial.println("\nLoading GameStats ...");
    gameStats_ptr = new GameStats();
    gameContext.gameStats = gameStats_ptr;

    Serial.println("\nLoading Server_PTR ...");
    server_ptr = new AsyncWebServer(80);

    Serial.println("\nInitializing WebSerial ...");
    webSerial_ptr = new WebSerial();

    Serial.println("\nInitializing SerialForwarder ...");
    forwardedSerial_ptr = new SerialForwarder(webSerial_ptr, &Serial);
    gameContext.serialForwarder = forwardedSerial_ptr;

    Serial.flush();
    delay(100);

    if (forwardedSerial_ptr)
    {
        forwardedSerial_ptr->enableWebSerial(true);
        Serial.println("\n[Main] Hardware Serial test after SerialForwarder initialization");
    }
    debugPrint("SYSTEM", "Initializing WiFiManager...");
    wifiManager_ptr = new WiFiManager(*preferences_ptr);
    gameContext.wifiManager = wifiManager_ptr;

    debugPrint("SYSTEM", "Initializing bluetoothManager_ptr...");
    bluetoothManager_ptr = new BluetoothManager();
    gameContext.bluetoothManager = bluetoothManager_ptr;

    debugPrint("SYSTEM", "Initializing hardware buttons...");
    globalButtonOk_ptr = new GPIOButton<ESPEventPolicy>(GPIO_NUM_0, LOW);
    physicalButtonUp_ptr = new GPIOButton<ESPEventPolicy>(PHYSICAL_UP_BUTTON_PIN, LOW);
    physicalButtonDown_ptr = new GPIOButton<ESPEventPolicy>(PHYSICAL_DOWN_BUTTON_PIN, LOW);

    debugPrint("SYSTEM", "Initializing display and engine...");
    DisplayConfig displayConf(SSD1306, I2C_SCL, I2C_SDA, U8X8_PIN_NONE, U8G2_R2, SCREEN_WIDTH, SCREEN_HEIGHT, true);

    u8g2 = new MyDisplayDriver(
        displayConf.rotation,
        displayConf.resetPin,
        displayConf.clockPin,
        displayConf.dataPin
    );
    u8g2->begin();


    screenStreamer_ptr = new ScreenStreamer(u8g2, server_ptr, true);
    screenStreamer_ptr->init();

    gameContext.display = u8g2;
    gameContext.defaultFont = u8g2_font_5x7_tf;

    EDGELogger engineLogger = [](const char* message) {
        if (forwardedSerial_ptr) {
            forwardedSerial_ptr->println(message);
        }
    };
    engine = new EDGE(u8g2, displayConf, engineLogger);

    // ******************* CRASH FIX ******************* //
    // Populate the game context with pointers from the engine
    gameContext.renderer = &engine->getRenderer();
    gameContext.sceneManager = &engine->getSceneManager();
    gameContext.inputManager = &engine->getInputManager();
    // *********************************************** //


    debugPrint("SYSTEM", "Initializing character manager...");
    characterManager_ptr = CharacterManager::getInstance();
    gameContext.characterManager = characterManager_ptr;

    debugPrint("SYSTEM", "Initializing PathGenerator...");
    pathGenerator_ptr = new PathGenerator();
    gameContext.pathGenerator = pathGenerator_ptr;

    debugPrint("SYSTEM", "Initializing deep sleep controller...");
    deepSleepController_ptr = new DeepSleepController(u8g2);
    gameContext.deepSleepController = deepSleepController_ptr;

    debugPrint("SYSTEM", "Initializing prequel manager...");
    prequelManager_ptr = new PrequelManager(gameContext);
    gameContext.prequelManager = prequelManager_ptr;

    debugPrint("SYSTEM", "Initializing periodic task manager...");
    periodicTaskManager_ptr = new PeriodicTaskManager(gameContext);
    gameContext.periodicTaskManager = periodicTaskManager_ptr;

    debugPrint("SYSTEM", "Initializing hardware input controller...");
    hardwareInputController_ptr = new HardwareInputController();
    gameContext.hardwareInputController = hardwareInputController_ptr;

    debugPrint("SYSTEM", "Initializing WeatherManager...");
    weatherManager_ptr = new WeatherManager(gameContext);
    gameContext.weatherManager = weatherManager_ptr;

    if (!preferences_ptr || !gameStats_ptr || !server_ptr || !webSerial_ptr || !forwardedSerial_ptr || !wifiManager_ptr || !bluetoothManager_ptr || !globalButtonOk_ptr || !physicalButtonUp_ptr || !physicalButtonDown_ptr || !u8g2 || !engine || !characterManager_ptr || !deepSleepController_ptr || !prequelManager_ptr || !periodicTaskManager_ptr || !hardwareInputController_ptr || !weatherManager_ptr || !pathGenerator_ptr || !screenStreamer_ptr)
    {
        Serial.println("!!! FATAL: Core object allocation failed! Halting.");
        while (1)
            ;
    }
    debugPrint("SYSTEM", "Core objects allocated/retrieved.");

    gameStats_ptr->load();
    if (gameStats_ptr->selectedLanguage != LANGUAGE_UNINITIALIZED)
    {
        currentLanguage = gameStats_ptr->selectedLanguage;
    }
    else
    {
        currentLanguage = Language::ENGLISH;
    }

    characterManager_ptr->init(gameStats_ptr);
    debugPrint("SYSTEM", "Initial game stats loaded & CharacterManager initialized.");

    commandHandler_ptr = new SerialCommandHandler(gameContext);
    gameContext.commandHandler = commandHandler_ptr;

    if (!commandHandler_ptr)
    {
        Serial.println("!!! FATAL: Command Handler allocation failed! Halting.");
        while (1)
            ;
    }

    keyQueue = xQueueCreate(KEY_QUEUE_LENGTH, KEY_QUEUE_ITEM_SIZE);
    if (keyQueue == NULL)
    {
        debugPrint("SYSTEM", "!!! ERROR Creating Key Queue !!!");
    }
    else
    {
        debugPrint("SYSTEM", "Key Queue created successfully.");
    }

    gameContext.lastWakeUpInfo = deepSleepController_ptr->processWakeUp(gameStats_ptr);
    debugPrintf("SLEEP_CONTROLLER", "Main setup: initialBootWasDeepSleepWake=%d, initialDeepSleepMinutesPassed=%d",
                gameContext.lastWakeUpInfo.wasWakeUpFromDeepSleep, gameContext.lastWakeUpInfo.sleepDurationMinutes);

    periodicTaskManager_ptr->init();
    lastActivityTime = millis();

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    debugPrint("SYSTEM", "Default event loop created.");
    if (u8g2)
    {
        u8g2->setPowerSave(0);
        debugPrint("SYSTEM", "Display power save OFF.");
    }

    engine->init();

    hardwareInputController_ptr->init(&gameContext, bluetoothManager_ptr, myControllers, &updateLastActivityTime);

    // SceneManager is now accessed via the context, which is guaranteed to be valid here
    gameContext.sceneManager->registerScene("BOOT", [&](void* configData) { return createBootScene_Factory(gameContext, configData); });
    gameContext.sceneManager->registerScene("MAIN", [&](void* configData) { return createMainScene_Factory(gameContext, configData); });
    gameContext.sceneManager->registerScene("STATS", [&](void* configData) { return createStatsScene_Factory(gameContext, configData); });
    gameContext.sceneManager->registerScene("PARAMS", [&](void* configData) { return createParamsScene_Factory(gameContext, configData); });
    gameContext.sceneManager->registerScene("FLAPPY_GAME", [&](void* configData) { return createFlappyGameScene_Factory(gameContext, configData); });
    gameContext.sceneManager->registerScene("ACTIONS", [&](void* configData) { return createActionScene_Factory(gameContext, configData); });
    gameContext.sceneManager->registerScene("SLEEPING", [&](void* configData) { return createSleepingScene_Factory(gameContext, configData); });
    gameContext.sceneManager->registerScene("PLAY_MENU", [&](void* configData) { return createPlayMenuScene_Factory(gameContext, configData); });
    gameContext.sceneManager->registerScene("LANGUAGE_SELECT_PREQUEL", [&](void* configData) { return createLangSelectPrequelScene_Factory(gameContext, configData); });
    gameContext.sceneManager->registerScene("PREQUEL_STAGE_1", [&](void* configData) { return createPrequel1Scene_Factory(gameContext, configData); });
    gameContext.sceneManager->registerScene("PREQUEL_STAGE_2", [&](void* configData) { return createPrequel2Scene_Factory(gameContext, configData); });
    gameContext.sceneManager->registerScene("PREQUEL_STAGE_3", [&](void* configData) { return createPrequel3Scene_Factory(gameContext, configData); });
    gameContext.sceneManager->registerScene("PREQUEL_STAGE_4", [&](void* configData) { return createPrequel4Scene_Factory(gameContext, configData); });

    BootSceneConfig bootConfig;
    bootConfig.isWakingUp = gameContext.lastWakeUpInfo.wasWakeUpFromDeepSleep;
    bootConfig.sleepDurationMinutes = gameContext.lastWakeUpInfo.sleepDurationMinutes;
    gameContext.sceneManager->requestSetCurrentScene("BOOT", &bootConfig);


    debugPrint("SYSTEM", "Display and Engine setup completed.");
    wifiManager_ptr->init(WIFI_SSID, WIFI_PASSWORD, server_ptr, u8g2, OTA_PASSWORD, gameStats_ptr);
    server_ptr->begin();
    debugPrint("WIFI_MANAGER", "WiFi Manager initialized and HTTP server started for OTA.");
    debugPrint("SYSTEM", "Initializing WebSerial OnMessage...");
    webSerial_ptr->onMessage(
        [&](const std::string &msg)
        {
            debugPrintf("WIFI_MANAGER", "WebSerial RX: %s", msg.c_str());
            if (gameContext.commandHandler)
            {
                gameContext.commandHandler->processSerialCommand(String(msg.c_str()));
            }
            else
            {
                debugPrint("SYSTEM", "ERROR: commandHandler is null in WebSerial callback (via context)!");
            }
        });
    webSerial_ptr->setBuffer(0);

    Serial.flush();
    delay(100);
    webSerial_ptr->begin(server_ptr);
    delay(100);
    debugPrint("SYSTEM", "WebSerial initialized.");
    debugPrint("SYSTEM", "Initializing setKeyQueue...");
    if (gameContext.inputManager)
    {
        gameContext.inputManager->setKeyQueue(keyQueue);
    }

    debugPrint("SYSTEM", "Initializing ESP_ERROR_CHECK...");
    ESP_ERROR_CHECK(esp_event_handler_instance_register(EBTN_EVENTS, ESP_EVENT_ANY_ID, &HardwareInputController::handleButtonEvent_static, NULL, NULL));
    globalButtonOk_ptr->enableEvent(ESPButton::event_t::click);
    globalButtonOk_ptr->enableEvent(ESPButton::event_t::longPress);
    globalButtonOk_ptr->enableEvent(ESPButton::event_t::release, false);
    globalButtonOk_ptr->enable();
    physicalButtonUp_ptr->enableEvent(ESPButton::event_t::click);
    physicalButtonUp_ptr->enable();
    physicalButtonDown_ptr->enableEvent(ESPButton::event_t::click);
    physicalButtonDown_ptr->enable();
    debugPrint("HARDWARE_INPUT", "Global & Physical Buttons setup completed.");
    debugPrint("BLUETOOTH", "Initializing Bluepad32 Core...");
    BP32.setup(&HardwareInputController::onConnectedController_static, &HardwareInputController::onDisconnectedController_static);
    debugPrint("BLUETOOTH", "Bluepad32 setup complete.");

    if (gameContext.bluetoothManager) {
        gameContext.bluetoothManager->init(&gameContext, myControllers);
        gameContext.bluetoothManager->fullyDisableStack(); 
        debugPrint("BLUETOOTH", "Bluetooth stack disabled by default on boot.");
    }


    debugPrint("BLUETOOTH", "Bluetooth Setup Complete.");

    if (gameContext.commandHandler)
        gameContext.commandHandler->init();

    debugPrint("SYSTEM", "Setup completed.");
    startTime = millis();
    previousTickTime = micros();
}

void loop()
{
    if (wifiManager_ptr && wifiManager_ptr->isRebootRequired()) {
        delay(500); // Give a moment for any final serial/network traffic
        ESP.restart();
    }

    if (!engine || !gameContext.commandHandler || !gameContext.wifiManager ||
        !gameContext.bluetoothManager || !gameContext.gameStats || !gameContext.serialForwarder ||
        !gameContext.characterManager || !gameContext.deepSleepController ||
        !prequelManager_ptr || !gameContext.periodicTaskManager || !gameContext.hardwareInputController ||
        !gameContext.weatherManager || !gameContext.pathGenerator || !gameContext.sceneManager || !gameContext.inputManager || !gameContext.renderer || !gameContext.display || !screenStreamer_ptr)
    {
        Serial.println("Loop Error: Core object pointer(s) or context members are NULL!");
        delay(1000);
        return;
    }

    gameContext.wifiManager->handleOTA();

    if (wifiManager_ptr->isOTAInProgress()) {
        if (gameContext.serialForwarder) {
            gameContext.serialForwarder->flushWebSerial(); 
        }
        vTaskDelay(pdMS_TO_TICKS(100)); 
        return;
    }

    if (gameContext.sceneManager->isSceneChangePending()) {
        String pendingName = gameContext.sceneManager->getPendingSceneName();
        if (pendingName == "POST_BOOT_TRANSITION") {
             debugPrint("SYSTEM", "main.cpp: Handling POST_BOOT_TRANSITION logic.");
             String nextScene = gameContext.prequelManager->getNextSceneNameAfterBootOrPrequel();
             gameContext.sceneManager->clearPendingSceneChange(); 
             gameContext.sceneManager->requestSetCurrentScene(nextScene, nullptr); 
        }
    }


    gameContext.commandHandler->handleSerialInput();

    unsigned long currentMillis = millis();


    gameContext.bluetoothManager->update(currentMillis);
    gameContext.inputManager->processQueuedKeys();

    if (currentMillis - lastActivityTime > INACTIVITY_TIMEOUT_MILLIS)
    {
        debugPrintf("SLEEP_CONTROLLER", "Inactivity timeout (%lu ms). Going to sleep.\n", INACTIVITY_TIMEOUT_MILLIS);
        gameContext.deepSleepController->goToSleep(false);
    }


    unsigned long currentTimeMicros = micros();
    if (currentTimeMicros - previousTickTime >= TICK_INTERVAL_MICROS)
    {
        unsigned long elapsedMicros = currentTimeMicros - previousTickTime;
        unsigned long ticksToProcess = elapsedMicros / TICK_INTERVAL_MICROS;
        if (ticksToProcess > 5) ticksToProcess = 5;
        previousTickTime += ticksToProcess * TICK_INTERVAL_MICROS;

        for (unsigned long i = 0; i < ticksToProcess; ++i)
        {
            engine->update();
        }
        tickCounter += ticksToProcess;

        engine->draw();

        if (screenStreamer_ptr) {
            screenStreamer_ptr->streamFrame();
        }
    } else {
        vTaskDelay(pdMS_TO_TICKS(1));
    }


    if (gameContext.periodicTaskManager) {
        gameContext.periodicTaskManager->update(currentMillis, lastActivityTime);
    }

    gameContext.serialForwarder->flushWebSerial();
}