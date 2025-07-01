#include "WiFiManager.h"
#include <WiFi.h>
#include "DebugUtils.h"
#include <Update.h>
#include <ArduinoOTA.h>
#include <esp_heap_caps.h>
#include <freertos/task.h>
#include "GameStats.h"
#include "Weather/WeatherManager.h"


// Initialize static member
WiFiManager* WiFiManager::_instance = nullptr;

WiFiManager::WiFiManager(Preferences& preferences) :
    _prefs(preferences)
{
    _instance = this;
}

void WiFiManager::loadCredentials() {
    if (_prefs.begin(PREF_WIFI_NAMESPACE, true)) { // read-only = true
        _wifiEnabledByUser = _prefs.getBool(PREF_KEY_WIFI_ENABLED, true); // Default to enabled
        _loaded_ssid = _prefs.getString(PREF_KEY_WIFI_SSID, "");
        _loaded_password = _prefs.getString(PREF_KEY_WIFI_PWD, "");
        _prefs.end();
        if (_loaded_ssid.length() > 0) {
            debugPrint("WIFI_MANAGER","WiFi credentials loaded from Preferences.");
        } else {
            debugPrint("WIFI_MANAGER","No WiFi credentials found in Preferences. Using defaults.");
        }
        debugPrintf("WIFI_MANAGER", "User WiFi preference is: %s", _wifiEnabledByUser ? "ENABLED" : "DISABLED");
    } else {
        debugPrint("WIFI_MANAGER","Failed to open WiFi preferences for reading. Using defaults.");
    }

    // Use defaults if nothing was loaded
    if (_loaded_ssid.length() == 0) {
        _loaded_ssid = _default_ssid ? _default_ssid : "";
    }
    if (_loaded_password.length() == 0) {
         _loaded_password = _default_password ? _default_password : "";
    }
}

void WiFiManager::enableWiFi(bool enable) {
    _wifiEnabledByUser = enable;
    if (_prefs.begin(PREF_WIFI_NAMESPACE, false)) {
        _prefs.putBool(PREF_KEY_WIFI_ENABLED, _wifiEnabledByUser);
        _prefs.end();
        debugPrintf("WIFI_MANAGER", "Set WiFi preference to %s and saved.", enable ? "ENABLED" : "DISABLED");
    } else {
        debugPrintf("WIFI_MANAGER", "Failed to save WiFi preference to %s.", enable ? "ENABLED" : "DISABLED");
    }

    if (enable) {
        startConnection();
    } else {
        WiFi.disconnect(true);
        debugPrint("WIFI_MANAGER", "WiFi disconnected by user setting.");
    }
}


void WiFiManager::saveCredentials(const String& ssid, const String& password) {
    if (_prefs.begin(PREF_WIFI_NAMESPACE, false)) { // read-only = false
        _prefs.putString(PREF_KEY_WIFI_SSID, ssid);
        _prefs.putString(PREF_KEY_WIFI_PWD, password);
        _prefs.end();
        debugPrint("WIFI_MANAGER","WiFi credentials saved to Preferences.");
        _loaded_ssid = ssid; // Update loaded credentials
        _loaded_password = password;
        // Saving credentials implies the user wants WiFi on.
        enableWiFi(true);
    } else {
        debugPrint("WIFI_MANAGER","Failed to open WiFi preferences for writing.");
    }
}

void WiFiManager::clearCredentials() {
    if (_prefs.begin(PREF_WIFI_NAMESPACE, false)) { // read-only = false
        _prefs.remove(PREF_KEY_WIFI_SSID);
        _prefs.remove(PREF_KEY_WIFI_PWD);
        _prefs.end();
        debugPrint("WIFI_MANAGER","WiFi credentials cleared from Preferences.");
        _loaded_ssid = _default_ssid ? _default_ssid : ""; // Revert to defaults
        _loaded_password = _default_password ? _default_password : "";
        // Clearing credentials also implies disabling WiFi until it's re-configured.
        enableWiFi(false);
    } else {
        debugPrint("WIFI_MANAGER","Failed to open WiFi preferences for clearing.");
    }
}

void WiFiManager::startConnection() {
    if (!_wifiEnabledByUser) {
        debugPrint("WIFI_MANAGER","Cannot start WiFi connection: Disabled by user preference.");
        return;
    }
    if (_loaded_ssid.length() == 0) {
        debugPrint("WIFI_MANAGER","Cannot start WiFi connection: SSID is empty.");
        // Disable for next boot since it can't work anyway
        if (_wifiEnabledByUser) {
            enableWiFi(false);
        }
        return;
    }
    debugPrintf("WIFI_MANAGER","Connecting to WiFi SSID: %s\n", _loaded_ssid.c_str());
    WiFi.begin(_loaded_ssid.c_str(), _loaded_password.c_str());
}


void WiFiManager::init(const char* default_ssid, const char* default_password, AsyncWebServer* server, U8G2* u8g2, const char* ota_password, GameStats* gameStats) {
    _default_ssid = default_ssid;
    _default_password = default_password;
    _ota_password = ota_password;
    _server = server;
    _u8g2 = u8g2;
    _gameStats = gameStats;

    if (!_server || !_u8g2 || !_gameStats) {
        debugPrint("WIFI_MANAGER","ERROR: WiFiManager requires valid server, U8G2, and GameStats pointers!");
        return;
    }

    debugPrint("WIFI_MANAGER","Initializing WiFiManager...");

    loadCredentials();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);

    // Use static wrappers for WiFi events
    WiFi.onEvent(WiFiGotIPWrapper, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFiEventWrapper);

    // Start connection only if conditions are met
    startConnection();

    // Configure server routes
    _server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        const char* html = 
            "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><title>TamaBouchi Control</title>"
            "<style>"
            "body{background-color:#2c2c2c;color:#eee;font-family:sans-serif;display:flex;justify-content:center;padding-top:2rem;padding-bottom:2rem;}"
            ".main-container{display:flex;flex-direction:column;gap:1.5rem;width:90%;max-width:700px;}"
            ".widget{background-color:#3c3c3c;padding:1.5rem;border-radius:8px;border:1px solid #555;box-shadow:0 4px 8px rgba(0,0,0,0.3);}"
            "h2{margin-top:0;border-bottom:1px solid #555;padding-bottom:0.5rem;}"
            "table{width:100%;border-collapse:collapse;}th,td{padding:8px;text-align:left;border-bottom:1px solid #4a4a4a;}"
            "th{color:#aaa;width:35%;}td span{font-weight:bold;color:#fff;}"
            ".nav-links a{display:block;background-color:#007bff;color:white;padding:12px;text-align:center;text-decoration:none;border-radius:4px;margin-bottom:1rem;transition:background-color 0.2s;}"
            ".nav-links a:hover{background-color:#0056b3;}"
            "</style></head><body>"
            "<div class='main-container'>"
            "  <div class='widget'><h2>Device Information</h2>"
            "    <table>"
            "      <tr><th>Uptime</th><td><span id='uptime'>-</span></td></tr>"
            "      <tr><th>Free Heap</th><td><span id='heap_free'>-</span> bytes</td></tr>"
            "      <tr><th>Min Free Heap</th><td><span id='heap_min'>-</span> bytes</td></tr>"
            "      <tr><th>WiFi Status</th><td><span id='wifi_status'>-</span></td></tr>"
            "      <tr><th>IP Address</th><td><span id='wifi_ip'>-</span></td></tr>"
            "    </table>"
            "  </div>"
            "  <div class='widget'><h2>TamaBouchi Status</h2>"
            "    <table>"
            "      <tr><th>Age</th><td><span id='stat_age'>-</span></td></tr>"
            "      <tr><th>Health / Happy</th><td><span id='stat_health'>-</span> / <span id='stat_happy'>-</span></td></tr>"
            "      <tr><th>Hunger / Fatigue</th><td><span id='stat_hunger'>-</span> / <span id='stat_fatigue'>-</span></td></tr>"
            "      <tr><th>Dirty Level</th><td><span id='stat_dirty'>-</span> / 100</td></tr>"
            "      <tr><th>Sickness</th><td><span id='stat_sickness'>-</span></td></tr>"
            "    </table>"
            "  </div>"
            "  <div class='widget'><h2>General Stats</h2>"
            "    <table>"
            "      <tr><th>Language</th><td><span id='stat_lang'>-</span></td></tr>"
            "      <tr><th>Money / Points</th><td><span id='stat_money'>-</span> / <span id='stat_points'>-</span></td></tr>"
            "      <tr><th>Total Play Time</th><td><span id='stat_playtime'>-</span></td></tr>"
            "      <tr><th>Currently Sleeping</th><td><span id='stat_sleeping'>-</span></td></tr>"
            "      <tr><th>Current Weather</th><td><span id='stat_weather'>-</span></td></tr>"
            "      <tr><th>Next Weather Change</th><td><span id='stat_weather_next'>-</span></td></tr>"
            "    </table>"
            "  </div>"
            "  <div class='widget'><h2>Actions</h2>"
            "    <div class='nav-links'>"
            "      <a href='/screenviewer' target='_blank'>OLED Screen Viewer</a>"
            "      <a href='/update'>Firmware Update</a>"
            "      <a href='/webserial' target='_blank'>Web Serial Console</a>"
            "    </div>"
            "  </div>"
            "</div>"
            "<script>"
            "function fetchData(){fetch('/stats').then(r=>r.json()).then(d=>{"
            "document.getElementById('uptime').textContent=d.uptime;"
            "document.getElementById('heap_free').textContent=d.heap_free;"
            "document.getElementById('heap_min').textContent=d.heap_min;"
            "document.getElementById('wifi_status').textContent=d.wifi_status;"
            "document.getElementById('wifi_ip').textContent=d.wifi_ip;"
            "document.getElementById('stat_age').textContent=d.stat_age;"
            "document.getElementById('stat_health').textContent=d.stat_health;"
            "document.getElementById('stat_happy').textContent=d.stat_happy;"
            "document.getElementById('stat_hunger').textContent=d.stat_hunger;"
            "document.getElementById('stat_fatigue').textContent=d.stat_fatigue;"
            "document.getElementById('stat_dirty').textContent=d.stat_dirty;"
            "document.getElementById('stat_sickness').textContent=d.stat_sickness;"
            "document.getElementById('stat_lang').textContent=d.stat_lang;"
            "document.getElementById('stat_money').textContent=d.stat_money;"
            "document.getElementById('stat_points').textContent=d.stat_points;"
            "document.getElementById('stat_playtime').textContent=d.stat_playtime;"
            "document.getElementById('stat_sleeping').textContent=d.stat_sleeping;"
            "document.getElementById('stat_weather').textContent=d.stat_weather;"
            "document.getElementById('stat_weather_next').textContent=d.stat_weather_next;"
            "}).catch(e=>console.error('Error fetching stats:',e));}"
            "document.addEventListener('DOMContentLoaded',function(){fetchData();setInterval(fetchData,5000);});"
            "</script></body></html>";
        request->send(200, "text/html", html);
    });

    _server->on("/stats", HTTP_GET, [this](AsyncWebServerRequest *request){
        String json = "{";
        
        unsigned long now = millis();
        unsigned long sec = now / 1000, min = sec / 60, hr = min / 60;
        char uptimeStr[20];
        snprintf(uptimeStr, sizeof(uptimeStr), "%lu:%02lu:%02lu", hr, min % 60, sec % 60);
        json += "\"uptime\":\"" + String(uptimeStr) + "\",";
        
        json += "\"heap_free\":" + String(ESP.getFreeHeap()) + ",";
        json += "\"heap_min\":" + String(ESP.getMinFreeHeap()) + ",";

        if (WiFi.status() == WL_CONNECTED) {
            json += "\"wifi_status\":\"Connected\",";
            json += "\"wifi_ip\":\"" + WiFi.localIP().toString() + "\",";
        } else {
            json += "\"wifi_status\":\"Disconnected\",";
            json += "\"wifi_ip\":\"N/A\",";
        }

        if(_gameStats) {
            GameStats* gs = _gameStats;
            json += "\"stat_age\":" + String(gs->age) + ",";
            json += "\"stat_health\":" + String(gs->health) + ",";
            json += "\"stat_happy\":" + String(gs->getModifiedHappiness()) + ",";
            json += "\"stat_hunger\":" + String(gs->hunger) + ",";
            json += "\"stat_fatigue\":" + String(gs->fatigue) + ",";
            json += "\"stat_dirty\":" + String(gs->dirty) + ",";
            json += "\"stat_sickness\":\"" + String(gs->getSicknessString()) + "\",";

            json += "\"stat_lang\":\"" + String(gs->selectedLanguage == Language::FRENCH ? "French" : "English") + "\",";
            json += "\"stat_money\":" + String(gs->money) + ",";
            json += "\"stat_points\":" + String(gs->points) + ",";
            
            char playTimeStr[20];
            snprintf(playTimeStr, sizeof(playTimeStr), "%uh %um", gs->playingTimeMinutes / 60, gs->playingTimeMinutes % 60);
            json += "\"stat_playtime\":\"" + String(playTimeStr) + "\",";

            json += "\"stat_sleeping\":\"" + String(gs->isSleeping ? "Yes" : "No") + "\",";
            json += "\"stat_weather\":\"" + String(WeatherManager::weatherTypeToString(gs->currentWeather)) + "\",";

            long remainingWeatherMs = (long)gs->nextWeatherChangeTime - (long)millis();
            char weatherNextStr[20];
            if (remainingWeatherMs <= 0) {
                strcpy(weatherNextStr, "Changing...");
            } else {
                snprintf(weatherNextStr, sizeof(weatherNextStr), "in %ld min", remainingWeatherMs / 60000);
            }
            json += "\"stat_weather_next\":\"" + String(weatherNextStr) + "\"";

        } else {
            json += "\"stat_age\":-1, \"stat_health\":-1, \"stat_happy\":-1, \"stat_hunger\":-1, \"stat_fatigue\":-1, \"stat_dirty\":-1, \"stat_sickness\":\"N/A\", \"stat_lang\":\"N/A\", \"stat_money\":-1, \"stat_points\":-1, \"stat_playtime\":\"N/A\", \"stat_sleeping\":\"N/A\", \"stat_weather\":\"N/A\", \"stat_weather_next\":\"N/A\"";
        }

        json += "}";
        request->send(200, "application/json", json);
    });

    _server->on("/update", HTTP_GET, [this](AsyncWebServerRequest *request){
        const char* html = 
            "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><title>ESP OTA Update</title>"
            "<style>"
            "body{background-color:#2c2c2c;color:#eee;font-family:sans-serif;text-align:center;display:flex;justify-content:center;align-items:center;height:100vh;margin:0;}"
            ".container{background-color:#3c3c3c;padding:2rem;border-radius:8px;border:1px solid #555;box-shadow:0 4px 8px rgba(0,0,0,0.3);width:350px;}"
            "h1{margin-top:0;}"
            "form{display:flex;flex-direction:column;align-items:center;}"
            "input[type=file]{background-color:#4a4a4a;border:1px solid #555;padding:10px;border-radius:4px;margin-bottom:1rem;cursor:pointer;}"
            "input[type=file]::file-selector-button{background-color:#5a5a5a;color:#eee;border:none;padding:8px 12px;border-radius:4px;cursor:pointer;margin-right:10px;}"
            "input[type=submit], button{background-color:#4CAF50;color:white;padding:12px 24px;border:none;border-radius:4px;cursor:pointer;font-size:1rem;transition:background-color 0.2s;}"
            "input[type=submit]:hover, button:hover{background-color:#45a049;}"
            "input[type=submit]:disabled{background-color:#555;cursor:not-allowed;}"
            ".btn-refresh{background-color:#3498db;}"
            ".btn-refresh:hover{background-color:#2980b9;}"
            "#progress-container{width:100%;background-color:#555;border-radius:4px;margin-top:1rem;overflow:hidden;}"
            "#progress-bar{width:0%;height:24px;background-color:#4CAF50;text-align:center;line-height:24px;color:white;transition:width 0.1s;}"
            "#status{margin-top:1rem;font-weight:bold;}"
            ".status-success{color:#2ecc71;}"
            ".status-error{color:#e74c3c;}"
            "</style></head><body>"
            "<div class='container'><h1>Firmware Update</h1>"
            "<form id='upload-form'><input type='file' name='update' id='file-input' accept='.bin' required><br>"
            "<input type='submit' value='Update' id='submit-btn'>"
            "</form><div id='progress-container' style='display:none;'><div id='progress-bar'>0%</div></div>"
            "<div id='status'></div></div>"
            "<script>"
            "const form=document.getElementById('upload-form');"
            "const fileInput=document.getElementById('file-input');"
            "const submitBtn=document.getElementById('submit-btn');"
            "const progressContainer=document.getElementById('progress-container');"
            "const progressBar=document.getElementById('progress-bar');"
            "const statusDiv=document.getElementById('status');"
            "form.addEventListener('submit',function(e){"
            "e.preventDefault();const file=fileInput.files[0];"
            "if(!file){statusDiv.textContent='Please select a file.';statusDiv.className='status-error';return;}"
            "if(!file.name.toLowerCase().endsWith('.bin')){statusDiv.textContent='Invalid file type. Please select a .bin file.';statusDiv.className='status-error';return;}"
            "const formData=new FormData();formData.append('update',file);"
            "const xhr=new XMLHttpRequest();"
            "xhr.open('POST','/update',true);"
            "xhr.upload.addEventListener('progress',function(e){"
            "if(e.lengthComputable){const percent=Math.round((e.loaded/e.total)*100);"
            "progressBar.style.width=percent+'%';progressBar.textContent=percent+'%';}"
            "});"
            "xhr.onload=function(){"
            "if(xhr.status===200&&xhr.responseText==='OTA_SUCCESS'){statusDiv.textContent='Update successful! Device rebooting...';statusDiv.className='status-success';"
            "submitBtn.disabled=true;setTimeout(()=>window.location.reload(),10000);}"
            "else{statusDiv.textContent='Update failed: '+(xhr.responseText||'Server error');statusDiv.className='status-error';"
            "submitBtn.disabled=false;fileInput.disabled=false;progressContainer.style.display='none';}"
            "};"
            "xhr.onerror=function(){"
            "statusDiv.textContent='Update failed: Connection error.';statusDiv.className='status-error';"
            "submitBtn.disabled=false;fileInput.disabled=false;progressContainer.style.display='none';"
            "};"
            "submitBtn.disabled=true;fileInput.disabled=true;statusDiv.textContent='Uploading...';statusDiv.className='';"
            "progressContainer.style.display='block';progressBar.style.width='0%';progressBar.textContent='0%';"
            "xhr.send(formData);"
            "});</script></body></html>";
        request->send(200, "text/html", html);
    });

    _server->on("/update", HTTP_POST, [this](AsyncWebServerRequest *request) {
        bool success = !Update.hasError();
        handleUpdateEnd(success); 
        
        request->send(200, "text/plain", success ? "OTA_SUCCESS" : "OTA_FAIL");

        if (success) {
            _rebootOnUpdate = true;
            debugPrint("WIFI_MANAGER", "Web OTA finished. Flag set to reboot.");
        }
    }, [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        if (!index) {
            handleUpdateStart();
            debugPrintf("WIFI_MANAGER", "Web OTA Start: %s\n", filename.c_str());

            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                handleUpdateError(Update.errorString());
            }
        }
        
        if (_otaInProgress) {
            if (Update.write(data, len) != len) {
                handleUpdateError(Update.errorString());
            }
            handleUpdateProgress(index + len, request->contentLength());
        }

        if (final) {
            if (_otaInProgress) {
                if (!Update.end(true)) {
                    handleUpdateError(Update.errorString());
                }
            }
        }
    });

    debugPrint("WIFI_MANAGER","WiFiManager Initialized with custom web OTA handler.");
}

void WiFiManager::beginArduinoOTA() {
    if (WiFi.status() != WL_CONNECTED) {
        debugPrint("WIFI_MANAGER", "Cannot start ArduinoOTA, WiFi not connected.");
        return;
    }

    ArduinoOTA.setPort(3232);
    ArduinoOTA.setHostname("tamabouchi");

    if (_ota_password && strlen(_ota_password) > 0) {
        ArduinoOTA.setPassword(_ota_password);
        debugPrint("WIFI_MANAGER", "ArduinoOTA: Password has been set.");
    } else {
        debugPrint("WIFI_MANAGER", "ArduinoOTA: No password set.");
    }

    ArduinoOTA
        .onStart(onOTAStartWrapper_static)
        .onEnd(onOTAEndWrapper_static)
        .onProgress(onOTAProgressWrapper_static)
        .onError(onOTAErrorWrapper_static);
        
    ArduinoOTA.begin();
    debugPrint("WIFI_MANAGER", "ArduinoOTA service started.");
}

void WiFiManager::handleOTA() {
    if (WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.handle();
    }
}

bool WiFiManager::isOTAInProgress() const {
    return _otaInProgress;
}

// --- Static Callback Wrappers ---
void WiFiManager::WiFiGotIPWrapper(WiFiEvent_t event, WiFiEventInfo_t info) { if (_instance) _instance->handleWiFiGotIP(event, info); }
void WiFiManager::WiFiEventWrapper(WiFiEvent_t event, WiFiEventInfo_t info) { if (_instance) _instance->handleWiFiEvent(event, info); }
void WiFiManager::onOTAStartWrapper_static() { if (_instance) _instance->handleUpdateStart(); }
void WiFiManager::onOTAEndWrapper_static() { if (_instance) _instance->handleUpdateEnd(true); }
void WiFiManager::onOTAProgressWrapper_static(unsigned int progress, unsigned int total) { if (_instance) _instance->handleUpdateProgress(progress, total); }
void WiFiManager::onOTAErrorWrapper_static(ota_error_t error) {
    if (_instance) {
        String errorMsg;
        switch (error) {
            case OTA_AUTH_ERROR:    errorMsg = "Auth Failed"; break;
            case OTA_BEGIN_ERROR:   errorMsg = "Begin Failed"; break;
            case OTA_CONNECT_ERROR: errorMsg = "Connect Failed"; break;
            case OTA_RECEIVE_ERROR: errorMsg = "Receive Failed"; break;
            case OTA_END_ERROR:     errorMsg = "End Failed"; break;
            default:                errorMsg = "Unknown Error"; break;
        }
        _instance->handleUpdateError(errorMsg);
    }
}


// --- Member Function Implementations ---
void WiFiManager::handleWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
    debugPrint("WIFI_MANAGER","WiFi connected");
    debugPrint("WIFI_MANAGER","IP address: ");
    debugPrint("WIFI_MANAGER", IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str());
    debugPrint("WIFI_MANAGER","RRSI: ");
    debugPrint("WIFI_MANAGER", String(WiFi.RSSI()).c_str());
    // NOW it's safe to start ArduinoOTA
    beginArduinoOTA();
}

void WiFiManager::handleWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    debugPrintf("WIFI_MANAGER","[WiFi-event] event: %d\n", event);
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            debugPrint("WIFI_MANAGER","WiFi Disconnected. Trying to reconnect...");
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
             debugPrint("WIFI_MANAGER","WiFi Station Started.");
             break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
             debugPrint("WIFI_MANAGER","Obtained IP address via generic handler: ");
             debugPrint("WIFI_MANAGER",String(WiFi.localIP()).c_str());
             break;
       default: break;
     }
}

void WiFiManager::drawUpdateScreen(const char* message, int progress) {
    if (!_u8g2) return; 
    _u8g2->setFont(u8g2_font_6x12_tf);
    _u8g2->firstPage();
    do {
      _u8g2->setCursor(0, 12); 
      _u8g2->print("SSID: ");   
      _u8g2->print(_loaded_ssid.c_str()); 
      _u8g2->setCursor(0, 24); 
      _u8g2->print("Ip: ");     
      if (WiFi.status() == WL_CONNECTED) WiFi.localIP().printTo(*_u8g2);
      else _u8g2->print("Connecting...");
      _u8g2->setCursor(0, 36); 
      _u8g2->print(message);
      char progressStr[20]; 
      snprintf(progressStr, sizeof(progressStr), "Progress: %d%%", progress);
      _u8g2->setCursor(0, 48); 
      _u8g2->print(progressStr);
      int barWidth = map(progress, 0, 100, 0, 120); 
      _u8g2->drawFrame(4, 55, 120, 8); 
      _u8g2->drawBox(4, 55, barWidth, 8); 
    } while (_u8g2->nextPage());
}

// --- Common Update Handlers ---
void WiFiManager::handleUpdateStart() {
    debugPrint("WIFI_MANAGER", "OTA update started!");
    _otaInProgress = true;
    _otaProgress = 0;
    drawUpdateScreen("Update Started", 0);
}

void WiFiManager::handleUpdateEnd(bool success) {
    if(success) {
        debugPrint("WIFI_MANAGER", "OTA update finished!");
        drawUpdateScreen("Update finished", 100);
    } else {
        debugPrint("WIFI_MANAGER", "OTA update failed!");
        drawUpdateScreen("Update FAILED!", _otaProgress);
    }
    _otaInProgress = false; 
}

void WiFiManager::handleUpdateProgress(uint32_t current, uint32_t final) {
    uint8_t newProgress = (final > 0) ? (current * 100) / final : 0;
    if (newProgress > _otaProgress || (millis() - _ota_progress_millis > 1000)) {
        _ota_progress_millis = millis();
        _otaProgress = newProgress;
        debugPrintf("WIFI_MANAGER", "OTA Progress: %u / %u (%d%%)\n", current, final, _otaProgress);
        drawUpdateScreen("Updating...", _otaProgress);
    }
}

void WiFiManager::handleUpdateError(String errorMsg) {
    debugPrintf("WIFI_MANAGER", "OTA Error: %s\n", errorMsg.c_str());
    _otaInProgress = false;
    drawUpdateScreen("OTA Error!", _otaProgress);
}