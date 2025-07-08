#include "PathGenerator.h"
#include <cmath>     // For trig, sqrt, abs, round, fmod
#include <algorithm> // For std::min/max
#include <vector>
#include "../DebugUtils.h" // Include debug utilities

PathGenerator::PathGenerator() {
    // Initialize noise generator defaults
    _noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    // Frequency and seed can be set per-generation if needed, or keep defaults
    _noise.SetFrequency(0.05);
    _noise.SetSeed(esp_random());
}

// --- Math Helpers (from _1AwakeningSparkScene) ---
float PathGenerator::normalizeAngle(float angle) {
    while (angle <= -PI) angle += 2 * PI;
    while (angle > PI) angle -= 2 * PI;
    return angle;
}

float PathGenerator::blendAngles(float angle1, float angle2, float weight2) {
    angle1 = normalizeAngle(angle1);
    angle2 = normalizeAngle(angle2);
    float diff = normalizeAngle(angle2 - angle1);
    return normalizeAngle(angle1 + diff * weight2);
}

bool PathGenerator::checkPathAngleConstraint(float newAngle, float lastAngle,
                                            const std::vector<PathPoint>& currentPath,
                                            const PathGenConfig& config) {
    if (currentPath.size() < 2) return true; // No constraint for the first segment
    float minAngleRad = radians(config.minAngleDegrees);
    if (minAngleRad <= 0.0f) return true; // Constraint disabled

    // Calculate the absolute difference, handling wrap-around PI / -PI
    float actualTurnAngle = abs(fmod(newAngle - lastAngle + PI, 2.0f * PI) - PI);
    return actualTurnAngle >= minAngleRad;
}

void PathGenerator::clampPointToBoundary(PathPoint& point, int margin, int screenW, int screenH) {
    int minX = -margin;
    int maxX = screenW + margin;
    int minY = -margin;
    int maxY = screenH + margin;

    // Explicitly cast point members to int for comparison if necessary,
    // though std::max/min with int and int16_t should be okay usually.
    // Casting result back to int16_t.
    point.x = (int16_t)std::max(minX, std::min(maxX, (int)point.x));
    point.y = (int16_t)std::max(minY, std::min(maxY, (int)point.y));
}

// --- Path Generation Logic (adapted from _1AwakeningSparkScene) ---
bool PathGenerator::generateFlyingPath(std::vector<PathPoint>& outPath,
                                      unsigned long& estimatedDurationMs,
                                      int screenW, int screenH,
                                      const PathGenConfig& config) {
    outPath.clear();
    _noise.SetSeed(esp_random()); // Use a new seed each time

    int targetPointsThisPath = random(config.minPoints, config.maxPoints + 1);
    debugPrintf("PATH_GENERATOR", "PathGen Start: Target %d points. MinAngle=%.1f deg", targetPointsThisPath, config.minAngleDegrees);

    PathPoint currentPoint;
    int startEdge = random(4);
    switch (startEdge) {
        case 0: currentPoint = { (int16_t)random(0, screenW), (int16_t)(-config.offscreenMargin) }; break;
        case 1: currentPoint = { (int16_t)random(0, screenW), (int16_t)(screenH + config.offscreenMargin) }; break;
        case 2: currentPoint = { (int16_t)(-config.offscreenMargin), (int16_t)random(0, screenH) }; break;
        default: currentPoint = { (int16_t)(screenW + config.offscreenMargin), (int16_t)random(0, screenH) }; break;
    }
    outPath.push_back(currentPoint);
    debugPrintf("PATH_GENERATOR", "  Point 0: Start (%d, %d)", currentPoint.x, currentPoint.y);

    float lastAngle = atan2(screenH / 2.0f - currentPoint.y, screenW / 2.0f - currentPoint.x);
    float totalDistance = 0;
    int totalAttempts = 0;

    for (int i = 0; i < targetPointsThisPath; ++i) {
        bool pointFound = false;
        for (int attempt = 1; attempt <= config.maxGenAttemptsPerPoint; ++attempt) {
            totalAttempts++;

            // Get noise value - Use a consistent noise scale
            float noiseVal = _noise.GetNoise(currentPoint.x * config.noiseScale, currentPoint.y * config.noiseScale);
            float angleDelta = noiseVal * PI; // Noise determines angle change magnitude/direction

            // Calculate step distance
            float stepDistance = (float)random(config.minStepDistance * 100, config.maxStepDistance * 100) / 100.0f;

            // Calculate potential next angle
            float potentialNextAngle = normalizeAngle(lastAngle + angleDelta);

            // Check if point is way outside boundaries and bias towards center if needed
            bool forceReturn = currentPoint.x < -config.clampMargin || currentPoint.x > screenW + config.clampMargin ||
                               currentPoint.y < -config.clampMargin || currentPoint.y > screenH + config.clampMargin;

            if (forceReturn) {
                float angleToCenter = atan2(screenH / 2.0f - currentPoint.y, screenW / 2.0f - currentPoint.x);
                // Blend current direction with direction towards center
                float biasWeight = config.returnToCenterBias + (float)random((int)(config.returnToCenterNudge * 100.f))/100.0f;
                potentialNextAngle = blendAngles(potentialNextAngle, angleToCenter, biasWeight);
                stepDistance *= 0.9f; // Slightly reduce step when returning
                // Fix potential std::max issue by casting
                stepDistance = std::max((float)config.minStepDistance, stepDistance);
                // debugPrintf("PATH_GENERATOR", "    Forcing return. Bias: %.2f", biasWeight);
            }

            // --- Angle Constraint Check and Correction ---
            bool angleOk = checkPathAngleConstraint(potentialNextAngle, lastAngle, outPath, config);

            if (!angleOk) {
                 // Try nudging slightly first
                float nudgedAngle1 = normalizeAngle(potentialNextAngle + config.angleNudgeRad);
                if (checkPathAngleConstraint(nudgedAngle1, lastAngle, outPath, config)) {
                    potentialNextAngle = nudgedAngle1; angleOk = true;
                    // debugPrintf("PATH_GENERATOR", "    Angle Nudged + %.1f deg", degrees(config.angleNudgeRad));
                } else {
                    float nudgedAngle2 = normalizeAngle(potentialNextAngle - config.angleNudgeRad);
                    if (checkPathAngleConstraint(nudgedAngle2, lastAngle, outPath, config)) {
                        potentialNextAngle = nudgedAngle2; angleOk = true;
                        // debugPrintf("PATH_GENERATOR", "    Angle Nudged - %.1f deg", degrees(config.angleNudgeRad));
                    } else {
                        // Nudging failed, try a more random deviation within allowed range from last angle
                        float randomDeviation = radians(random(config.minAngleDegrees + 1, 180)) * (random(0,2)?1:-1); // Deviate more drastically but still within limits relative to *last* angle
                        potentialNextAngle = normalizeAngle(lastAngle + randomDeviation);
                        angleOk = checkPathAngleConstraint(potentialNextAngle, lastAngle, outPath, config); // Re-check (should pass by definition now)
                        if (!angleOk) {
                             // Should theoretically not happen with the random deviation logic above
                             // debugPrintf("PATH_GENERATOR", "    Angle Random Deviation Failed (This shouldn't happen!)");
                             continue; // Try again with new noise/step
                        }
                        // debugPrintf("PATH_GENERATOR", "    Angle Random Deviation: %.1f deg", degrees(randomDeviation));
                    }
                }
            }
            // --- End Angle Constraint Check ---


            PathPoint nextPoint = {
                (int16_t)round(currentPoint.x + cos(potentialNextAngle) * stepDistance),
                (int16_t)round(currentPoint.y + sin(potentialNextAngle) * stepDistance)
            };

            // Clamp point to avoid extreme excursions (important for return bias)
            clampPointToBoundary(nextPoint, config.clampMargin, screenW, screenH);

            // Check if the clamped point is too close to the current point
            float dx_check = nextPoint.x - currentPoint.x;
            float dy_check = nextPoint.y - currentPoint.y;
            // Fix potential std::max/min issue by casting
            if (sqrt(dx_check*dx_check + dy_check*dy_check) < (float)config.minStepDistance * 0.8f) {
                 // If clamped point is too close, indicates we might be stuck near a boundary
                 // Try a more drastic angle change on next attempt
                 lastAngle = normalizeAngle(lastAngle + radians(random(90, 180) * (random(0,2)?1:-1))); // Random large turn
                // debugPrintf("PATH_GENERATOR", "    Clamped point too close, forcing large turn.");
                 continue;
            }

            // Valid point found
            outPath.push_back(nextPoint);
            totalDistance += sqrt(dx_check*dx_check + dy_check*dy_check);
            currentPoint = nextPoint;
            lastAngle = potentialNextAngle;
            pointFound = true;
            // debugPrintf("PATH_GENERATOR", "  Point %d: Added (%d, %d) after %d attempts. Angle: %.1f deg, Step: %.1f", outPath.size()-1, currentPoint.x, currentPoint.y, attempt, degrees(lastAngle), stepDistance);
            break; // Exit attempt loop
        }

        if (!pointFound) {
             debugPrintf("PATH_GENERATOR", "Warning: PathGen STUCK at point %d after %d attempts.", i + 1, config.maxGenAttemptsPerPoint);
            // Could potentially try to force an exit point here, or just return false
            break; // Exit generation loop
        }
    }

    // Add a final point to ensure the path exits the screen, if it hasn't already
    if (outPath.size() > 1 &&
        (currentPoint.x >= -config.offscreenMargin && currentPoint.x <= screenW + config.offscreenMargin) &&
        (currentPoint.y >= -config.offscreenMargin && currentPoint.y <= screenH + config.offscreenMargin))
    {
        float exitAngle = lastAngle + _noise.GetNoise(currentPoint.x * config.noiseScale + 50.0f, currentPoint.y * config.noiseScale + 50.0f) * PI * 0.3f; // Slightly deviate exit angle
        float exitDistance = config.maxStepDistance * 1.5f; // Ensure it goes far enough
        PathPoint finalPoint = {
            (int16_t)round(currentPoint.x + cos(exitAngle) * exitDistance),
            (int16_t)round(currentPoint.y + sin(exitAngle) * exitDistance)
        };
         // Clamp the final point just to be safe, though it should exit
        clampPointToBoundary(finalPoint, config.offscreenMargin * 4, screenW, screenH);

        outPath.push_back(finalPoint);
        float dx = finalPoint.x - currentPoint.x;
        float dy = finalPoint.y - currentPoint.y;
        totalDistance += sqrt(dx*dx + dy*dy);
         debugPrintf("PATH_GENERATOR", "  Point %d: Exit (%d, %d)", outPath.size()-1, finalPoint.x, finalPoint.y);
    }

    // Calculate duration based on distance
    estimatedDurationMs = config.baseDurationMs + (unsigned long)(totalDistance * config.durationPerPixel);
    // Fix potential std::max/min issue by casting
    estimatedDurationMs = std::max(config.minDurationMs, std::min(config.maxDurationMs, estimatedDurationMs));


    debugPrintf("PATH_GENERATOR", "PathGen: Completed with %u points. Total attempts: %d. Est Dur: %lu ms", outPath.size(), totalAttempts, estimatedDurationMs);

    return outPath.size() >= 3; // Need at least start, middle, end
}