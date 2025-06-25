#include "StormWeatherEffect.h"
#include "StormWeatherGraphics.h"
#include <U8g2lib.h>
#include <algorithm>
#include <cmath>
#include "../../../System/GameContext.h" // Ensure GameContext is known for _context usage
#include "../../../DebugUtils.h"
#include "SerialForwarder.h"


StormWeatherEffect::StormWeatherEffect(GameContext &context) // Takes GameContext
    : WeatherEffectBase(context)
{ // Pass context to base
    _activeStrikes.reserve(MAX_SIMULTANEOUS_STRIKES);
    if (_context.serialForwarder)
        _context.serialForwarder->println("StormWeatherEffect created");
}

void StormWeatherEffect::init(unsigned long currentTime)
{
    initRainDrops();
    initWindLines();
    _activeStrikes.clear();
    _lastStrikeTriggerTime = currentTime;
    _strikeCountLast10s = 0;
    _strikeCountLastMinute = 0;
    _fullscreenStrikeCountLastMinute = 0;
    _last10sBoundary = currentTime;
    _lastMinuteBoundary = currentTime;
    _lastMinuteBoundaryFullscreen = currentTime;
    if (_context.serialForwarder)
        _context.serialForwarder->println("StormWeatherEffect init");
}

void StormWeatherEffect::update(unsigned long currentTime)
{
    updateRainDrops();
    updateWindLines();
    updateLightningStrikes();

    if (currentTime - _last10sBoundary >= 10000)
    {
        _strikeCountLast10s = 0;
        _last10sBoundary = currentTime;
    }
    if (currentTime - _lastMinuteBoundary >= 60000)
    {
        _strikeCountLastMinute = 0;
        _lastMinuteBoundary = currentTime;
    }
    if (currentTime - _lastMinuteBoundaryFullscreen >= 60000)
    {
        _fullscreenStrikeCountLastMinute = 0;
        _lastMinuteBoundaryFullscreen = currentTime;
    }

    if (_activeStrikes.size() < MAX_SIMULTANEOUS_STRIKES &&
        currentTime - _lastStrikeTriggerTime > MIN_TIME_BETWEEN_STRIKES_MS &&
        _strikeCountLast10s < MAX_STRIKES_PER_10S &&
        _strikeCountLastMinute < MAX_STRIKES_PER_MINUTE)
    {
        if (random(100) < 15)
        {
            triggerLightning(currentTime);
        }
    }
}

void StormWeatherEffect::initRainDrops()
{
    if (!_context.renderer)
        return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();
    for (int i = 0; i < MAX_RAIN_DROPS_STORM; ++i)
    {
        _rainDrops[i] = {(int)random(0, screenW), (int)random(-screenH, -5), (int)random(2, 5)};
    }
}

void StormWeatherEffect::updateRainDrops()
{
    if (!_context.renderer)
        return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();
    uint8_t speed_factor = 2;
    uint8_t current_density = _particleDensity;
    uint8_t drops_to_update = (MAX_RAIN_DROPS_STORM * current_density) / 100;
    drops_to_update = std::min((int)MAX_RAIN_DROPS_STORM, (int)drops_to_update);

    for (int i = 0; i < drops_to_update; ++i)
    {
        int current_speed = std::abs(_rainDrops[i].speed);
        if (current_speed == 0)
            current_speed = 1;
        _rainDrops[i].y += current_speed * speed_factor;
        _rainDrops[i].x += static_cast<int>(round(_currentWindFactor * speed_factor * 0.7f));

        if (_rainDrops[i].y > screenH || _rainDrops[i].x < -10 || _rainDrops[i].x > screenW + 10)
        {
            _rainDrops[i].x = (int)random(0, screenW);
            _rainDrops[i].y = -(int)random(5, 30);
            _rainDrops[i].speed = (int)random(2, 5);
        }
    }
}

void StormWeatherEffect::drawRain()
{
    if (!_context.renderer || !_context.display)
        return;
    U8G2 *u8g2 = _context.display;
    Renderer &renderer = *_context.renderer;

    int screenW = renderer.getWidth();
    int screenH = renderer.getHeight();
    uint8_t current_density = _particleDensity;
    uint8_t drops_to_draw = (MAX_RAIN_DROPS_STORM * current_density) / 100;
    drops_to_draw = std::min((int)MAX_RAIN_DROPS_STORM, (int)drops_to_draw);

    for (int i = 0; i < drops_to_draw; ++i)
    {
        int x_local = _rainDrops[i].x;
        int y1_local = _rainDrops[i].y;
        int y2_local = y1_local + 3;
        if (x_local >= 0 && x_local < screenW &&
            ((y1_local >= 0 && y1_local < screenH) || (y2_local >= 0 && y2_local < screenH) || (y1_local < 0 && y2_local >= screenH)))
        {
            int draw_y1 = std::max(0, std::min(screenH - 1, y1_local));
            int draw_y2 = std::max(0, std::min(screenH - 1, y2_local));
            if (draw_y1 <= draw_y2)
                renderer.drawLine(x_local, draw_y1, x_local, draw_y2);
        }
    }
}

void StormWeatherEffect::initWindLines()
{
    if (!_context.renderer)
        return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();
    for (int i = 0; i < MAX_WIND_LINES_STORM; ++i)
    {
        int dx = (int)random(8, 20);
        int dy = (int)random(-3, 4);
        _windLines[i].x1 = (int)random(0, screenW);
        _windLines[i].y1 = (int)random(0, screenH);
        _windLines[i].x2 = _windLines[i].x1 - dx;
        _windLines[i].y2 = _windLines[i].y1 + dy;
    }
}

void StormWeatherEffect::updateWindLines()
{
    if (!_context.renderer)
        return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();
    for (int i = 0; i < MAX_WIND_LINES_STORM; ++i)
    {
        float baseSpeed = 4.5f;
        float windEffectSpeed = baseSpeed + std::abs(_currentWindFactor) * 2.5f;
        int dy = static_cast<int>(round(_currentWindFactor * 1.2f));
        dy = std::max(-3, std::min(3, dy));
        int dx_magnitude = static_cast<int>(round(windEffectSpeed));
        dx_magnitude = std::max(3, std::min(10, dx_magnitude));
        int dx = static_cast<int>(_currentWindFactor * (float)dx_magnitude * 0.9f);
        dx = std::max(-dx_magnitude, std::min(dx_magnitude, dx));
        if (dx == 0 && _currentWindFactor != 0.0f)
            dx = (_currentWindFactor < 0) ? -1 : 1;

        _windLines[i].x1 += dx;
        _windLines[i].x2 += dx;
        _windLines[i].y1 += dy;
        _windLines[i].y2 += dy;

        bool outOfBounds = (_windLines[i].x1 < -35 && _windLines[i].x2 < -35) ||
                           (_windLines[i].x1 > screenW + 35 && _windLines[i].x2 > screenW + 35) ||
                           (_windLines[i].y1 < -35 && _windLines[i].y2 < -35) ||
                           (_windLines[i].y1 > screenH + 35 && _windLines[i].y2 > screenH + 35);
        if (outOfBounds)
        {
            if (_currentWindFactor > 0.1f)
                _windLines[i].x1 = -random(5, 20);
            else if (_currentWindFactor < -0.1f)
                _windLines[i].x1 = screenW + random(5, 20);
            else
                _windLines[i].x1 = random(0, screenW);
            _windLines[i].y1 = random(0, screenH);
            int len_dx = random(12, 25);
            int len_dy = random(-3, 4);
            _windLines[i].x2 = (_currentWindFactor > 0.1f) ? (_windLines[i].x1 + len_dx) : (_windLines[i].x1 - len_dx);
            _windLines[i].y2 = _windLines[i].y1 + len_dy;
        }
    }
}

void StormWeatherEffect::drawWindLines()
{
    if (!_context.renderer)
        return;
    Renderer &renderer = *_context.renderer;
    int screenW = renderer.getWidth();
    int screenH = renderer.getHeight();
    for (int i = 0; i < MAX_WIND_LINES_STORM; ++i)
    {
        int x1_abs = renderer.getXOffset() + _windLines[i].x1;
        int y1_abs = renderer.getYOffset() + _windLines[i].y1;
        int x2_abs = renderer.getXOffset() + _windLines[i].x2;
        int y2_abs = renderer.getYOffset() + _windLines[i].y2;

        bool isLineOnScreen = !((x1_abs < renderer.getXOffset() && x2_abs < renderer.getXOffset()) ||
                                (x1_abs >= renderer.getXOffset() + screenW && x2_abs >= renderer.getXOffset() + screenW) ||
                                (y1_abs < renderer.getYOffset() && y2_abs < renderer.getYOffset()) ||
                                (y1_abs >= renderer.getYOffset() + screenH && y2_abs >= renderer.getYOffset() + screenH));
        if (isLineOnScreen)
        {
            // Use local coordinates for renderer.drawLine
            renderer.drawLine(_windLines[i].x1, _windLines[i].y1, _windLines[i].x2, _windLines[i].y2);
        }
    }
}

void StormWeatherEffect::triggerLightning(unsigned long currentTime)
{
    if (!_context.renderer)
        return;
    Renderer &renderer = *_context.renderer;

    int strikesToCreate = 1;
    if (random(100) < 20)
        strikesToCreate = random(2, 4);
    bool createdFullscreenThisBurst = false;

    for (int k = 0; k < strikesToCreate; ++k)
    {
        if (_activeStrikes.size() >= MAX_SIMULTANEOUS_STRIKES ||
            _strikeCountLast10s >= MAX_STRIKES_PER_10S ||
            _strikeCountLastMinute >= MAX_STRIKES_PER_MINUTE)
            break;

        LightningStrike newStrike;
        const unsigned char *strikeBitmap = nullptr;
        int frameWidth = 0, frameHeight = 0, frameCount = 0;
        size_t bytesPerFrame = 0;
        StrikeType selectedStrikeType;

        int choice = random(100);
        bool tryFullscreen = (choice < 10) && !createdFullscreenThisBurst && (_fullscreenStrikeCountLastMinute < MAX_FULLSCREEN_STRIKES_PER_MINUTE);

        if (tryFullscreen)
        {
            selectedStrikeType = StrikeType::FULLSCREEN;
            strikeBitmap = epd_bitmap_strike3;
            frameWidth = STRIKE3_FRAME_WIDTH;
            frameHeight = STRIKE3_FRAME_HEIGHT;
            frameCount = STRIKE3_FRAME_COUNT;
            bytesPerFrame = STRIKE3_BYTES_PER_FRAME;
            newStrike.x = 0;
            newStrike.y = 0;
            newStrike.screenFlash = true;
            _fullscreenStrikeCountLastMinute++;
            createdFullscreenThisBurst = true;
            if (_context.serialForwarder)
                _context.serialForwarder->println("Storm: Triggering Fullscreen Strike");
        }
        else
        {
            choice = random(2);
            if (choice == 0)
            {
                selectedStrikeType = StrikeType::SMALL_1;
                strikeBitmap = epd_bitmap_strike1;
                frameWidth = STRIKE1_FRAME_WIDTH;
                frameHeight = STRIKE1_FRAME_HEIGHT;
                frameCount = STRIKE1_FRAME_COUNT;
                bytesPerFrame = STRIKE1_BYTES_PER_FRAME;
            }
            else
            {
                selectedStrikeType = StrikeType::SMALL_2;
                strikeBitmap = epd_bitmap_strike2;
                frameWidth = STRIKE2_FRAME_WIDTH;
                frameHeight = STRIKE2_FRAME_HEIGHT;
                frameCount = STRIKE2_FRAME_COUNT;
                bytesPerFrame = STRIKE2_BYTES_PER_FRAME;
            }
            newStrike.x = random(0, renderer.getWidth() - frameWidth);
            newStrike.y = 0;
            newStrike.screenFlash = (random(100) < 15);
        }
        newStrike.type = selectedStrikeType;
        newStrike.animator.reset(new Animator(renderer, strikeBitmap, frameWidth, frameHeight, frameCount, bytesPerFrame, newStrike.x, newStrike.y, STRIKE_ANIM_FRAME_DURATION_MS, 1));

        if (newStrike.animator)
        {
            _activeStrikes.push_back(std::move(newStrike));
            _strikeCountLast10s++;
            _strikeCountLastMinute++;
            _lastStrikeTriggerTime = currentTime;
            if (_context.serialForwarder)
                _context.serialForwarder->printf("Storm: ⚡ Lightning Strike %d/%d! (Type %d, Flash: %s)\n", k + 1, strikesToCreate, (int)selectedStrikeType, newStrike.screenFlash ? "Yes" : "No");
        }
        else
        {
            if (_context.serialForwarder)
                _context.serialForwarder->println("Storm: Failed to create lightning animator!");
        }
    }
}

void StormWeatherEffect::updateLightningStrikes()
{
    for (auto it = _activeStrikes.begin(); it != _activeStrikes.end();)
    {
        if (!it->animator || !it->animator->update())
        {
            it = _activeStrikes.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void StormWeatherEffect::drawLightningStrikes()
{
    for (auto &strike : _activeStrikes)
    {
        if (strike.animator)
        {
            strike.animator->draw();
        }
    }
}

bool StormWeatherEffect::isFlashing() const
{
    for (const auto &s : _activeStrikes)
    {
        if (s.screenFlash)
            return true;
    }
    return false;
}

void StormWeatherEffect::drawBackground() {}

void StormWeatherEffect::drawForeground()
{
    if (!_context.renderer || !_context.display)
        return;
    U8G2 *u8g2 = _context.display;
    Renderer &renderer = *_context.renderer;

    bool fullscreenStrikeActive = false;
    for (const auto &s : _activeStrikes)
    {
        if (s.type == StrikeType::FULLSCREEN)
        {
            fullscreenStrikeActive = true;
            break;
        }
    }

    uint8_t originalColor = u8g2->getDrawColor();

    u8g2->setDrawColor(2);
    drawRain();
    drawWindLines();

    if (fullscreenStrikeActive)
        u8g2->setDrawColor(2);
    else
        u8g2->setDrawColor(1);
    drawLightningStrikes();

    if (isFlashing())
    {
        u8g2->setDrawColor(2);
        u8g2->drawBox(renderer.getXOffset(), renderer.getYOffset(), renderer.getWidth(), renderer.getHeight());
    }

    u8g2->setDrawColor(originalColor);
}

WeatherType StormWeatherEffect::getType() const
{
    return WeatherType::STORM;
}