#include "ScreenStreamer.h"
#include <memory>
#include <vector>
#include "esp_event.h"
#include "espasyncbutton.hpp"
#include "GlobalMappings.h"

ScreenStreamer::ScreenStreamer(U8G2* u8g2, AsyncWebServer* server, bool flip180, bool useCompression, bool useDeltaFrames)
    : _u8g2_ptr(u8g2), _server(server), _ws(nullptr), _flip180(flip180), _useCompression(useCompression), _useDeltaFrames(useDeltaFrames),
      _lastFrameTime(0), _frameInterval(33), // Default to ~30 FPS
      _lastFullFrameTime(0), _fullFrameInterval(10000) // 10 seconds
{
    if (!_u8g2_ptr || !_server) {
        debugPrint("U8G2_WEBSTREAM", "ERROR: ScreenStreamer requires valid U8G2 and WebServer pointers!");
        return;
    }

    size_t bufferSize = (_u8g2_ptr->getDisplayWidth() * _u8g2_ptr->getDisplayHeight()) / 8;
    // Allocate a buffer large enough for worst-case RLE or for the delta payload
    _processBuffer.reset(new uint8_t[bufferSize * 2 + 1]);
    debugPrint("U8G2_WEBSTREAM", "Processing buffer allocated.");

    if (_flip180) {
        _flipBuffer.reset(new uint8_t[bufferSize]);
        debugPrint("U8G2_WEBSTREAM", "Flip buffer allocated.");
    }

    if (_useDeltaFrames) {
        _previousBuffer.reset(new uint8_t[bufferSize]);
        memset(_previousBuffer.get(), 0, bufferSize); // Initialize to a known state
        debugPrint("U8G2_WEBSTREAM", "Delta frame buffer allocated.");
    }
}

ScreenStreamer::~ScreenStreamer() {
    if (_ws) {
        delete _ws;
    }
}

void ScreenStreamer::init() {
    if (!_server) return;
    
    _ws = new AsyncWebSocket("/screenviewer/ws");

    _ws->onEvent(std::bind(&ScreenStreamer::onWsEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
    _server->addHandler(_ws);
    
    _server->on("/screenviewer", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", SCREENVIEWER_HTML, SCREENVIEWER_HTML_SIZE);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    debugPrint("U8G2_WEBSTREAM", "ScreenStreamer initialized.");
}

void ScreenStreamer::postVirtualButtonEvent(gpio_num_t pin, ESPButton::event_t eventType) {
    debugPrintf("U8G2_WEBSTREAM", "Posting virtual button event. Pin: %d, Type: %d\n", (int)pin, (int)eventType);
    EventMsg msg = {(int32_t)pin, 0};
    esp_err_t err = esp_event_post(EBTN_EVENTS, static_cast<int32_t>(eventType), &msg, sizeof(EventMsg), pdMS_TO_TICKS(10));
    if (err != ESP_OK) {
        debugPrintf("U8G2_WEBSTREAM", "Error posting virtual button event: %s\n", esp_err_to_name(err));
    }
}

void ScreenStreamer::onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        debugPrintf("U8G2_WEBSTREAM", "WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        _force_next_frame_stream = true; // Force a full frame for the new client
    } else if (type == WS_EVT_DISCONNECT) {
        debugPrintf("U8G2_WEBSTREAM", "WebSocket client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            data[len] = 0; // Null-terminate the received data
            String message = String((char*)data);

            if (message.equals("ping")) {
                client->text("pong");
            } else if (message.startsWith("BTN_EVENT:")) {
                String pinStr = "";
                String typeStr = "";

                int pinIdx = message.indexOf("PIN=");
                int typeIdx = message.indexOf("TYPE=");
                
                if (pinIdx != -1 && typeIdx != -1) {
                    int commaIdx = message.indexOf(',', pinIdx);
                    if (commaIdx > pinIdx) {
                        pinStr = message.substring(pinIdx + 4, commaIdx);
                    } else {
                         pinStr = message.substring(pinIdx + 4);
                    }
                    
                    commaIdx = message.indexOf(',', typeIdx);
                    if (commaIdx > typeIdx) {
                        typeStr = message.substring(typeIdx + 5, commaIdx);
                    } else {
                        typeStr = message.substring(typeIdx + 5);
                    }
                    
                    gpio_num_t pin = (gpio_num_t)pinStr.toInt();
                    ESPButton::event_t eventType;
                    bool eventFound = true;

                    if (typeStr.equalsIgnoreCase("PRESS")) eventType = ESPButton::event_t::press;
                    else if (typeStr.equalsIgnoreCase("RELEASE")) eventType = ESPButton::event_t::release;
                    else if (typeStr.equalsIgnoreCase("CLICK")) eventType = ESPButton::event_t::click;
                    else if (typeStr.equalsIgnoreCase("LONG_PRESS")) eventType = ESPButton::event_t::longPress;
                    else {
                        eventFound = false;
                    }

                    if (eventFound) {
                        postVirtualButtonEvent(pin, eventType);
                    } else {
                        debugPrintf("U8G2_WEBSTREAM", "Unknown button event type received: %s\n", typeStr.c_str());
                    }
                }
            }
        }
    }
}

void ScreenStreamer::flipBuffer180(const uint8_t* src, uint8_t* dest, int width, int height) {
    int buffer_size = (width * height) / 8;
    memset(dest, 0, buffer_size);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int src_byte_index = (y / 8) * width + x;
            int src_bit_mask = 1 << (y % 8);
            if ((src[src_byte_index] & src_bit_mask) != 0) {
                int dest_x = width - 1 - x;
                int dest_y = height - 1 - y;
                int dest_byte_index = (dest_y / 8) * width + dest_x;
                int dest_bit_mask = 1 << (dest_y % 8);
                dest[dest_byte_index] |= dest_bit_mask;
            }
        }
    }
}

size_t ScreenStreamer::compressRLE(const uint8_t* src, size_t srcLen, uint8_t* dest) {
    size_t destIndex = 0;
    for (size_t i = 0; i < srcLen; ) {
        uint8_t currentByte = src[i];
        uint8_t runCount = 1;
        while (i + runCount < srcLen && src[i + runCount] == currentByte && runCount < 255) {
            runCount++;
        }
        dest[destIndex++] = runCount;
        dest[destIndex++] = currentByte;
        i += runCount;
    }
    return destIndex;
}

void ScreenStreamer::sendFullFrame() {
    const uint8_t* sourceBuffer = _u8g2_ptr->getBufferPtr();
    size_t sourceBufferSize = (_u8g2_ptr->getDisplayWidth() * _u8g2_ptr->getDisplayHeight()) / 8;

    if (_flip180) {
        flipBuffer180(_u8g2_ptr->getBufferPtr(), _flipBuffer.get(), _u8g2_ptr->getDisplayWidth(), _u8g2_ptr->getDisplayHeight());
        sourceBuffer = _flipBuffer.get();
    }

    uint8_t* payload = _processBuffer.get();
    size_t payloadSize = 0;

    if (_useCompression) {
        size_t compressedSize = compressRLE(sourceBuffer, sourceBufferSize, payload + 1);
        if (compressedSize < sourceBufferSize) {
            payload[0] = 'C';
            payloadSize = 1 + compressedSize;
        } else {
            payload[0] = 'U';
            memcpy(payload + 1, sourceBuffer, sourceBufferSize);
            payloadSize = 1 + sourceBufferSize;
        }
    } else {
        payload[0] = 'U';
        memcpy(payload + 1, sourceBuffer, sourceBufferSize);
        payloadSize = 1 + sourceBufferSize;
    }
    
    _ws->binaryAll(payload, payloadSize);

    // After sending a full frame, update the previous buffer for delta comparison
    if (_useDeltaFrames) {
        memcpy(_previousBuffer.get(), sourceBuffer, sourceBufferSize);
    }
}

void ScreenStreamer::sendDeltaFrame() {
    const uint8_t* sourceBuffer = _u8g2_ptr->getBufferPtr();
    size_t sourceBufferSize = (_u8g2_ptr->getDisplayWidth() * _u8g2_ptr->getDisplayHeight()) / 8;

    if (_flip180) {
        flipBuffer180(_u8g2_ptr->getBufferPtr(), _flipBuffer.get(), _u8g2_ptr->getDisplayWidth(), _u8g2_ptr->getDisplayHeight());
        sourceBuffer = _flipBuffer.get();
    }

    // Find differences between the current buffer and the previous one
    std::vector<uint8_t> deltaPayload;
    for (size_t i = 0; i < sourceBufferSize; ++i) {
        if (sourceBuffer[i] != _previousBuffer.get()[i]) {
            // Index needs 2 bytes since it can be > 255
            deltaPayload.push_back(i >> 8);   // Index High Byte
            deltaPayload.push_back(i & 0xFF); // Index Low Byte
            deltaPayload.push_back(sourceBuffer[i]); // New Value
        }
    }

    if (!deltaPayload.empty()) {
        uint8_t* payload = _processBuffer.get();
        payload[0] = 'D'; // Delta frame header
        memcpy(payload + 1, deltaPayload.data(), deltaPayload.size());
        size_t payloadSize = 1 + deltaPayload.size();

        _ws->binaryAll(payload, payloadSize);

        // Update previous buffer with the new data
        memcpy(_previousBuffer.get(), sourceBuffer, sourceBufferSize);
    }
}


void ScreenStreamer::streamFrame() {
    unsigned long now = millis();
    if (now - _lastFrameTime < _frameInterval) {
        return; // Pacing control
    }
    _lastFrameTime = now;

    if (!_u8g2_ptr || !_ws || _ws->count() == 0) {
        return;
    }
    _ws->cleanupClients();

    // Back-pressure flow control
    const std::list<AsyncWebSocketClient>& clients = _ws->getClients();
    for (std::list<AsyncWebSocketClient>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->status() == WS_CONNECTED && !it->canSend()) {
            return;
        }
    }

    // Logic for periodic full frame updates
    if (_useDeltaFrames && (now - _lastFullFrameTime > _fullFrameInterval)) {
        _force_next_frame_stream = true;
    }

    if (_force_next_frame_stream || !_useDeltaFrames) {
        sendFullFrame();
        _force_next_frame_stream = false;
        _lastFullFrameTime = now;
    } else {
        sendDeltaFrame();
    }
}