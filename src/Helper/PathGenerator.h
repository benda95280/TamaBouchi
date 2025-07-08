#ifndef PATH_GENERATOR_H
#define PATH_GENERATOR_H

#include "FastNoiseLite.h" // <-- Corrected include path
#include <vector>
#include <Arduino.h> // For random(), radians(), degrees() etc.
#include <stdint.h>  // For int16_t

// --- Define PathPoint Struct HERE ---
struct PathPoint {
    int16_t x;
    int16_t y;
};
// --- End PathPoint Definition ---


// --- Path Generation Configuration ---
struct PathGenConfig {
    int minPoints = 15;
    int maxPoints = 25;
    float minStepDistance = 15.0f;
    float maxStepDistance = 40.0f;
    float minAngleDegrees = 30.0f;
    float noiseScale = 0.06f;
    int offscreenMargin = 10;
    int clampMargin = 30;
    unsigned long baseDurationMs = 6000;
    float durationPerPixel = 25.0f;
    unsigned long minDurationMs = 5000;
    unsigned long maxDurationMs = 25000;
    int maxGenAttemptsPerPoint = 30;
    float angleNudgeRad = radians(15.0f);
    float returnToCenterBias = 0.4f; // 0.0 to 1.0
    float returnToCenterNudge = 0.2f; // 0.0 to 1.0
};


class PathGenerator {
public:
    PathGenerator(); // Constructor can initialize noise

    // Generates a dynamic path based on noise
    bool generateFlyingPath(std::vector<PathPoint>& outPath,
                            unsigned long& estimatedDurationMs,
                            int screenW, int screenH,
                            const PathGenConfig& config = PathGenConfig()); // Use default config if none provided

private:
    FastNoiseLite _noise;

    // --- Internal Math/Geometry Helpers ---
    // (Moved from _1AwakeningSparkScene)
    float normalizeAngle(float angle);
    float blendAngles(float angle1, float angle2, float weight2);
    bool checkPathAngleConstraint(float newAngle, float lastAngle,
                                 const std::vector<PathPoint>& currentPath,
                                 const PathGenConfig& config);
    void clampPointToBoundary(PathPoint& point, int margin, int screenW, int screenH);
};

#endif // PATH_GENERATOR_H