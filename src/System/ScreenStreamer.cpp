#include "ScreenStreamer.h"
#include <memory>
#include <vector>
#include "esp_event.h"
#include "espasyncbutton.hpp"
#include "GlobalMappings.h"

ScreenStreamer::ScreenStreamer(U8G2* u8g2, AsyncWebServer* server, bool flip180, bool useCompression, bool useDeltaFrames, int batchSize)
    : _u8g2_ptr(u8g2), _server(server), _ws(nullptr), _flip180(flip180), _useCompression(useCompression), _useDeltaFrames(useDeltaFrames), _batchSize(batchSize),
      _lastFrameTime(0), _frameInterval(33), // Default to ~30 FPS
      _lastFullFrameTime(0), _fullFrameInterval(10000) // 10 seconds
{
    if (!_u8g2_ptr || !_server) {
        debugPrint("U8G2_WEBSTREAM", "ERROR: ScreenStreamer requires valid U8G2 and WebServer pointers!");
        return;
    }

    // Buffers are no longer allocated here. They will be allocated on first client connect.
    debugPrint("U8G2_WEBSTREAM", "ScreenStreamer constructed. Buffers will be allocated on client connect.");
}

ScreenStreamer::~ScreenStreamer() {
    if (_ws) {
        delete _ws;
    }
    // unique_ptrs will automatically handle deallocation if they are still holding memory.
}

void ScreenStreamer::allocateBuffers() {
    if (_processBuffer) {
        debugPrint("U8G2_WEBSTREAM", "allocateBuffers called, but buffers already exist. Ignoring.");
        return;
    }
    
    debugPrint("U8G2_WEBSTREAM", "First client connected. Allocating streaming buffers...");
    size_t bufferSize = (_u8g2_ptr->getDisplayWidth() * _u8g2_ptr->getDisplayHeight()) / 8;

    if (_batchSize > 1) {
        _useDeltaFrames = false;
        size_t batchBufferSizeBytes = bufferSize * _batchSize;
        
        // Use a more conservative size for the compression buffer (e.g., 120% of raw size)
        _processBufferCapacity = (size_t)(batchBufferSizeBytes * 1.2);
        if (_processBufferCapacity < 2048) _processBufferCapacity = 2048; // Ensure a minimum reasonable size

        _processBuffer.reset(new (std::nothrow) uint8_t[_processBufferCapacity]);
        if (!_processBuffer) {
            debugPrint("U8G2_WEBSTREAM", "FATAL: Failed to allocate _processBuffer!");
            deallocateBuffers();
            return;
        }
        
        _batchBuffer.reset(new (std::nothrow) uint8_t[batchBufferSizeBytes]);
        if (!_batchBuffer) {
            debugPrint("U8G2_WEBSTREAM", "FATAL: Failed to allocate _batchBuffer!");
            deallocateBuffers();
            return;
        }
        debugPrintf("U8G2_WEBSTREAM", "Batching enabled. Batch buffer: %u bytes, Process buffer: %u bytes", batchBufferSizeBytes, _processBufferCapacity);
    } else {
        _processBufferCapacity = bufferSize * 2 + 1; // For single frames, worst-case is still needed
        _processBuffer.reset(new (std::nothrow) uint8_t[_processBufferCapacity]);
        if (!_processBuffer) {
            debugPrint("U8G2_WEBSTREAM", "FATAL: Failed to allocate _processBuffer (non-batch)!");
            deallocateBuffers();
            return;
        }
        debugPrint("U8G2_WEBSTREAM", "Processing buffer allocated for single frames.");
    }
    
    if (_flip180) {
        _flipBuffer.reset(new (std::nothrow) uint8_t[bufferSize]);
        if (!_flipBuffer) {
            debugPrint("U8G2_WEBSTREAM", "FATAL: Failed to allocate _flipBuffer!");
            deallocateBuffers();
            return;
        }
        debugPrint("U8G2_WEBSTREAM", "Flip buffer allocated.");
    }
    
    _previousBuffer.reset(new (std::nothrow) uint8_t[bufferSize]);
    if (!_previousBuffer) {
        debugPrint("U8G2_WEBSTREAM", "FATAL: Failed to allocate _previousBuffer!");
        deallocateBuffers();
        return;
    }
    memset(_previousBuffer.get(), 0, bufferSize);
    debugPrint("U8G2_WEBSTREAM", "Previous frame buffer allocated.");
}

void ScreenStreamer::deallocateBuffers() {
    if (!_processBuffer && !_previousBuffer) {
        debugPrint("U8G2_WEBSTREAM", "deallocateBuffers called, but no buffers to free. Ignoring.");
        return;
    }

    debugPrint("U8G2_WEBSTREAM", "Last client disconnected. Deallocating streaming buffers to free RAM.");
    _processBuffer.reset();
    _flipBuffer.reset();
    _previousBuffer.reset();
    _batchBuffer.reset();
    _processBufferCapacity = 0; // Reset capacity
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
        if (_ws->count() == 1) { // First client
            allocateBuffers();
        }
        _force_next_frame_stream = true;
    } else if (type == WS_EVT_DISCONNECT) {
        debugPrintf("U8G2_WEBSTREAM", "WebSocket client #%u disconnected\n", client->id());
        if (_ws->count() == 0) { // Last client
            deallocateBuffers();
        }
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
    if (!dest) {
        debugPrint("U8G2_WEBSTREAM", "flipBuffer180 error: destination buffer is null.");
        return;
    }
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

size_t ScreenStreamer::compressRLE(const uint8_t* src, size_t srcLen, uint8_t* dest, size_t destLen) {
    size_t destIndex = 0;
    for (size_t i = 0; i < srcLen; ) {
        uint8_t currentByte = src[i];
        uint8_t runCount = 1;
        while (i + runCount < srcLen && src[i + runCount] == currentByte && runCount < 255) {
            runCount++;
        }
        
        if (destIndex + 2 > destLen) {
            // Not enough space in destination buffer for this run
            return 0; // Indicate failure
        }

        dest[destIndex++] = runCount;
        dest[destIndex++] = currentByte;
        i += runCount;
    }
    return destIndex;
}

// Simple CRC32 implementation
uint32_t ScreenStreamer::crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xffffffff;
    while (length--) {
        uint8_t byte = *data++;
        crc = crc ^ byte;
        for (int j = 0; j < 8; j++) {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xedb88320 & mask);
        }
    }
    return ~crc;
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
        size_t compressedSize = compressRLE(sourceBuffer, sourceBufferSize, payload + 1, _processBufferCapacity - 1);
        if (compressedSize > 0) { // Failure returns 0
            payload[0] = 'C';
            payloadSize = 1 + compressedSize;
        } else {
            payload[0] = 'U';
            memcpy(payload + 1, sourceBuffer, sourceBufferSize);
            payloadSize = 1 + sourceBufferSize;
            debugPrint("U8G2_WEBSTREAM", "RLE compression failed for full frame, sending uncompressed.");
        }
    } else {
        payload[0] = 'U';
        memcpy(payload + 1, sourceBuffer, sourceBufferSize);
        payloadSize = 1 + sourceBufferSize;
    }
    
    _ws->binaryAll(payload, payloadSize);
    memcpy(_previousBuffer.get(), sourceBuffer, sourceBufferSize);
}

void ScreenStreamer::sendDeltaFrame() {
    const uint8_t* sourceBuffer = _u8g2_ptr->getBufferPtr();
    size_t sourceBufferSize = (_u8g2_ptr->getDisplayWidth() * _u8g2_ptr->getDisplayHeight()) / 8;

    if (_flip180) {
        flipBuffer180(_u8g2_ptr->getBufferPtr(), _flipBuffer.get(), _u8g2_ptr->getDisplayWidth(), _u8g2_ptr->getDisplayHeight());
        sourceBuffer = _flipBuffer.get();
    }

    std::vector<uint8_t> deltaPayload;
    for (size_t i = 0; i < sourceBufferSize; ++i) {
        if (sourceBuffer[i] != _previousBuffer.get()[i]) {
            deltaPayload.push_back(i >> 8);
            deltaPayload.push_back(i & 0xFF);
            deltaPayload.push_back(sourceBuffer[i]);
        }
    }

    if (!deltaPayload.empty()) {
        uint8_t* payload = _processBuffer.get();
        payload[0] = 'D';
        memcpy(payload + 1, deltaPayload.data(), deltaPayload.size());
        size_t payloadSize = 1 + deltaPayload.size();

        _ws->binaryAll(payload, payloadSize);
        memcpy(_previousBuffer.get(), sourceBuffer, sourceBufferSize);
    }
}

void ScreenStreamer::sendBatchFrame() {
    size_t frameSize = (_u8g2_ptr->getDisplayWidth() * _u8g2_ptr->getDisplayHeight()) / 8;
    size_t batchDataSize = frameSize * _currentBatchFrameCount;
    bool isCompressed = false;

    size_t dataToSendSize = 0;
    uint8_t* dataToSendPtr = nullptr;
    
    if (_useCompression) {
        size_t compressedSize = compressRLE(_batchBuffer.get(), batchDataSize, _processBuffer.get(), _processBufferCapacity);
        if (compressedSize > 0) { // compressRLE returns 0 on failure/overflow
            dataToSendSize = compressedSize;
            dataToSendPtr = _processBuffer.get();
            isCompressed = true;
        } else {
            // Compression failed or result would overflow. Send uncompressed.
            dataToSendSize = batchDataSize;
            dataToSendPtr = _batchBuffer.get();
            isCompressed = false;
            debugPrint("U8G2_WEBSTREAM", "RLE compression overflowed or failed, sending uncompressed batch.");
        }
    } else {
        dataToSendSize = batchDataSize;
        dataToSendPtr = _batchBuffer.get();
        isCompressed = false;
    }

    // Calculate CRC on the data that will actually be sent (compressed or not)
    uint32_t crc = crc32(dataToSendPtr, dataToSendSize);

    // Packet structure: [Header 'B'] [Frame Count] [Target FPS] [IsCompressed] [CRC32] [Data]
    size_t headerSize = 1 + 1 + 1 + 1 + 4;
    size_t totalPacketSize = headerSize + dataToSendSize;

    std::unique_ptr<uint8_t[]> finalPacket(new uint8_t[totalPacketSize]);
    finalPacket[0] = 'B';
    finalPacket[1] = (uint8_t)_currentBatchFrameCount;
    finalPacket[2] = (uint8_t)(1000 / _frameInterval); // Target FPS
    finalPacket[3] = isCompressed ? 1 : 0; // The new compression flag
    memcpy(&finalPacket[4], &crc, sizeof(crc));
    memcpy(&finalPacket[headerSize], dataToSendPtr, dataToSendSize);

    _ws->binaryAll(finalPacket.get(), totalPacketSize);

    // After sending, update the previous buffer with the *last* frame from the batch
    memcpy(_previousBuffer.get(), _batchBuffer.get() + (batchDataSize - frameSize), frameSize);
    _currentBatchFrameCount = 0; // Reset batch
}


void ScreenStreamer::streamFrame() {
    unsigned long now = millis();
    if (now - _lastFrameTime < _frameInterval) {
        return; 
    }
    _lastFrameTime = now;

    if (!_u8g2_ptr || !_ws || _ws->count() == 0 || !_processBuffer || !_previousBuffer) {
        return;
    }
    _ws->cleanupClients();
    
    // If we are currently sending a frame, check if the send is complete
    if (_isSending) {
        bool allQueuesEmpty = true;
        const std::list<AsyncWebSocketClient>& clients = _ws->getClients();
        for (const AsyncWebSocketClient& client : clients) {
            if (client.status() == WS_CONNECTED && client.queueLen() > 0) {
                allQueuesEmpty = false;
                break;
            }
        }
        if (allQueuesEmpty) {
            _isSending = false;
        } else {
            return;
        }
    }
    
    // Perform dirty check using memcmp
    const uint8_t* currentU8G2Buffer = _u8g2_ptr->getBufferPtr();
    size_t bufferSize = (_u8g2_ptr->getDisplayWidth() * _u8g2_ptr->getDisplayHeight()) / 8;
    bool isDirty = (memcmp(currentU8G2Buffer, _previousBuffer.get(), bufferSize) != 0);

    if (!isDirty && !_force_next_frame_stream) {
        return; // No change, no send
    }
    
    // Handle non-batching mode first
    if (_batchSize <= 1) {
        if (_force_next_frame_stream || !_useDeltaFrames) {
            sendFullFrame();
            _force_next_frame_stream = false;
            _lastFullFrameTime = now;
        } else {
            sendDeltaFrame();
        }
        _isSending = true; // Mark as sending
        return;
    }

    // --- Batching Logic ---
    const uint8_t* sourceBuffer = currentU8G2Buffer;
    if (_flip180) {
        flipBuffer180(currentU8G2Buffer, _flipBuffer.get(), _u8g2_ptr->getDisplayWidth(), _u8g2_ptr->getDisplayHeight());
        sourceBuffer = _flipBuffer.get();
    }
    
    // Add the current dirty frame to the batch
    memcpy(_batchBuffer.get() + (_currentBatchFrameCount * bufferSize), sourceBuffer, bufferSize);
    _currentBatchFrameCount++;

    // Update the "last drawn" buffer for the next dirty check
    memcpy(_previousBuffer.get(), sourceBuffer, bufferSize);
    
    // If the batch is full, send it
    if (_currentBatchFrameCount >= _batchSize) {
        sendBatchFrame();
        _isSending = true; // Mark as sending
    }
}