#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// ---------------- Tunables ----------------
constexpr int   TARGET_SCORE = 301;   // classic "301" darts
constexpr int   MIN_PLAYERS = 2;
constexpr int   MAX_PLAYERS = 4;
constexpr int   DARTS_PER_VISIT = 3;     // 3 darts per turn, like real darts

constexpr float BOARD_CENTER_X = 400.f;
constexpr float BOARD_CENTER_Y = 300.f;
constexpr float AIM_X_MIN = 150.f, AIM_X_MAX = 650.f;
constexpr float AIM_Y_MIN = 50.f, AIM_Y_MAX = 550.f;

// ---------------- App state machine ----------------
enum class AppState {
    MainMenu,
    Credits,
    PlayerCountMenu,
    GameModeMenu,
    Aiming,
    ThrowAnim,    // brief automatic flight/impact animation
    ThrowFlash,   // brief automatic "dart score" popup between darts in a visit
    VisitResult,  // end-of-visit summary, waits for player input
    GameOver
};

enum class AimMode { FreeRoam, TwoAxis };

enum class ThrowResult { Normal, Bust, Win };

// ---------------- Data ----------------
struct Player {
    std::string name;
    int score = 0;
};

struct AimState {
    // Free-roam (classic) mode: a dot that drifts/wanders and is nudged by arrow keys
    float xp = BOARD_CENTER_X, yp = BOARD_CENTER_Y;
    float xs = 0.f, ys = 0.f;

    // Two-axis mode: first a vertical line sweeps left<->right (choosing X),
    // then a horizontal line sweeps up<->down (choosing Y).
    int   axisPhase = 0;          // 0 = choosing X, 1 = choosing Y
    float axisVal = AIM_X_MIN;  // current sweeping position
    float axisDir = 1.f;        // +1 or -1
    float axisSpeed = 260.f;      // px/sec

    void reset(AimMode mode);
};

struct ScoreResult {
    int score = 0;
    std::string label;   // e.g. "Triple 20", "Bullseye", "Miss"
};

// ---------------- Functions ----------------
void updateFreeRoam(AimState& aim, float dt);
void updateTwoAxis(AimState& aim, float dt);

// Confirms the current axis in TwoAxis mode. Returns true once BOTH axes are
// locked (i.e. the dart has actually been thrown).
bool confirmTwoAxis(AimState& aim);

ScoreResult calculateScore(float x, float y);

// Applies a dart's score to a player's running total, honouring the
// "no busting over the target" darts rule. visitStartScore is the player's
// score at the start of THIS visit (used to revert on a bust).
ThrowResult applyDartScore(Player& player, int dartScore, int visitStartScore);