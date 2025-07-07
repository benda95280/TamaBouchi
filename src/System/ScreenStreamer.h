#ifndef SCREEN_STREAMER_H
#define SCREEN_STREAMER_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <ESPAsyncWebServer.h>
#include <atomic>
#include <memory>
#include "System/ScreenWebPage.h"
#include "DebugUtils.h"
#include "espasyncbutton.hpp" // For event types

// Forward Declaration
class Renderer;

class ScreenStreamer {
public:
    ScreenStreamer(U8G2* u8g2, AsyncWebServer* server, bool flip180 = false, bool useCompression = true, bool useDeltaFrames = true, int batchSize = 0);
    ~ScreenStreamer();
    
    void init();
    void streamFrame();

private:
    U8G2* _u8g2_ptr;
    AsyncWebServer* _server;
    AsyncWebSocket* _ws;
    
    // Configuration
    bool _flip180;
    bool _useCompression;
    bool _useDeltaFrames;
    int _batchSize;

    // Buffers
    std::unique_ptr<uint8_t[]> _processBuffer; // Used for compression output/final payload
    size_t _processBufferCapacity = 0;
    std::unique_ptr<uint8_t[]> _flipBuffer;    // Used for flipping if enabled
    std::unique_ptr<uint8_t[]> _previousBuffer; // Used for dirty checking
    std::unique_ptr<uint8_t[]> _batchBuffer;   // For batching frames
    int _currentBatchFrameCount = 0;          // Counter for frames in batch

    // Timers & State
    unsigned long _lastFrameTime;
    unsigned long _frameInterval;
    unsigned long _lastFullFrameTime;
    const unsigned long _fullFrameInterval;
    std::atomic<bool> _force_next_frame_stream{true};
    bool _isSending = false;

    // Methods
    void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
    void postVirtualButtonEvent(gpio_num_t pin, ESPButton::event_t eventType);
    void flipBuffer180(const uint8_t* src, uint8_t* dest, int width, int height);
    size_t compressRLE(const uint8_t* src, size_t srcLen, uint8_t* dest, size_t destLen);
    uint32_t crc32(const uint8_t *data, size_t length);
    void sendFullFrame();
    void sendDeltaFrame();
    void sendBatchFrame();

    // Dynamic Buffer Management
    void allocateBuffers();
    void deallocateBuffers();
};

#endif // SCREEN_STREAMER_H