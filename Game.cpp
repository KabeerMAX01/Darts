#include "Game.h"
#include <cmath>
#include <cstdlib>

using namespace sf;

// Manual clamp (avoids relying on std::clamp / <algorithm> across toolchains)
static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static constexpr double PI = 3.14159265358979323846;

void AimState::reset(AimMode mode) {
    xp = BOARD_CENTER_X;
    yp = BOARD_CENTER_Y;
    xs = ys = 0.f;
    axisPhase = 0;
    axisVal = AIM_X_MIN;
    axisDir = 1.f;
    if (mode == AimMode::FreeRoam) {
        xp = static_cast<float>(rand() % 500 + 150);
        yp = static_cast<float>(rand() % 500 + 50);
    }
}

void updateFreeRoam(AimState& aim, float dt) {
    // Arrow keys nudge the dart, but it also drifts on its own - classic mechanic.
    if (Keyboard::isKeyPressed(Keyboard::Right)) aim.xp += 1.0f;
    if (Keyboard::isKeyPressed(Keyboard::Left))  aim.xp -= 1.0f;
    if (Keyboard::isKeyPressed(Keyboard::Up))    aim.yp -= 1.0f;
    if (Keyboard::isKeyPressed(Keyboard::Down))  aim.yp += 1.0f;

    aim.xs += static_cast<float>((rand() % 11 - 5.0) / 10.0);
    aim.ys += static_cast<float>((rand() % 11 - 5.0) / 10.0);
    aim.xs = clampf(aim.xs, -2.0f, 2.0f);
    aim.ys = clampf(aim.ys, -2.0f, 2.0f);

    if (aim.xp <= AIM_X_MAX && aim.xp >= AIM_X_MIN) {
        aim.xp += 0.5f * aim.xs;
    }
    else {
        aim.xs = 0.f;
        aim.xp = (aim.xp >= AIM_X_MAX) ? AIM_X_MAX : AIM_X_MIN;
    }

    if (aim.yp <= AIM_Y_MAX && aim.yp >= AIM_Y_MIN) {
        aim.yp += 0.5f * aim.ys;
    }
    else {
        aim.ys = 0.f;
        aim.yp = (aim.yp >= AIM_Y_MAX) ? AIM_Y_MAX : AIM_Y_MIN;
    }
}

void updateTwoAxis(AimState& aim, float dt) {
    float lo = (aim.axisPhase == 0) ? AIM_X_MIN : AIM_Y_MIN;
    float hi = (aim.axisPhase == 0) ? AIM_X_MAX : AIM_Y_MAX;

    aim.axisVal += aim.axisDir * aim.axisSpeed * dt;
    if (aim.axisVal >= hi) { aim.axisVal = hi; aim.axisDir = -1.f; }
    if (aim.axisVal <= lo) { aim.axisVal = lo; aim.axisDir = 1.f; }

    if (aim.axisPhase == 0) {
        aim.xp = aim.axisVal;
        aim.yp = BOARD_CENTER_Y; // keep something sane to draw before Y is chosen
    }
    else {
        aim.yp = aim.axisVal;
    }
}

bool confirmTwoAxis(AimState& aim) {
    if (aim.axisPhase == 0) {
        aim.xp = aim.axisVal;
        aim.axisPhase = 1;
        aim.axisVal = AIM_Y_MIN;
        aim.axisDir = 1.f;
        return false; // X locked, still need Y
    }
    else {
        aim.yp = aim.axisVal;
        return true; // both locked - dart is thrown
    }
}

// Picks the dartboard "wedge" number for a given angle (radians, 0..2PI),
// matching a 20-wedge layout, PI/10 wide each.
static int wedgeNumber(double ang) {
    static const int numbers[20] = {
        6,13,4,18,1,20,5,12,9,14,11,8,16,7,19,3,17,2,15,10
    };
    int idx = static_cast<int>(std::lround(ang / (PI / 10.0))) % 20;
    if (idx < 0) idx += 20;
    return numbers[idx];
}

ScoreResult calculateScore(float x, float y) {
    constexpr double BULL_RADIUS = 7;
    constexpr double BULLSEYE_RADIUS = 18;
    constexpr double INNER_RING_RADIUS = 98;
    constexpr double INNER_RING_RADIUS2 = 112;
    constexpr double OUTER_RING_RADIUS = 170;
    constexpr double OUTER_RING_RADIUS2 = 186;

    float tempX = x - BOARD_CENTER_X;
    float tempY = BOARD_CENTER_Y - y;
    double distance = std::sqrt(static_cast<double>(tempX) * tempX + static_cast<double>(tempY) * tempY);
    double ang = std::atan2(tempY, tempX);
    if (ang < 0) ang += 2.0 * PI;

    ScoreResult result;

    if (distance <= BULL_RADIUS) {
        result.score = 50;
        result.label = "BULLSEYE!";
        return result;
    }
    if (distance <= BULLSEYE_RADIUS) {
        result.score = 25;
        result.label = "Bull";
        return result;
    }

    int base = wedgeNumber(ang);

    if (distance <= INNER_RING_RADIUS2) {
        if (distance > INNER_RING_RADIUS) {
            result.score = base * 3;
            result.label = "Triple " + std::to_string(base);
        }
        else {
            result.score = base;
            result.label = "Single " + std::to_string(base);
        }
        return result;
    }
    if (distance <= OUTER_RING_RADIUS2) {
        if (distance > OUTER_RING_RADIUS) {
            result.score = base * 2;
            result.label = "Double " + std::to_string(base);
        }
        else {
            result.score = base;
            result.label = "Single " + std::to_string(base);
        }
        return result;
    }

    result.score = 0;
    result.label = "Miss";
    return result;
}

ThrowResult applyDartScore(Player& player, int dartScore, int visitStartScore) {
    int tentative = player.score + dartScore;
    if (tentative > TARGET_SCORE) {
        // Bust: real darts rule - the whole visit is voided, score reverts.
        player.score = visitStartScore;
        return ThrowResult::Bust;
    }
    if (tentative == TARGET_SCORE) {
        player.score = tentative;
        return ThrowResult::Win;
    }
    player.score = tentative;
    return ThrowResult::Normal;
}