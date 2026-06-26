#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include "Game.h"

using namespace sf;
using namespace std;

// ---------------------------------------------------------------------------
// Small rendering helpers
// ---------------------------------------------------------------------------

static Text makeText(const Font& font, const string& s, unsigned size, Color color) {
    Text t;
    t.setFont(font);
    t.setString(s);
    t.setCharacterSize(size);
    t.setFillColor(color);
    return t;
}

static void centerOrigin(Text& t) {
    FloatRect b = t.getLocalBounds();
    t.setOrigin(b.left + b.width / 2.0f, b.top + b.height / 2.0f);
}

// Draws a vector dartboard so the game is fully playable even if Dartboard.jpg
// is missing. If the image loads fine, this is never called.
static void drawDartboardFallback(RenderWindow& window) {
    RectangleShape backdrop(Vector2f(800, 600));
    backdrop.setFillColor(Color(235, 230, 215));
    window.draw(backdrop);

    struct Ring { float radius; Color color; };
    vector<Ring> rings = {
        {186, Color(20, 20, 20)},
        {170, Color(235, 230, 215)},
        {112, Color(20, 20, 20)},
        {98,  Color(200, 40, 40)},
        {18,  Color(20, 20, 20)},
        {7,   Color(200, 40, 40)},
    };
    for (size_t i = 0; i < rings.size(); ++i) {
        CircleShape c(rings[i].radius);
        c.setOrigin(rings[i].radius, rings[i].radius);
        c.setPosition(BOARD_CENTER_X, BOARD_CENTER_Y);
        c.setFillColor(rings[i].color);
        window.draw(c);
    }
    // Alternate single-area wedge tint (purely cosmetic) on the two single bands
    static const int numbers[20] = { 6,13,4,18,1,20,5,12,9,14,11,8,16,7,19,3,17,2,15,10 };
    for (int i = 0; i < 20; ++i) {
        double a0 = (i - 0.5) * (3.14159265 / 10.0);
        double midA = i * (3.14159265 / 10.0);
        bool alt = (i % 2 == 0);
        Color wedgeColor = alt ? Color(40, 40, 40) : Color(235, 230, 215);
        // thin wedge between single bands (98-112 triple ring already drawn solid; this is just the outer single ring tint)
        ConvexShape wedge;
        wedge.setPointCount(4);
        float r1 = 112.f, r2 = 170.f;
        double a1 = a0, a2 = a0 + 3.14159265 / 10.0;
        wedge.setPoint(0, Vector2f(BOARD_CENTER_X + r1 * cos(a1), BOARD_CENTER_Y - r1 * sin(a1)));
        wedge.setPoint(1, Vector2f(BOARD_CENTER_X + r2 * cos(a1), BOARD_CENTER_Y - r2 * sin(a1)));
        wedge.setPoint(2, Vector2f(BOARD_CENTER_X + r2 * cos(a2), BOARD_CENTER_Y - r2 * sin(a2)));
        wedge.setPoint(3, Vector2f(BOARD_CENTER_X + r1 * cos(a2), BOARD_CENTER_Y - r1 * sin(a2)));
        wedge.setFillColor(wedgeColor);
        window.draw(wedge);
        (void)midA; (void)numbers[i];
    }
}

static bool tryLoadFont(Font& font, const string& preferredPath) {
    if (font.loadFromFile(preferredPath)) return true;
    static const char* fallbacks[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };
    for (auto p : fallbacks) {
        if (font.loadFromFile(p)) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Menu drawing - returns nothing, just renders the given options with the
// selected one highlighted and gently pulsing.
// ---------------------------------------------------------------------------

static void drawMenu(RenderWindow& window, Font& font, const string& title,
    const string& subtitle, const vector<string>& options,
    int selected, float t) {
    RectangleShape bg(Vector2f(800, 600));
    bg.setFillColor(Color(18, 22, 28));
    window.draw(bg);

    // soft decorative rings echoing a dartboard, top-right corner
    for (int i = 0; i < 4; ++i) {
        float r = 60.f + i * 35.f;
        CircleShape ring(r);
        ring.setOrigin(r, r);
        ring.setPosition(700, 120);
        ring.setFillColor(Color::Transparent);
        ring.setOutlineThickness(2.f);
        ring.setOutlineColor(Color(200, 50, 50, 60));
        window.draw(ring);
    }

    Text titleText = makeText(font, title, 54, Color(235, 235, 235));
    titleText.setStyle(Text::Bold);
    centerOrigin(titleText);
    titleText.setPosition(400, 90);
    // drop shadow
    Text shadow = titleText;
    shadow.setFillColor(Color(0, 0, 0, 140));
    shadow.setPosition(404, 94);
    window.draw(shadow);
    window.draw(titleText);

    if (!subtitle.empty()) {
        Text sub = makeText(font, subtitle, 18, Color(170, 175, 185));
        centerOrigin(sub);
        sub.setPosition(400, 145);
        window.draw(sub);
    }

    float startY = 250.f;
    float gap = 56.f;
    for (size_t i = 0; i < options.size(); ++i) {
        bool isSelected = (static_cast<int>(i) == selected);
        float scale = isSelected ? 1.0f + 0.04f * sin(t * 6.0f) : 1.0f;

        Text opt = makeText(font, options[i], 28, isSelected ? Color(255, 210, 80) : Color(200, 200, 205));
        centerOrigin(opt);
        opt.setPosition(400, startY + gap * i);
        opt.setScale(scale, scale);
        window.draw(opt);

        if (isSelected) {
            float bob = 6.0f * sin(t * 6.0f);
            Text arrowL = makeText(font, ">", 28, Color(255, 210, 80));
            arrowL.setPosition(400 - opt.getGlobalBounds().width / 2.0f - 36.0f + bob * 0.2f, startY + gap * i - 18);
            Text arrowR = makeText(font, "<", 28, Color(255, 210, 80));
            arrowR.setPosition(400 + opt.getGlobalBounds().width / 2.0f + 18.0f - bob * 0.2f, startY + gap * i - 18);
            window.draw(arrowL);
            window.draw(arrowR);
        }
    }

    Text hint = makeText(font, "Up/Down to choose   Enter to confirm   Esc to go back", 14, Color(120, 125, 135));
    centerOrigin(hint);
    hint.setPosition(400, 560);
    window.draw(hint);
}

// ---------------------------------------------------------------------------
// Credits screen
// ---------------------------------------------------------------------------

static void drawCredits(RenderWindow& window, Font& font, float t) {
    RectangleShape bg(Vector2f(800, 600));
    bg.setFillColor(Color(18, 22, 28));
    window.draw(bg);

    for (int i = 0; i < 3; ++i) {
        float r = 70.f + i * 40.f;
        CircleShape ring(r);
        ring.setOrigin(r, r);
        ring.setPosition(400, 300);
        ring.setFillColor(Color::Transparent);
        ring.setOutlineThickness(2.f);
        ring.setOutlineColor(Color(200, 50, 50, 35));
        window.draw(ring);
    }

    Text title = makeText(font, "CREDITS", 50, Color(235, 235, 235));
    title.setStyle(Text::Bold);
    centerOrigin(title);
    title.setPosition(400, 90);
    Text shadow = title;
    shadow.setFillColor(Color(0, 0, 0, 140));
    shadow.setPosition(404, 94);
    window.draw(shadow);
    window.draw(title);

    Text sub = makeText(font, "Made by", 18, Color(170, 175, 185));
    centerOrigin(sub);
    sub.setPosition(400, 155);
    window.draw(sub);

    static const char* names[] = {
        "Kabeer Ahmed Shahzeb",
        "Abdul Ahad Shams",
        "M. Babar Shahzad",
        "Muhammad Anas"
    };

    float startY = 230.f;
    float gap = 56.f;
    for (int i = 0; i < 4; ++i) {
        float bob = 3.0f * sin(t * 3.0f + i * 0.6f);

        Text nameText = makeText(font, names[i], 26, Color(255, 210, 80));
        nameText.setStyle(Text::Bold);
        centerOrigin(nameText);
        nameText.setPosition(400, startY + gap * i + bob);
        window.draw(nameText);

        // small dart-tip flourish on either side, matching the in-game dot
        float halfW = nameText.getGlobalBounds().width / 2.0f + 26.0f;
        CircleShape dotL(4.f);
        dotL.setOrigin(4.f, 4.f);
        dotL.setFillColor(Color::Magenta);
        dotL.setPosition(400 - halfW, startY + gap * i + bob);
        CircleShape dotR = dotL;
        dotR.setPosition(400 + halfW, startY + gap * i + bob);
        window.draw(dotL);
        window.draw(dotR);
    }

    Text hint = makeText(font, "Press ENTER or Esc to return to the menu", 14, Color(120, 125, 135));
    centerOrigin(hint);
    hint.setPosition(400, 560);
    window.draw(hint);
}

// ---------------------------------------------------------------------------
// Sidebar scoreboard shown during aiming / results
// ---------------------------------------------------------------------------

static void drawSidebar(RenderWindow& window, Font& font, const vector<Player>& players,
    int currentPlayer, AimMode mode) {
    RectangleShape panel(Vector2f(800, 44));
    panel.setFillColor(Color(15, 15, 20, 235));
    panel.setPosition(0, 0);
    window.draw(panel);

    float x = 16.f;
    for (size_t i = 0; i < players.size(); ++i) {
        bool active = (static_cast<int>(i) == currentPlayer);
        string label = players[i].name + ": " + to_string(players[i].score);
        Text t = makeText(font, label, 16, active ? Color(255, 210, 80) : Color(200, 200, 200));
        t.setStyle(active ? Text::Bold : Text::Regular);
        t.setPosition(x, 12);
        window.draw(t);
        x += t.getGlobalBounds().width + 30.f;
    }

    string modeStr = (mode == AimMode::FreeRoam) ? "Mode: Classic Aim" : "Mode: Two-Axis";
    Text m = makeText(font, modeStr, 13, Color(140, 145, 155));
    m.setPosition(800 - m.getGlobalBounds().width - 14, 14);
    // We can't know width before draw call positioning logic above without measuring twice; recompute:
    FloatRect mb = m.getLocalBounds();
    m.setPosition(800 - mb.width - 16, 14);
    window.draw(m);
}

// ---------------------------------------------------------------------------
int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    RenderWindow window(VideoMode(800, 600), "Darts");
    window.setFramerateLimit(120);

    Font font;
    if (!tryLoadFont(font, "Arimo.ttf")) {
        cerr << "Error loading font (tried Arimo.ttf and system fallbacks)" << endl;
        return -1;
    }

    Texture backgroundTexture;
    bool hasBoardImage = backgroundTexture.loadFromFile("Dartboard.jpg");
    Sprite backgroundSprite;
    if (hasBoardImage) {
        backgroundSprite.setTexture(backgroundTexture);
        Vector2u windowSize = window.getSize();
        Vector2u imageSize = backgroundTexture.getSize();
        backgroundSprite.setPosition(
            (windowSize.x - imageSize.x) / 2.0f,
            (windowSize.y - imageSize.y) / 2.0f
        );
    }
    else {
        cerr << "Dartboard.jpg not found - using a built-in vector dartboard instead." << endl;
    }

    // ---------------- App state ----------------
    AppState state = AppState::MainMenu;

    int mainMenuSel = 0;
    vector<string> mainMenuOptions = { "Play", "Credits", "Quit" };

    int playerCountSel = 0; // 0 -> 2 players, 1 -> 3, 2 -> 4
    vector<string> playerCountOptions = { "2 Players", "3 Players", "4 Players" };

    int gameModeSel = 0;
    vector<string> gameModeOptions = { "Classic Free Aim", "Two-Axis Precision" };

    vector<Player> players;
    int currentPlayer = 0;
    int dartsThisVisit = 0;
    int visitStartScore = 0;
    vector<ScoreResult> visitThrows;
    AimMode aimMode = AimMode::FreeRoam;
    AimState aim;
    ThrowResult lastResult = ThrowResult::Normal;
    string winnerName;

    Clock globalClock;
    Clock stateClock; // time since entering current transient state (ThrowAnim/ThrowFlash)

    auto startNewGame = [&]() {
        players.clear();
        int n = playerCountSel + MIN_PLAYERS;
        for (int i = 0; i < n; ++i) {
            Player p;
            p.name = "Player " + to_string(i + 1);
            p.score = 0;
            players.push_back(p);
        }
        currentPlayer = 0;
        dartsThisVisit = 0;
        visitStartScore = players[0].score;
        visitThrows.clear();
        aim.reset(aimMode);
        state = AppState::Aiming;
        };

    auto beginNextVisit = [&]() {
        dartsThisVisit = 0;
        visitStartScore = players[currentPlayer].score;
        visitThrows.clear();
        aim.reset(aimMode);
        state = AppState::Aiming;
        };

    auto finalizeThrow = [&]() {
        ScoreResult sr = calculateScore(aim.xp, aim.yp);
        visitThrows.push_back(sr);
        ThrowResult tr = applyDartScore(players[currentPlayer], sr.score, visitStartScore);
        lastResult = tr;
        dartsThisVisit++;
        stateClock.restart();
        state = AppState::ThrowAnim;
        };

    while (window.isOpen()) {
        float dt = 1.0f / 120.0f;
        float t = globalClock.getElapsedTime().asSeconds();

        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) window.close();

            if (event.type == Event::KeyPressed) {
                switch (state) {
                case AppState::MainMenu:
                    if (event.key.code == Keyboard::Up || event.key.code == Keyboard::Down) {
                        mainMenuSel = (mainMenuSel + 1) % static_cast<int>(mainMenuOptions.size());
                    }
                    else if (event.key.code == Keyboard::Enter) {
                        if (mainMenuSel == 0) state = AppState::PlayerCountMenu;
                        else if (mainMenuSel == 1) state = AppState::Credits;
                        else window.close();
                    }
                    break;

                case AppState::Credits:
                    if (event.key.code == Keyboard::Enter || event.key.code == Keyboard::Escape) {
                        state = AppState::MainMenu;
                    }
                    break;

                case AppState::PlayerCountMenu:
                    if (event.key.code == Keyboard::Up || event.key.code == Keyboard::Down) {
                        playerCountSel = (playerCountSel + 1) % static_cast<int>(playerCountOptions.size());
                    }
                    else if (event.key.code == Keyboard::Enter) {
                        state = AppState::GameModeMenu;
                    }
                    else if (event.key.code == Keyboard::Escape) {
                        state = AppState::MainMenu;
                    }
                    break;

                case AppState::GameModeMenu:
                    if (event.key.code == Keyboard::Up || event.key.code == Keyboard::Down) {
                        gameModeSel = (gameModeSel + 1) % static_cast<int>(gameModeOptions.size());
                    }
                    else if (event.key.code == Keyboard::Enter) {
                        aimMode = (gameModeSel == 0) ? AimMode::FreeRoam : AimMode::TwoAxis;
                        startNewGame();
                    }
                    else if (event.key.code == Keyboard::Escape) {
                        state = AppState::PlayerCountMenu;
                    }
                    break;

                case AppState::Aiming:
                    if (event.key.code == Keyboard::Escape) {
                        state = AppState::MainMenu;
                    }
                    else if (event.key.code == Keyboard::Space) {
                        if (aimMode == AimMode::FreeRoam) {
                            finalizeThrow();
                        }
                        else {
                            bool thrown = confirmTwoAxis(aim);
                            if (thrown) finalizeThrow();
                        }
                    }
                    break;

                case AppState::VisitResult:
                    if (event.key.code == Keyboard::Enter) {
                        currentPlayer = (currentPlayer + 1) % static_cast<int>(players.size());
                        beginNextVisit();
                    }
                    else if (event.key.code == Keyboard::Escape) {
                        state = AppState::MainMenu;
                    }
                    break;

                case AppState::GameOver:
                    if (event.key.code == Keyboard::Enter) {
                        state = AppState::MainMenu;
                    }
                    else if (event.key.code == Keyboard::Escape) {
                        window.close();
                    }
                    break;

                default: break;
                }
            }
        }

        // ---------------- Per-frame state updates ----------------
        if (state == AppState::Aiming) {
            if (aimMode == AimMode::FreeRoam) updateFreeRoam(aim, dt);
            else updateTwoAxis(aim, dt);
        }
        else if (state == AppState::ThrowAnim) {
            if (stateClock.getElapsedTime().asSeconds() > 0.35f) {
                bool visitOver = (lastResult != ThrowResult::Normal) || (dartsThisVisit >= DARTS_PER_VISIT);
                if (lastResult == ThrowResult::Win) {
                    winnerName = players[currentPlayer].name;
                    state = AppState::GameOver;
                }
                else if (visitOver) {
                    state = AppState::VisitResult;
                }
                else {
                    stateClock.restart();
                    state = AppState::ThrowFlash;
                }
            }
        }
        else if (state == AppState::ThrowFlash) {
            if (stateClock.getElapsedTime().asSeconds() > 0.8f) {
                aim.reset(aimMode); // ready for next dart in the same visit
                state = AppState::Aiming;
            }
        }

        // ---------------- Rendering ----------------
        window.clear(Color::White);

        switch (state) {
        case AppState::MainMenu:
            drawMenu(window, font, "DARTS", "A classic game of precision", mainMenuOptions, mainMenuSel, t);
            break;

        case AppState::Credits:
            drawCredits(window, font, t);
            break;

        case AppState::PlayerCountMenu:
            drawMenu(window, font, "How many players?", "Choose 2 to 4 players", playerCountOptions, playerCountSel, t);
            break;

        case AppState::GameModeMenu:
            drawMenu(window, font, "Choose your style", "How would you like to aim?", gameModeOptions, gameModeSel, t);
            break;

        case AppState::Aiming:
        case AppState::ThrowAnim:
        case AppState::ThrowFlash: {
            if (hasBoardImage) window.draw(backgroundSprite);
            else drawDartboardFallback(window);

            if (state == AppState::Aiming && aimMode == AimMode::TwoAxis) {
                // draw sweeping guide line for whichever axis is active
                if (aim.axisPhase == 0) {
                    RectangleShape line(Vector2f(2.f, AIM_Y_MAX - AIM_Y_MIN));
                    line.setFillColor(Color(255, 0, 255, 200));
                    line.setPosition(aim.xp - 1.f, AIM_Y_MIN);
                    window.draw(line);
                }
                else {
                    RectangleShape vline(Vector2f(2.f, AIM_Y_MAX - AIM_Y_MIN));
                    vline.setFillColor(Color(255, 0, 255, 90));
                    vline.setPosition(aim.xp - 1.f, AIM_Y_MIN);
                    window.draw(vline);

                    RectangleShape line(Vector2f(AIM_X_MAX - AIM_X_MIN, 2.f));
                    line.setFillColor(Color(255, 0, 255, 200));
                    line.setPosition(AIM_X_MIN, aim.yp - 1.f);
                    window.draw(line);
                }
            }
            else {
                float size = 5.0f;
                CircleShape dot(size);
                dot.setPosition(aim.xp - size, aim.yp - size);
                dot.setFillColor(Color::Magenta);
                window.draw(dot);
            }

            if (state == AppState::ThrowAnim) {
                float elapsed = stateClock.getElapsedTime().asSeconds();
                float radius = 6.f + elapsed * 60.f;
                int alpha = static_cast<int>(255 * (1.0f - elapsed / 0.35f));
                alpha = max(0, alpha);
                CircleShape ring(radius);
                ring.setOrigin(radius, radius);
                ring.setPosition(aim.xp, aim.yp);
                ring.setFillColor(Color::Transparent);
                ring.setOutlineThickness(3.f);
                Color ringColor = (lastResult == ThrowResult::Bust) ? Color(220, 40, 40, alpha)
                    : (lastResult == ThrowResult::Win) ? Color(255, 210, 80, alpha)
                    : Color(255, 255, 255, alpha);
                ring.setOutlineColor(ringColor);
                window.draw(ring);
            }

            if (state == AppState::ThrowFlash && !visitThrows.empty()) {
                ScoreResult sr = visitThrows.back();
                Text flash = makeText(font, sr.label + "  (" + to_string(sr.score) + ")", 26, Color(255, 230, 120));
                flash.setStyle(Text::Bold);
                centerOrigin(flash);
                flash.setPosition(400, 300);
                Text shadow = flash;
                shadow.setFillColor(Color(0, 0, 0, 160));
                shadow.setPosition(403, 303);
                window.draw(shadow);
                window.draw(flash);

                Text dartCount = makeText(font, "Dart " + to_string(dartsThisVisit) + " of " + to_string(DARTS_PER_VISIT), 16, Color(230, 230, 230));
                centerOrigin(dartCount);
                dartCount.setPosition(400, 335);
                window.draw(dartCount);
            }

            drawSidebar(window, font, players, currentPlayer, aimMode);

            string instr = (aimMode == AimMode::FreeRoam)
                ? "Arrows to steer  -  Space to throw"
                : (aim.axisPhase == 0 ? "Space to lock the vertical line" : "Space to lock the horizontal line");
            if (state == AppState::Aiming) {
                Text hint = makeText(font, instr, 14, Color(70, 70, 70));
                hint.setPosition(16, 570);
                window.draw(hint);
            }
            break;
        }

        case AppState::VisitResult: {
            if (hasBoardImage) window.draw(backgroundSprite);
            else drawDartboardFallback(window);
            drawSidebar(window, font, players, currentPlayer, aimMode);

            RectangleShape panel(Vector2f(560, 260));
            panel.setOrigin(280, 130);
            panel.setPosition(400, 320);
            panel.setFillColor(Color(10, 10, 14, 225));
            panel.setOutlineThickness(2.f);
            panel.setOutlineColor(Color(255, 210, 80, 180));
            window.draw(panel);

            string headline;
            Color headColor;
            if (lastResult == ThrowResult::Bust) {
                headline = "BUST! Score stays at " + to_string(visitStartScore);
                headColor = Color(230, 70, 70);
            }
            else {
                headline = players[currentPlayer].name + "'s visit";
                headColor = Color(255, 210, 80);
            }
            Text head = makeText(font, headline, 26, headColor);
            head.setStyle(Text::Bold);
            centerOrigin(head);
            head.setPosition(400, 230);
            window.draw(head);

            float ly = 270.f;
            int visitTotal = 0;
            for (size_t i = 0; i < visitThrows.size(); ++i) {
                string line = "Dart " + to_string(i + 1) + ": " + visitThrows[i].label +
                    " = " + to_string(visitThrows[i].score);
                visitTotal += visitThrows[i].score;
                Text lt = makeText(font, line, 18, Color(220, 220, 225));
                centerOrigin(lt);
                lt.setPosition(400, ly);
                window.draw(lt);
                ly += 28.f;
            }

            if (lastResult != ThrowResult::Bust) {
                Text total = makeText(font, "Visit total: " + to_string(visitTotal) +
                    "   |   New score: " + to_string(players[currentPlayer].score), 16, Color(180, 200, 255));
                centerOrigin(total);
                total.setPosition(400, ly + 6);
                window.draw(total);
            }

            Text hint = makeText(font, "Press ENTER for next player's turn  -  Esc for menu", 14, Color(160, 160, 165));
            centerOrigin(hint);
            hint.setPosition(400, 420);
            window.draw(hint);
            break;
        }

        case AppState::GameOver: {
            RectangleShape bg(Vector2f(800, 600));
            bg.setFillColor(Color(12, 14, 18));
            window.draw(bg);

            for (int i = 0; i < 24; ++i) {
                float ang = t * 1.2f + i * (2.f * 3.14159265f / 24.f);
                float orbit = 220.f + 12.f * sin(t * 2.f + i);
                float cx = 400 + orbit * cos(ang);
                float cy = 150 + orbit * 0.35f * sin(ang);
                CircleShape spark(2.5f);
                spark.setPosition(cx, cy);
                spark.setFillColor(Color(255, 210, 80, 180));
                window.draw(spark);
            }

            float pulse = 1.0f + 0.06f * sin(t * 3.0f);
            Text win = makeText(font, winnerName + " WINS!", 48, Color(255, 210, 80));
            win.setStyle(Text::Bold);
            centerOrigin(win);
            win.setPosition(400, 170);
            win.setScale(pulse, pulse);
            Text winShadow = win;
            winShadow.setFillColor(Color(0, 0, 0, 160));
            winShadow.setPosition(405, 175);
            window.draw(winShadow);
            window.draw(win);

            Text sub = makeText(font, "Checked out exactly on " + to_string(TARGET_SCORE), 18, Color(190, 195, 205));
            centerOrigin(sub);
            sub.setPosition(400, 225);
            window.draw(sub);

            float ly = 290.f;
            vector<Player> sorted = players;
            std::sort(sorted.begin(), sorted.end(), [](const Player& a, const Player& b) { return a.score > b.score; });
            for (size_t i = 0; i < sorted.size(); ++i) {
                bool isWinner = (sorted[i].name == winnerName);
                string line = sorted[i].name + "  -  " + to_string(sorted[i].score);
                Text lt = makeText(font, line, 20, isWinner ? Color(255, 210, 80) : Color(200, 200, 205));
                lt.setStyle(isWinner ? Text::Bold : Text::Regular);
                centerOrigin(lt);
                lt.setPosition(400, ly);
                window.draw(lt);
                ly += 32.f;
            }

            Text hint = makeText(font, "Press ENTER to return to the menu  -  Esc to quit", 14, Color(150, 150, 158));
            centerOrigin(hint);
            hint.setPosition(400, 560);
            window.draw(hint);
            break;
        }
        }

        window.display();
    }

    return 0;
}