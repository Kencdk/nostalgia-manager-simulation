#include "App.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <map>
#include <set>

#include "imgui.h"
#include "TeamOverview.h"

namespace nm {

namespace {
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

bool contains(const std::string& hay, const std::string& needle) {
    if (needle.empty()) return true;
    return lower(hay).find(lower(needle)) != std::string::npos;
}

// Pull the running score "(h-a)" out of a goal event string, if present.
bool parseScore(const std::string& text, int& h, int& a) {
    size_t open = text.rfind('(');
    size_t dash = text.find('-', open == std::string::npos ? 0 : open);
    size_t close = text.find(')', dash == std::string::npos ? 0 : dash);
    if (open == std::string::npos || dash == std::string::npos ||
        close == std::string::npos || dash < open || close < dash)
        return false;
    try {
        h = std::stoi(text.substr(open + 1, dash - open - 1));
        a = std::stoi(text.substr(dash + 1, close - dash - 1));
        return true;
    } catch (...) {
        return false;
    }
}

double playerOverall(const Player& p) { return PlayerAbility(p); }

// "Michael Laudrup" -> "M.Laudrup" (first initial + surname).
std::string shortName(const std::string& full) {
    size_t sp = full.find(' ');
    if (sp == std::string::npos || sp == 0) return full;
    std::string surname = full.substr(sp + 1);
    while (!surname.empty() && surname.front() == ' ') surname.erase(surname.begin());
    if (surname.empty()) return full;
    return std::string(1, full[0]) + "." + surname;
}

// "DR, WBR, MR" - every position the player can fill.
std::string playablePosStr(const Player& p) {
    std::string s;
    for (Position pos : p.playablePositions) {
        if (!s.empty()) s += ", ";
        s += PosName(pos);
    }
    return s.empty() ? PosName(p.primaryPos) : s;
}

// Hover tooltip on a player row listing natural + alternative positions.
void positionTooltip(const Player& p) {
    if (!ImGui::IsItemHovered()) return;
    ImGui::BeginTooltip();
    ImGui::Text("Natural: %s", PosName(p.primaryPos).c_str());
    ImGui::Text("Can play: %s", playablePosStr(p).c_str());
    ImGui::EndTooltip();
}

// Shade an RGBA colour by a multiplier (clamped), keeping alpha.
ImU32 shade(ImU32 c, float m) {
    ImVec4 v = ImGui::ColorConvertU32ToFloat4(c);
    auto cl = [](float x) { return x < 0 ? 0.f : (x > 1 ? 1.f : x); };
    return ImGui::ColorConvertFloat4ToU32(ImVec4(cl(v.x * m), cl(v.y * m), cl(v.z * m), v.w));
}

// Parse a color name to ImU32 (RGBA). Supports common color names.
ImU32 parseColor(const std::string& colorName) {
    std::string lower = colorName;
    for (char& c : lower) c = std::tolower(c);

    // Common football kit colors
    if (lower == "red") return IM_COL32(220, 50, 50, 255);
    if (lower == "blue") return IM_COL32(50, 100, 220, 255);
    if (lower == "green") return IM_COL32(50, 180, 50, 255);
    if (lower == "yellow") return IM_COL32(255, 220, 50, 255);
    if (lower == "orange") return IM_COL32(255, 140, 50, 255);
    if (lower == "purple" || lower == "violet") return IM_COL32(150, 50, 200, 255);
    if (lower == "white") return IM_COL32(240, 240, 240, 255);
    if (lower == "black") return IM_COL32(40, 40, 40, 255);
    if (lower == "grey" || lower == "gray") return IM_COL32(150, 150, 150, 255);
    if (lower == "pink") return IM_COL32(255, 150, 180, 255);
    if (lower == "brown") return IM_COL32(140, 90, 50, 255);
    if (lower == "navy") return IM_COL32(30, 40, 100, 255);
    if (lower == "skyblue" || lower == "sky blue" || lower == "lightblue" || lower == "light blue") 
        return IM_COL32(135, 206, 250, 255);
    if (lower == "darkblue" || lower == "dark blue") return IM_COL32(20, 50, 150, 255);
    if (lower == "darkgreen" || lower == "dark green") return IM_COL32(30, 100, 30, 255);
    if (lower == "lightgreen" || lower == "light green") return IM_COL32(144, 238, 144, 255);
    if (lower == "maroon") return IM_COL32(128, 0, 0, 255);
    if (lower == "teal") return IM_COL32(0, 128, 128, 255);
    if (lower == "gold") return IM_COL32(255, 215, 0, 255);
    if (lower == "silver") return IM_COL32(192, 192, 192, 255);

    // Default to white if color not recognized
    return IM_COL32(240, 240, 240, 255);
}

// A glossy, beveled coloured button matching the start-menu art.
bool tintButton(const char* label, ImU32 base, const ImVec2& size) {
    ImGui::PushStyleColor(ImGuiCol_Button, base);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, shade(base, 1.25f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, shade(base, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_Border, shade(base, 1.7f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
    bool r = ImGui::Button(label, size);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(5);
    return r;
}

// Section header inside a panel: a coloured bar with centred title.
void panelHeader(const char* title, ImU32 col = IM_COL32(120, 70, 40, 255)) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    float h = ImGui::GetTextLineHeight() + 10;
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), col, 4.0f);
    dl->AddRect(p, ImVec2(p.x + w, p.y + h), IM_COL32(210, 170, 90, 255), 4.0f, 0, 1.5f);
    ImVec2 ts = ImGui::CalcTextSize(title);
    dl->AddText(ImVec2(p.x + (w - ts.x) * 0.5f, p.y + 5), IM_COL32(245, 225, 170, 255), title);
    ImGui::Dummy(ImVec2(w, h + 4));
}

// Squad list panel (Starting XI then Substitutes), as in the match mock-up.
void squadPanel(const char* id, const char* title, const Team* t, const ImVec2& size,
                const std::map<int, App::PlayerMatchStats>* stats = nullptr,
                std::function<void(const Player*)> onPlayerClick = nullptr) {
    ImGui::BeginChild(id, size, true);
    panelHeader(title);
    if (!t) {
        ImGui::TextDisabled("-");
        ImGui::EndChild();
        return;
    }
    // Show the configured Starting XI (in selection order) first, then the rest
    // as substitutes, so the side reflects any tactics changes.
    std::vector<const Player*> starters, subs;
    for (int pid : t->startingXI)
        for (const auto& p : t->squad)
            if (p.id == pid) { starters.push_back(&p); break; }
    for (const auto& p : t->squad) {
        bool isStarter = std::find(t->startingXI.begin(), t->startingXI.end(), p.id) !=
                         t->startingXI.end();
        if (!isStarter) subs.push_back(&p);
    }

    // Helper to calculate rating from stats
    auto calculateRating = [](const App::PlayerMatchStats& ps) -> float {
        if (ps.minutesPlayed < 1) return 0.0f;

        float rating = 6.0f;  // Base rating

        // Goals are very valuable
        rating += ps.goals * 1.5f;

        // Assists
        rating += ps.assists * 1.0f;

        // Shots
        if (ps.shots > 0) {
            float shotAccuracy = (float)ps.shotsOnTarget / ps.shots;
            rating += (ps.shots * 0.1f) + (shotAccuracy * 0.5f);
        }

        // Passing
        if (ps.passes > 0) {
            float passAccuracy = (float)ps.passesCompleted / ps.passes;
            rating += (passAccuracy - 0.7f) * 2.0f;  // Bonus/penalty around 70% baseline
        }

        // Defensive actions
        rating += ps.tackles * 0.2f;
        rating += ps.interceptions * 0.2f;

        // Fouls penalty
        rating -= ps.fouls * 0.3f;

        // Clamp between 1-10
        if (rating < 1.0f) rating = 1.0f;
        if (rating > 10.0f) rating = 10.0f;

        return rating;
    };

    // Column headers
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.86f, 0.78f, 0.55f, 1));
    ImGui::Columns(4, "player_headers", true);
    ImGui::SetColumnWidth(0, size.x * 0.50f);  // Name column
    ImGui::SetColumnWidth(1, size.x * 0.15f);  // Goals
    ImGui::SetColumnWidth(2, size.x * 0.15f);  // Assists
    ImGui::SetColumnWidth(3, size.x * 0.20f);  // Rating
    ImGui::Text("Name");
    ImGui::NextColumn();
    ImGui::Text("G");
    ImGui::NextColumn();
    ImGui::Text("A");
    ImGui::NextColumn();
    ImGui::Text("Rating");
    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::Columns(1);
    ImGui::PopStyleColor();

    auto line = [stats, calculateRating, onPlayerClick](const Player* p) {
        ImGui::Columns(4, "player_stats", true);
        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.50f);
        ImGui::SetColumnWidth(1, ImGui::GetWindowWidth() * 0.15f);
        ImGui::SetColumnWidth(2, ImGui::GetWindowWidth() * 0.15f);
        ImGui::SetColumnWidth(3, ImGui::GetWindowWidth() * 0.20f);

        // Name - make it clickable if callback provided
        char nameLabel[256];
        std::snprintf(nameLabel, sizeof(nameLabel), "%2d %-3s %s", 
                     p->shirtNumber, PosName(p->primaryPos).c_str(),
                     shortName(p->name).c_str());

        if (onPlayerClick) {
            if (ImGui::Selectable(nameLabel, false)) {
                onPlayerClick(p);
            }
        } else {
            ImGui::Text("%s", nameLabel);
        }
        ImGui::NextColumn();

        // Goals
        if (stats) {
            auto it = stats->find(p->shirtNumber);
            if (it != stats->end() && it->second.goals > 0) {
                ImGui::Text("%d", it->second.goals);
            } else {
                ImGui::Text("-");
            }
        } else {
            ImGui::Text("-");
        }
        ImGui::NextColumn();

        // Assists
        if (stats) {
            auto it = stats->find(p->shirtNumber);
            if (it != stats->end() && it->second.assists > 0) {
                ImGui::Text("%d", it->second.assists);
            } else {
                ImGui::Text("-");
            }
        } else {
            ImGui::Text("-");
        }
        ImGui::NextColumn();

        // Rating
        if (stats) {
            auto it = stats->find(p->shirtNumber);
            if (it != stats->end() && it->second.minutesPlayed > 0) {
                float rating = calculateRating(it->second);
                ImGui::Text("%.1f", rating);
            } else {
                ImGui::Text("-");
            }
        } else {
            ImGui::Text("-");
        }
        ImGui::NextColumn();

        ImGui::Columns(1);
    };

    ImGui::TextColored(ImVec4(0.86f, 0.78f, 0.55f, 1), "Starting XI");
    for (const Player* p : starters) line(p);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.86f, 0.78f, 0.55f, 1), "Substitutes");
    int shown = 0;
    for (const Player* p : subs) {
        if (shown++ >= 5) break;
        line(p);
    }
    ImGui::EndChild();
}

// Selectable formations offered on the Tactics screen.
const char* const kFormations[] = {"4-4-2",   "4-3-3", "3-5-2",   "4-5-1",
                                   "5-3-2",   "4-4-1-1", "3-4-3", "4-2-3-1",
                                   "5-4-1",   "4-1-4-1"};

// Best starter for a given attribute (used to derive set-piece takers / captain).
const Player* bestStarterFor(Team* t, const char* attr, bool outfieldOnly) {
    const Player* best = nullptr;
    double bestVal = -1.0;
    for (int pid : t->startingXI) {
        const Player* p = t->findPlayer(pid);
        if (!p) continue;
        if (outfieldOnly && p->role == Role::GK) continue;
        double v = p->attr.get(attr);
        if (v > bestVal) { bestVal = v; best = p; }
    }
    return best;
}

// One "label: value" row inside a tactics info panel.
void tacticRow(const char* label, const std::string& value) {
    ImGui::TextColored(ImVec4(0.78f, 0.70f, 0.50f, 1), "%s", label);
    ImGui::SameLine(150);
    ImGui::TextColored(ImVec4(0.95f, 0.92f, 0.82f, 1), "%s", value.c_str());
}
}  // namespace

void ApplyNostalgiaTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0.0f;
    s.FrameRounding = 5.0f;
    s.GrabRounding = 4.0f;
    s.ChildRounding = 6.0f;
    s.PopupRounding = 5.0f;
    s.FrameBorderSize = 1.0f;
    s.WindowBorderSize = 0.0f;
    s.FramePadding = ImVec2(10, 6);
    s.ItemSpacing = ImVec2(10, 8);
    s.ScrollbarSize = 14.0f;

    ImVec4* c = s.Colors;
    auto col = [](int r, int g, int b, int a = 255) {
        return ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
    };
    c[ImGuiCol_WindowBg]        = col(24, 30, 22);       // dark pitch green
    c[ImGuiCol_ChildBg]         = col(34, 30, 24, 235);  // leather
    c[ImGuiCol_PopupBg]         = col(28, 26, 22, 245);
    c[ImGuiCol_Border]          = col(120, 92, 48);      // muted gold
    c[ImGuiCol_FrameBg]         = col(48, 42, 32);
    c[ImGuiCol_FrameBgHovered]  = col(64, 56, 40);
    c[ImGuiCol_FrameBgActive]   = col(74, 64, 46);
    c[ImGuiCol_TitleBgActive]   = col(40, 34, 26);
    c[ImGuiCol_Button]          = col(58, 78, 52);
    c[ImGuiCol_ButtonHovered]   = col(78, 104, 66);
    c[ImGuiCol_ButtonActive]    = col(46, 62, 42);
    c[ImGuiCol_Header]          = col(70, 60, 40);
    c[ImGuiCol_HeaderHovered]   = col(96, 80, 50);
    c[ImGuiCol_HeaderActive]    = col(110, 90, 56);
    c[ImGuiCol_TableHeaderBg]   = col(60, 50, 34);
    c[ImGuiCol_TableRowBg]      = col(40, 36, 28);
    c[ImGuiCol_TableRowBgAlt]   = col(46, 42, 32);
    c[ImGuiCol_TableBorderStrong] = col(120, 92, 48);
    c[ImGuiCol_TableBorderLight]  = col(80, 64, 40);
    c[ImGuiCol_Text]            = col(238, 232, 214);
    c[ImGuiCol_TextDisabled]    = col(150, 140, 120);
    c[ImGuiCol_CheckMark]       = col(220, 180, 90);
    c[ImGuiCol_SliderGrab]      = col(200, 160, 80);
    c[ImGuiCol_SliderGrabActive]= col(230, 190, 100);
    c[ImGuiCol_ScrollbarGrab]   = col(90, 76, 50);
    c[ImGuiCol_Separator]       = col(120, 92, 48);
}

bool App::init(const std::string& dataDir) {
    dataDir_ = dataDir;
    ApplyNostalgiaTheme();
    cfg_.loadFile(dataDir_ + "/engine.cfg");
    if (!db_.load(dataDir_)) {
        status_ = "Failed to load database from " + dataDir_;
        return false;
    }
    AppLoadTexture(dataDir_ + "/images/menu_bg.png", &menuBg_);

    // Load decorative screenshots for Friendly Match screen
    // Screenshots should be named 1.png, 2.png, 3.png, etc.
    for (int i = 1; i <= 20; ++i) {  // Try loading up to 20 images
        AppTexture screenshot;
        std::string pngPath = dataDir_ + "/images/teams/" + std::to_string(i) + ".png";
        std::string jpgPath = dataDir_ + "/images/teams/" + std::to_string(i) + ".jpg";

        if (AppLoadTexture(pngPath, &screenshot)) {
            friendlyScreenshots_.push_back(screenshot);
            printf("Loaded screenshot %d from %s\n", i, pngPath.c_str());
        } else if (AppLoadTexture(jpgPath, &screenshot)) {
            friendlyScreenshots_.push_back(screenshot);
            printf("Loaded screenshot %d from %s\n", i, jpgPath.c_str());
        } else {
            // Stop when we hit the first missing number
            if (i == 1) {
                printf("No screenshots found in data/images/teams/\n");
            }
            break;
        }
    }

    if (!friendlyScreenshots_.empty()) {
        printf("Total screenshots loaded for Friendly screen: %d\n", 
               (int)friendlyScreenshots_.size());
    }

    leagues_ = db_.leagues();
    status_ = "Loaded " + std::to_string(db_.teams.size()) + " teams across " +
              std::to_string(leagues_.size()) + " leagues.";
    return true;
}

Team* App::teamById(int id) { return db_.findTeam(id); }

void App::beginScreen(const char* title) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("##screen", nullptr, flags);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120, 230, 150, 255));
    ImGui::SetWindowFontScale(1.6f);
    ImGui::TextUnformatted(title);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
}

// Helper function to draw cycling screenshot backgrounds on any screen
void App::drawCyclingBackground() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 pos = vp->WorkPos;
    const ImVec2 size = vp->WorkSize;
    ImDrawList* bg = ImGui::GetBackgroundDrawList();

    if (!friendlyScreenshots_.empty()) {
        // Auto-cycle through screenshots
        static float carouselTimer = 0.0f;
        static int currentScreenshot = 0;

        carouselTimer += ImGui::GetIO().DeltaTime;
        if (carouselTimer >= 3.0f) {  // Change image every 3 seconds
            carouselTimer = 0.0f;
            currentScreenshot = (currentScreenshot + 1) % friendlyScreenshots_.size();
        }

        const AppTexture& screenshot = friendlyScreenshots_[currentScreenshot];

        if (screenshot.ok && screenshot.w > 0 && screenshot.h > 0) {
            // Scale image to cover the window (same logic as main menu background)
            float ws = size.x / size.y;
            float is = (float)screenshot.w / (float)screenshot.h;
            ImVec2 uv0(0, 0), uv1(1, 1);

            if (ws > is) {  // window wider than image: crop top/bottom
                float v = is / ws;
                uv0.y = (1 - v) * 0.5f;
                uv1.y = 1 - uv0.y;
            } else {  // crop left/right
                float u = ws / is;
                uv0.x = (1 - u) * 0.5f;
                uv1.x = 1 - uv0.x;
            }

            // Draw background image covering full window
            bg->AddImage(screenshot.id, pos, ImVec2(pos.x + size.x, pos.y + size.y), uv0, uv1);

            // Add semi-transparent dark overlay so text is readable
            bg->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), 
                             IM_COL32(0, 0, 0, 120));
        }
    } else {
        // Fallback: gradient background if no screenshots
        bg->AddRectFilledMultiColor(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                                    IM_COL32(20, 40, 70, 255), IM_COL32(20, 40, 70, 255),
                                    IM_COL32(10, 20, 35, 255), IM_COL32(10, 20, 35, 255));
    }
}

void App::beginFullscreen(const char* id, bool withBackground) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoScrollbar;
    if (!withBackground) flags |= ImGuiWindowFlags_NoBackground;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(id, nullptr, flags);
    ImGui::PopStyleVar();
}

void App::render() {
    switch (screen_) {
        case Screen::Main: renderMain(); break;
        case Screen::Friendly: renderFriendly(); break;
        case Screen::Tactics: renderTactics(); break;
        case Screen::Match: renderMatch(); break;
        case Screen::Database: renderDatabase(); break;
        case Screen::Career: renderCareer(); break;
        case Screen::Data: renderData(); break;
        case Screen::About: renderAbout(); break;
        case Screen::PlayerDetail: renderPlayerDetail(); break;
        case Screen::TeamOverview: renderTeamOverview(); break;
    }
}

void App::renderMain() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 pos = vp->WorkPos;
    const ImVec2 size = vp->WorkSize;

    // Background: the stadium key-art, scaled to "cover" the window.
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    if (menuBg_.ok && menuBg_.w > 0 && menuBg_.h > 0) {
        float ws = size.x / size.y, is = (float)menuBg_.w / (float)menuBg_.h;
        ImVec2 uv0(0, 0), uv1(1, 1);
        if (ws > is) {  // window wider than image: crop top/bottom
            float v = is / ws;
            uv0.y = (1 - v) * 0.5f;
            uv1.y = 1 - uv0.y;
        } else {  // crop left/right
            float u = ws / is;
            uv0.x = (1 - u) * 0.5f;
            uv1.x = 1 - uv0.x;
        }
        bg->AddImage(menuBg_.id, pos, ImVec2(pos.x + size.x, pos.y + size.y), uv0, uv1);
    } else {
        bg->AddRectFilledMultiColor(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                                    IM_COL32(20, 40, 70, 255), IM_COL32(20, 40, 70, 255),
                                    IM_COL32(10, 20, 14, 255), IM_COL32(10, 20, 14, 255));
        ImVec2 ts = ImGui::CalcTextSize("Nostalgia Manager Simulation");
        bg->AddText(ImVec2(pos.x + (size.x - ts.x) * 0.5f, pos.y + size.y * 0.18f),
                    IM_COL32(245, 215, 120, 255), "Nostalgia Manager Simulation");
    }

    beginFullscreen("##main", false);

    // Centred vertical stack of glossy coloured buttons, as in the mock-up.
    const ImVec2 bsz(430, 58);
    const float gap = 14.0f;
    struct Item { const char* label; ImU32 col; Screen scr; };
    const Item items[] = {
        {"Friendly Match", IM_COL32(86, 150, 38, 255), Screen::Friendly},
        {"Career Game", IM_COL32(196, 150, 40, 255), Screen::Career},
        {"Load Game", IM_COL32(40, 92, 178, 255), Screen::Career},
        {"Edit Database", IM_COL32(180, 92, 30, 255), Screen::Database},
        {"Exit", IM_COL32(188, 42, 38, 255), Screen::Main},
    };
    const int n = (int)(sizeof(items) / sizeof(items[0]));
    float startY = pos.y + size.y * 0.46f;
    float startX = pos.x + (size.x - bsz.x) * 0.5f;

    ImGui::SetWindowFontScale(1.25f);
    for (int i = 0; i < n; ++i) {
        ImGui::SetCursorScreenPos(ImVec2(startX, startY + i * (bsz.y + gap)));
        if (tintButton(items[i].label, items[i].col, bsz)) {
            if (std::strcmp(items[i].label, "Exit") == 0) {
                quit_ = true;
            } else if (std::strcmp(items[i].label, "Load Game") == 0) {
                careerLoad();
            } else {
                screen_ = items[i].scr;
            }
        }
    }
    ImGui::SetWindowFontScale(1.0f);

    // Keep Data Sources / About reachable as small secondary links.
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 16, pos.y + size.y - 40));
    if (ImGui::Button("Data Sources")) screen_ = Screen::Data;
    ImGui::SameLine();
    if (ImGui::Button("About")) screen_ = Screen::About;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 st = ImGui::CalcTextSize(status_.c_str());
    dl->AddText(ImVec2(pos.x + size.x - st.x - 16, pos.y + size.y - 32),
                IM_COL32(230, 225, 205, 230), status_.c_str());

    ImGui::End();
}

void App::teamPicker(const char* id, int& leagueIdx, int& teamId, char* filter,
                     size_t filterSz) {
    ImGui::PushID(id);
    if (leagues_.empty()) {
        ImGui::TextDisabled("No leagues loaded.");
        ImGui::PopID();
        return;
    }
    if (leagueIdx < 0 || leagueIdx >= static_cast<int>(leagues_.size())) leagueIdx = 0;
    if (ImGui::BeginCombo("League", leagues_[leagueIdx].c_str())) {
        for (int i = 0; i < static_cast<int>(leagues_.size()); ++i) {
            bool sel = (i == leagueIdx);
            if (ImGui::Selectable(leagues_[i].c_str(), sel)) {
                leagueIdx = i;
                teamId = -1;
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::InputTextWithHint("Filter", "type to filter teams", filter, filterSz);

    std::vector<Team*> teams = db_.teamsInLeague(leagues_[leagueIdx]);
    std::sort(teams.begin(), teams.end(),
              [](Team* a, Team* b) { return a->name < b->name; });
    ImGui::BeginChild("teamlist", ImVec2(360, 240), true);
    for (Team* t : teams) {
        if (!contains(t->name, filter)) continue;
        char label[160];
        std::snprintf(label, sizeof(label), "%s  (%d players)", t->name.c_str(),
                      static_cast<int>(t->squad.size()));
        if (ImGui::Selectable(label, teamId == t->id)) teamId = t->id;
    }
    ImGui::EndChild();
    ImGui::PopID();
}

void App::renderFriendly() {
    // Draw cycling background
    drawCyclingBackground();

    beginScreen("Friendly Match");
    if (ImGui::Button("< Back")) screen_ = Screen::Main;
    ImGui::Spacing();

    ImGui::Columns(2, "sel", false);
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1, 1), "Home");
    teamPicker("home", homeLeague_, homeTeam_, homeFilter_, sizeof(homeFilter_));
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1, 0.7f, 0.5f, 1), "Away");
    teamPicker("away", awayLeague_, awayTeam_, awayFilter_, sizeof(awayFilter_));
    ImGui::Columns(1);

    ImGui::Spacing();
    Team* home = teamById(homeTeam_);
    Team* away = teamById(awayTeam_);
    ImGui::Text("Selected: %s  vs  %s",
                home ? home->name.c_str() : "(none)",
                away ? away->name.c_str() : "(none)");

    ImGui::Spacing();

    // Buttons to view team overviews
    if (home) {
        if (ImGui::Button("View Home Team", ImVec2(180, 32))) {
            openTeamOverview(home, Screen::Friendly);
        }
    }
    ImGui::SameLine();
    if (away) {
        if (ImGui::Button("View Away Team", ImVec2(180, 32))) {
            openTeamOverview(away, Screen::Friendly);
        }
    }

    ImGui::Spacing();

    bool ok = home && away && home->id != away->id && home->squad.size() >= 7 &&
              away->squad.size() >= 7;
    if (!ok) ImGui::BeginDisabled();
    if (tintButton("Continue to Tactics", IM_COL32(86, 150, 38, 255), ImVec2(220, 40)))
        openTactics(home, Screen::Friendly);
    if (!ok) ImGui::EndDisabled();
    if (home && away && home->id == away->id)
        ImGui::TextDisabled("Pick two different teams.");
    else if ((home && home->squad.size() < 7) || (away && away->squad.size() < 7))
        ImGui::TextDisabled("Both teams need at least 7 players.");

    ImGui::End();
}

void App::openTactics(Team* team, Screen returnTo) {
    tacticsTeam_ = team;
    tacticsReturn_ = returnTo;
    tacticsXiSel_ = -1;
    tacticsSubSel_ = -1;
    if (team) {
        // Set to club's preferred formation if not already set
        if (team->formation.empty()) {
            team->formation = team->preferredFormation;
        }
        // Only regenerate the starting XI if it's empty (first time opening)
        if (team->startingXI.empty()) {
            team->autoSelectXI();
        }
    }
    screen_ = Screen::Tactics;
}

void App::renderTactics() {
    // Draw cycling background
    drawCyclingBackground();

    beginScreen("Tactics");
    Team* t = tacticsTeam_;
    if (ImGui::Button("< Back")) { screen_ = tacticsReturn_; ImGui::End(); return; }
    if (!t) {
        ImGui::TextDisabled("No team selected.");
        ImGui::End();
        return;
    }
    ImGui::SameLine();
    ImGui::SetWindowFontScale(1.25f);
    ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.45f, 1), "%s", t->name.c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing();

    const ImVec4 gold(0.86f, 0.78f, 0.55f, 1);
    const bool inMatch = (tacticsReturn_ == Screen::Match);
    const bool subsLeft = !inMatch || matchSubsUsed_ < kMaxMatchSubs;
    float avail = ImGui::GetContentRegionAvail().y - 56;
    if (avail < 200) avail = 200;
    float leftW = 320.0f, rightW = 340.0f;
    float midW = ImGui::GetContentRegionAvail().x - leftW - rightW - 24;
    if (midW < 260) midW = 260;

    // ---- Left Panel: Squad List ----
    int dragSourceIdx = -1, dragTargetIdx = -1;
    int subPlayerDragId = -1;  // Player ID being dragged from subs/rest
    ImGui::BeginChild("tac_squad", ImVec2(leftW, avail), true);
    panelHeader("Squad");
    ImGui::TextColored(gold, "Starting XI");
    for (size_t i = 0; i < t->startingXI.size(); ++i) {
        Player* p = t->findPlayer(t->startingXI[i]);
        if (!p) continue;
        Position assignedPos = i < t->assignedPositions.size() ? t->assignedPositions[i] : p->primaryPos;
        char lbl[160];
        std::snprintf(lbl, sizeof(lbl), "%2d  %-3s %s", p->shirtNumber,
                      PosName(assignedPos).c_str(), shortName(p->name).c_str());

        ImGui::PushID(static_cast<int>(i));
        if (ImGui::Selectable(lbl, tacticsPlayerSel_ == p->id)) {
            tacticsPlayerSel_ = (tacticsPlayerSel_ == p->id) ? -1 : p->id;
        }
        // Double-click to view player details
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            openPlayerDetail(p, Screen::Tactics);
        }

        // Drag source - can drag from starting XI
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            int idx = static_cast<int>(i);
            ImGui::SetDragDropPayload("PLAYER_SLOT", &idx, sizeof(int));
            ImGui::Text("%s", p->name.c_str());
            ImGui::EndDragDropSource();
        }

        // Drop target - can drop starting XI players or substitutes
        if (ImGui::BeginDragDropTarget()) {
            // Accept drags from other starting XI positions
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PLAYER_SLOT")) {
                dragSourceIdx = *static_cast<const int*>(payload->Data);
                dragTargetIdx = static_cast<int>(i);
            }
            // Accept drags from substitutes/rest of squad
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SUB_PLAYER")) {
                subPlayerDragId = *static_cast<const int*>(payload->Data);
                dragTargetIdx = static_cast<int>(i);
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();
        positionTooltip(*p);
    }
    ImGui::Spacing();
    ImGui::TextColored(gold, "Substitutes");
    int swapStarter = -1, swapSub = -1;
    int shownSubs = 0;
    int subIdx = 0;
    int swapSubSource = -1, swapSubTarget = -1;  // For swapping subs with each other

    for (const Player& pl : t->squad) {
        bool starting = std::find(t->startingXI.begin(), t->startingXI.end(), pl.id) !=
                        t->startingXI.end();
        if (starting) continue;
        if (shownSubs >= 5) break;
        shownSubs++;
        char lbl[160];
        std::snprintf(lbl, sizeof(lbl), "%2d  %-3s %s", pl.shirtNumber,
                      PosName(pl.primaryPos).c_str(), shortName(pl.name).c_str());

        ImGui::PushID(100 + subIdx);  // Unique ID for subs
        if (ImGui::Selectable(lbl, tacticsXiSel_ == pl.id)) {
            if (tacticsXiSel_ != -1) { swapStarter = tacticsXiSel_; swapSub = pl.id; }
            else tacticsXiSel_ = (tacticsXiSel_ == pl.id) ? -1 : pl.id;
        }
        // Double-click to view player details
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            openPlayerDetail(&pl, Screen::Tactics);
        }

        // Make substitutes draggable
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            int subId = pl.id;
            ImGui::SetDragDropPayload("SUB_PLAYER", &subId, sizeof(int));
            ImGui::Text("%s", pl.name.c_str());
            ImGui::EndDragDropSource();
        }

        // Make substitutes accept drops from starting XI and other subs
        if (ImGui::BeginDragDropTarget()) {
            // Accept drags from starting XI
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PLAYER_SLOT")) {
                int starterIdx = *static_cast<const int*>(payload->Data);
                if (starterIdx >= 0 && starterIdx < static_cast<int>(t->startingXI.size())) {
                    swapStarter = t->startingXI[starterIdx];
                    swapSub = pl.id;
                }
            }
            // Accept drags from other substitutes (for reordering bench)
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SUB_PLAYER")) {
                int sourceSubId = *static_cast<const int*>(payload->Data);
                if (sourceSubId != pl.id) {
                    swapSubSource = sourceSubId;
                    swapSubTarget = pl.id;
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();
        positionTooltip(pl);
        subIdx++;
    }

    // Rest of Squad (players beyond the first 5 subs) - only show when NOT in match
    if (!inMatch) {
        ImGui::Spacing();
        ImGui::TextColored(gold, "Rest of Squad");
        int squadIdx = 0;
        int restIdx = 0;
        for (const Player& pl : t->squad) {
            bool starting = std::find(t->startingXI.begin(), t->startingXI.end(), pl.id) !=
                            t->startingXI.end();
            if (starting) continue;

            squadIdx++;
            if (squadIdx <= 5) continue;  // Skip the first 5 subs (already shown)

            char lbl[160];
            std::snprintf(lbl, sizeof(lbl), "%2d  %-3s %s", pl.shirtNumber,
                          PosName(pl.primaryPos).c_str(), shortName(pl.name).c_str());

            ImGui::PushID(200 + restIdx);  // Unique ID for rest of squad
            if (ImGui::Selectable(lbl, tacticsXiSel_ == pl.id)) {
                if (tacticsXiSel_ != -1) { swapStarter = tacticsXiSel_; swapSub = pl.id; }
                else tacticsXiSel_ = (tacticsXiSel_ == pl.id) ? -1 : pl.id;
            }
            // Double-click to view player details
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                openPlayerDetail(&pl, Screen::Tactics);
            }

            // Make rest of squad draggable
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                int subId = pl.id;
                ImGui::SetDragDropPayload("SUB_PLAYER", &subId, sizeof(int));
                ImGui::Text("%s", pl.name.c_str());
                ImGui::EndDragDropSource();
            }

            ImGui::PopID();
            positionTooltip(pl);
            restIdx++;
        }
    }

    if (swapStarter != -1 && swapSub != -1 && subsLeft) {
        auto it = std::find(t->startingXI.begin(), t->startingXI.end(), swapStarter);
        if (it != t->startingXI.end()) {
            *it = swapSub;
            if (inMatch) ++matchSubsUsed_;
        }
        tacticsXiSel_ = tacticsSubSel_ = -1;
    }

    // Handle swapping substitutes with each other (bench reordering)
    if (swapSubSource != -1 && swapSubTarget != -1) {
        // Find both players in the squad
        auto itSource = std::find_if(t->squad.begin(), t->squad.end(),
                                      [swapSubSource](const Player& p) { return p.id == swapSubSource; });
        auto itTarget = std::find_if(t->squad.begin(), t->squad.end(),
                                      [swapSubTarget](const Player& p) { return p.id == swapSubTarget; });

        // Swap their positions in the squad vector
        if (itSource != t->squad.end() && itTarget != t->squad.end()) {
            std::iter_swap(itSource, itTarget);
        }
    }

    if (inMatch) {
        ImGui::Spacing();
        ImVec4 c = subsLeft ? gold : ImVec4(0.9f, 0.5f, 0.4f, 1);
        ImGui::TextColored(c, "Subs: %d / %d", matchSubsUsed_, kMaxMatchSubs);
    }
    ImGui::EndChild();

    // ---- Centre Panel: Tactical Pitch with Formation Positions ----
    ImGui::SameLine();
    ImGui::BeginChild("tac_pitch", ImVec2(midW, avail), true);
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 o = ImGui::GetCursorScreenPos();
        ImVec2 sz = ImGui::GetContentRegionAvail();

        // Draw pitch
        dl->AddRectFilled(o, ImVec2(o.x + sz.x, o.y + sz.y), IM_COL32(74, 132, 62, 255), 4);
        int stripes = 10;
        for (int s = 0; s < stripes; ++s) {
            if (s % 2) continue;
            float y0 = o.y + sz.y * s / stripes;
            float y1 = o.y + sz.y * (s + 1) / stripes;
            dl->AddRectFilled(ImVec2(o.x, y0), ImVec2(o.x + sz.x, y1),
                              IM_COL32(82, 142, 68, 255));
        }
        ImU32 line = IM_COL32(225, 235, 220, 150);
        dl->AddRect(ImVec2(o.x + 8, o.y + 8), ImVec2(o.x + sz.x - 8, o.y + sz.y - 8),
                    line, 0, 0, 2.0f);
        dl->AddLine(ImVec2(o.x + 8, o.y + sz.y * 0.5f),
                    ImVec2(o.x + sz.x - 8, o.y + sz.y * 0.5f), line, 2.0f);
        dl->AddCircle(ImVec2(o.x + sz.x * 0.5f, o.y + sz.y * 0.5f),
                      sz.x * 0.12f, line, 32, 2.0f);

        float padX = 40.0f, padY = 34.0f;
        float drawWidth = sz.x - 2 * padX;
        float drawHeight = sz.y - 2 * padY;

        // Fixed position mapping for formation positions
        // xPos: lateral position (0 = left touchline, 0.5 = center, 1 = right touchline)
        // yPos: depth (0 = attack, 1 = defense)
        auto getPositionCoords = [](Position pos) -> std::pair<float, float> {
            float yPos = 0.5f, xPos = 0.5f;
            switch (pos) {
                // Forwards - wide players at quarter positions, center at 0.5
                case Position::FL:  yPos = 0.08f; xPos = 0.25f; break;
                case Position::FC:  yPos = 0.08f; xPos = 0.50f; break;
                case Position::FR:  yPos = 0.08f; xPos = 0.75f; break;

                // Attacking Midfielders
                case Position::AML: yPos = 0.28f; xPos = 0.20f; break;
                case Position::AMC: yPos = 0.28f; xPos = 0.50f; break;
                case Position::AMR: yPos = 0.28f; xPos = 0.80f; break;

                // Midfielders - wide at 0.15/0.85, center at 0.5
                case Position::ML:  yPos = 0.48f; xPos = 0.15f; break;
                case Position::MC:  yPos = 0.48f; xPos = 0.50f; break;
                case Position::MR:  yPos = 0.48f; xPos = 0.85f; break;

                // Defensive Midfielders
                case Position::DM:  yPos = 0.65f; xPos = 0.50f; break;

                // Defenders
                case Position::WBL: yPos = 0.74f; xPos = 0.12f; break;
                case Position::DL:  yPos = 0.80f; xPos = 0.22f; break;
                case Position::DC:  yPos = 0.83f; xPos = 0.50f; break;
                case Position::DR:  yPos = 0.80f; xPos = 0.78f; break;
                case Position::WBR: yPos = 0.74f; xPos = 0.88f; break;

                // Goalkeeper
                case Position::GK:  yPos = 0.95f; xPos = 0.50f; break;
            }
            return {xPos, yPos};
        };

        // Group positions with same coordinates
        std::map<Position, int> positionCount;
        for (size_t i = 0; i < t->assignedPositions.size(); ++i) {
            positionCount[t->assignedPositions[i]]++;
        }

        std::map<Position, int> positionIndex;
        const ImGuiPayload* dragPayload = ImGui::GetDragDropPayload();
        bool isDragging = dragPayload && (dragPayload->IsDataType("PLAYER_SLOT") || dragPayload->IsDataType("SUB_PLAYER"));

        // Draw each formation slot
        for (size_t slotIdx = 0; slotIdx < t->startingXI.size() && slotIdx < t->assignedPositions.size(); ++slotIdx) {
            Player* p = t->findPlayer(t->startingXI[slotIdx]);
            Position assignedPos = t->assignedPositions[slotIdx];

            auto [baseX, baseY] = getPositionCoords(assignedPos);

            // Handle multiple players at same position (spread horizontally)
            int totalAtPos = positionCount[assignedPos];
            int thisIndex = positionIndex[assignedPos]++;
            float xOffset = 0.0f;

            if (totalAtPos > 1) {
                // Determine if this is a central position (xPos near 0.5)
                bool isCentral = (baseX >= 0.35f && baseX <= 0.65f);

                if (isCentral) {
                    // CENTRAL PLAYERS: Spread symmetrically around center
                    if (totalAtPos == 2) {
                        // Two central players: left and right of center
                        xOffset = (thisIndex == 0) ? -0.15f : 0.15f;
                    } else if (totalAtPos == 3) {
                        // Three: left, center, right
                        float offsets[] = {-0.15f, 0.0f, 0.15f};
                        xOffset = offsets[thisIndex];
                    } else if (totalAtPos == 4) {
                        // Four: wider spread, skip exact center
                        float offsets[] = {-0.22f, -0.08f, 0.08f, 0.22f};
                        xOffset = offsets[thisIndex];
                    } else if (totalAtPos == 5) {
                        // Five: full central spread
                        float offsets[] = {-0.22f, -0.11f, 0.0f, 0.11f, 0.22f};
                        xOffset = offsets[thisIndex];
                    } else {
                        // More than 5: distribute evenly
                        float spacing = 0.44f / (totalAtPos - 1);
                        xOffset = (thisIndex * spacing) - 0.22f;
                    }
                } else {
                    // WIDE PLAYERS: Smaller spread, stay on their flank
                    if (totalAtPos == 2) {
                        xOffset = (thisIndex == 0) ? -0.08f : 0.08f;
                    } else if (totalAtPos == 3) {
                        float offsets[] = {-0.10f, 0.0f, 0.10f};
                        xOffset = offsets[thisIndex];
                    } else {
                        float spacing = 0.20f / (totalAtPos - 1);
                        xOffset = (thisIndex * spacing) - 0.10f;
                    }
                }
            }

            float xPos = std::max(0.05f, std::min(0.95f, baseX + xOffset));
            float x = o.x + padX + drawWidth * xPos;
            float y = o.y + padY + drawHeight * baseY;

            bool gk = RoleOf(assignedPos) == Role::GK;
            ImU32 jersey = gk ? IM_COL32(60, 150, 70, 255) : IM_COL32(46, 96, 176, 255);
            if (p && tacticsPlayerSel_ == p->id) jersey = IM_COL32(200, 140, 60, 255);
            float r = 21.0f;

            // Make position draggable
            ImGui::SetCursorScreenPos(ImVec2(x - r, y - r));
            ImGui::PushID(static_cast<int>(slotIdx) + 1000);  // Offset to avoid ID collision with list
            ImGui::InvisibleButton("pos", ImVec2(r * 2, r * 2));
            bool hovered = ImGui::IsItemHovered();
            bool clicked = ImGui::IsItemClicked();

            if (p && clicked) {
                tacticsPlayerSel_ = (tacticsPlayerSel_ == p->id) ? -1 : p->id;
            }

            // Drag source
            if (p && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                int srcIdx = static_cast<int>(slotIdx);
                ImGui::SetDragDropPayload("PLAYER_SLOT", &srcIdx, sizeof(int));
                ImGui::Text("%s", p->name.c_str());
                ImGui::EndDragDropSource();
            }

            // Drop target
            if (ImGui::BeginDragDropTarget()) {
                // Accept drags from other starting XI positions
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PLAYER_SLOT")) {
                    int srcIdx = *static_cast<const int*>(payload->Data);
                    int tgtIdx = static_cast<int>(slotIdx);
                    if (srcIdx != tgtIdx && dragSourceIdx == -1) {
                        dragSourceIdx = srcIdx;
                        dragTargetIdx = tgtIdx;
                    }
                }
                // Accept drags from substitutes/rest of squad
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SUB_PLAYER")) {
                    subPlayerDragId = *static_cast<const int*>(payload->Data);
                    dragTargetIdx = static_cast<int>(slotIdx);
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopID();

            // Visual feedback for drag
            if (isDragging && hovered) {
                dl->AddCircle(ImVec2(x, y), r + 4, IM_COL32(255, 220, 100, 255), 24, 3.0f);
            }

            if (p) {
                dl->AddCircleFilled(ImVec2(x, y), r, jersey, 24);
                dl->AddCircle(ImVec2(x, y), r, IM_COL32(225, 200, 120, 255), 24, 2.0f);

                // Get player tactics for forward run indicator
                PlayerTactics& pt = t->tactics.getPlayerTactics(p->id);
                if (pt.forwardRun != ForwardRun::None) {
                    float arrowLen = (pt.forwardRun == ForwardRun::Short) ? 15.0f : 25.0f;
                    ImVec2 arrowStart = ImVec2(x, y - r - 2);
                    ImVec2 arrowEnd = ImVec2(x, y - r - 2 - arrowLen);
                    dl->AddLine(arrowStart, arrowEnd, IM_COL32(255, 200, 80, 255), 2.5f);
                    dl->AddTriangleFilled(
                        ImVec2(arrowEnd.x, arrowEnd.y),
                        ImVec2(arrowEnd.x - 5, arrowEnd.y + 7),
                        ImVec2(arrowEnd.x + 5, arrowEnd.y + 7),
                        IM_COL32(255, 200, 80, 255));
                }

                char num[8];
                std::snprintf(num, sizeof(num), "%d", p->shirtNumber);
                ImVec2 ns = ImGui::CalcTextSize(num);
                dl->AddText(ImVec2(x - ns.x * 0.5f, y - ns.y * 0.5f),
                            IM_COL32(255, 255, 255, 255), num);

                // Position label
                std::string posLabel = PosName(assignedPos);
                ImVec2 ps = ImGui::CalcTextSize(posLabel.c_str());
                dl->AddText(ImVec2(x - ps.x * 0.5f, y - r - ps.y - 1),
                            IM_COL32(248, 214, 130, 255), posLabel.c_str());

                char nm[64];
                std::snprintf(nm, sizeof(nm), "%s", shortName(p->name).c_str());
                ImVec2 ms = ImGui::CalcTextSize(nm);
                float lx = x - ms.x * 0.5f - 4, ly = y + r + 3;
                dl->AddRectFilled(ImVec2(lx, ly), ImVec2(lx + ms.x + 8, ly + ms.y + 4),
                                  IM_COL32(20, 24, 18, 210), 3);
                dl->AddText(ImVec2(lx + 4, ly + 2), IM_COL32(238, 232, 214, 255), nm);
            } else {
                // Empty slot
                dl->AddCircle(ImVec2(x, y), r, IM_COL32(180, 180, 180, 150), 24, 2.0f);
                std::string posLabel = PosName(assignedPos);
                ImVec2 ps = ImGui::CalcTextSize(posLabel.c_str());
                dl->AddText(ImVec2(x - ps.x * 0.5f, y - ps.y * 0.5f),
                            IM_COL32(180, 180, 180, 200), posLabel.c_str());
            }
        }

        if (!inMatch) {
            ImVec2 tp = ImVec2(o.x + 10, o.y + sz.y - 25);
            dl->AddText(tp, IM_COL32(230, 230, 230, 200), "Drag players to swap or create custom positions");
        }

        // Make the entire pitch a drop target for creating custom positions
        ImGui::SetCursorScreenPos(o);
        ImGui::InvisibleButton("pitch_drop", sz);
        if (ImGui::BeginDragDropTarget()) {
            // Only accept drops from starting XI when creating custom positions
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PLAYER_SLOT")) {
                ImVec2 mousePos = ImGui::GetMousePos();
                // Calculate normalized position (0-1) within the drawable area
                float relX = (mousePos.x - o.x - padX) / drawWidth;
                float relY = (mousePos.y - o.y - padY) / drawHeight;
                relX = std::max(0.05f, std::min(0.95f, relX));
                relY = std::max(0.05f, std::min(0.95f, relY));

                int srcIdx = *static_cast<const int*>(payload->Data);

                // Determine the closest position based on Y coordinate
                Position newPos = Position::MC;  // Default
                if (relY < 0.15f) {
                    // Forward line
                    if (relX < 0.35f) newPos = Position::FL;
                    else if (relX > 0.65f) newPos = Position::FR;
                    else newPos = Position::FC;
                } else if (relY < 0.35f) {
                    // Attacking midfield line
                    if (relX < 0.3f) newPos = Position::AML;
                    else if (relX > 0.7f) newPos = Position::AMR;
                    else newPos = Position::AMC;
                } else if (relY < 0.58f) {
                    // Midfield line
                    if (relX < 0.25f) newPos = Position::ML;
                    else if (relX > 0.75f) newPos = Position::MR;
                    else newPos = Position::MC;
                } else if (relY < 0.72f) {
                    // Defensive midfield
                    newPos = Position::DM;
                } else if (relY < 0.88f) {
                    // Defense line
                    if (relX < 0.2f) newPos = Position::WBL;
                    else if (relX > 0.8f) newPos = Position::WBR;
                    else if (relX < 0.35f) newPos = Position::DL;
                    else if (relX > 0.65f) newPos = Position::DR;
                    else newPos = Position::DC;
                } else {
                    // Goalkeeper
                    newPos = Position::GK;
                }

                // Check if this creates a new position not in current formation
                bool positionExists = false;
                for (size_t i = 0; i < t->assignedPositions.size(); ++i) {
                    if (i != srcIdx && t->assignedPositions[i] == newPos) {
                        positionExists = true;
                        break;
                    }
                }

                if (!positionExists || t->assignedPositions[srcIdx] != newPos) {
                    // This is a custom position change
                    t->assignedPositions[srcIdx] = newPos;
                    // Update player role to match the new position
                    t->updatePlayerRoles();
                    // Mark formation as custom
                    if (t->tactics.formation.find(" Custom") == std::string::npos) {
                        t->tactics.formation = t->tactics.formation + " Custom";
                        t->formation = t->tactics.formation;
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }
    ImGui::EndChild();

    // Apply position swap - swap players but keep positions fixed to formation
    if (dragSourceIdx != -1 && dragTargetIdx != -1 && dragSourceIdx != dragTargetIdx) {
        // Only swap the player IDs - the assigned positions stay with their formation slots
        std::swap(t->startingXI[dragSourceIdx], t->startingXI[dragTargetIdx]);
        // Note: We do NOT swap assignedPositions - each slot keeps its position
        // This means when you drag a CB to an MC slot, the CB now plays as MC
        t->updatePlayerRoles();  // Update roles to match new positions
    }

    // Apply substitute drop - replace starting XI player with substitute
    if (subPlayerDragId != -1 && dragTargetIdx != -1) {
        // Replace the player at dragTargetIdx with the substitute
        t->startingXI[dragTargetIdx] = subPlayerDragId;
        // The substitute takes on the position of the slot they're dropped into
        // No need to modify assignedPositions - the slot keeps its position
        if (inMatch) ++matchSubsUsed_;
        t->updatePlayerRoles();  // Update roles to match new positions
    }

    // ---- Right Panel: Formation & Team Tactics ----
    ImGui::SameLine();
    ImGui::BeginChild("tac_right", ImVec2(rightW, avail), false);

    ImGui::BeginChild("tac_formation", ImVec2(0, avail * 0.35f), true);
    panelHeader("Formation");
    tacticRow("Current:", t->tactics.formation);
    ImGui::Spacing();
    if (tintButton("Change Formation", IM_COL32(60, 130, 60, 255), ImVec2(-1, 34))) {
        // Strip " Custom" suffix to find base formation
        std::string baseFormation = t->tactics.formation;
        const std::string customSuffix = " Custom";
        if (baseFormation.size() >= customSuffix.size() && 
            baseFormation.substr(baseFormation.size() - customSuffix.size()) == customSuffix) {
            baseFormation = baseFormation.substr(0, baseFormation.size() - customSuffix.size());
        }

        int n = static_cast<int>(sizeof(kFormations) / sizeof(kFormations[0]));
        int cur = 0;
        for (int i = 0; i < n; ++i)
            if (baseFormation == kFormations[i]) { cur = i; break; }
        t->tactics.formation = kFormations[(cur + 1) % n];
        t->formation = t->tactics.formation;
        if (!inMatch) {
            t->autoSelectXI();
            tacticsXiSel_ = tacticsSubSel_ = tacticsPlayerSel_ = -1;
        } else {
            t->updateFormationPositions();
        }
    }
    ImGui::EndChild();

    ImGui::BeginChild("tac_team_instructions", ImVec2(0, avail * 0.35f), true);
    panelHeader("Team Instructions");

    // Passing Style
    if (ImGui::Button(PassingStyleName(t->tactics.passingStyle).c_str(), ImVec2(-1, 28))) {
        int val = static_cast<int>(t->tactics.passingStyle);
        t->tactics.passingStyle = static_cast<PassingStyle>((val + 1) % 3);
    }
    ImGui::TextDisabled("Passing Style");
    ImGui::Spacing();

    // Tackling Style
    if (ImGui::Button(TacklingStyleName(t->tactics.tacklingStyle).c_str(), ImVec2(-1, 28))) {
        int val = static_cast<int>(t->tactics.tacklingStyle);
        t->tactics.tacklingStyle = static_cast<TacklingStyle>((val + 1) % 3);
    }
    ImGui::TextDisabled("Tackling");
    ImGui::Spacing();

    // Pressing
    if (ImGui::Button(PressingLevelName(t->tactics.pressing).c_str(), ImVec2(-1, 28))) {
        int val = static_cast<int>(t->tactics.pressing);
        t->tactics.pressing = static_cast<PressingLevel>((val + 1) % 3);
    }
    ImGui::TextDisabled("Pressing");
    ImGui::Spacing();

    // Counter Attack
    if (ImGui::Checkbox("Counter Attack", &t->tactics.counterAttack)) {}
    ImGui::Spacing();

    // Offside Trap
    if (ImGui::Checkbox("Offside Trap", &t->tactics.offsideTrap)) {}

    ImGui::EndChild();

    ImGui::BeginChild("tac_player_instr", ImVec2(0, 0), true);
    panelHeader("Player Instructions");
    if (tacticsPlayerSel_ != -1) {
        Player* selPlayer = t->findPlayer(tacticsPlayerSel_);
        if (selPlayer) {
            ImGui::TextColored(gold, "%s", selPlayer->name.c_str());
            ImGui::Spacing();

            PlayerTactics& pt = t->tactics.getPlayerTactics(selPlayer->id);

            ImGui::Text("Forward Runs:");
            if (ImGui::Button(ForwardRunName(pt.forwardRun).c_str(), ImVec2(-1, 28))) {
                int val = static_cast<int>(pt.forwardRun);
                pt.forwardRun = static_cast<ForwardRun>((val + 1) % 3);
            }
        }
    } else {
        ImGui::TextDisabled("Select a player");
    }
    ImGui::EndChild();
    ImGui::EndChild();

    // ---- Action bar ----
    ImGui::Spacing();
    if (inMatch) ImGui::BeginDisabled();
    if (ImGui::Button("Auto-pick XI", ImVec2(160, 40))) {
        t->autoSelectXI();
        tacticsXiSel_ = tacticsSubSel_ = tacticsPlayerSel_ = -1;
    }
    if (inMatch) ImGui::EndDisabled();
    ImGui::SameLine();
    if (inMatch) ImGui::BeginDisabled();
    if (ImGui::Button("Reset Instructions", ImVec2(160, 40))) {
        // Clear all player-specific instructions
        for (auto& pt : t->tactics.playerSettings) {
            pt.forwardRun = ForwardRun::None;
        }
    }
    if (inMatch) ImGui::EndDisabled();
    ImGui::SameLine();
    if (tacticsReturn_ == Screen::Friendly) {
        Team* away = teamById(awayTeam_);
        bool ok = away && t->id != away->id;
        if (!ok) ImGui::BeginDisabled();
        if (tintButton("Kick Off", IM_COL32(86, 150, 38, 255), ImVec2(220, 40)))
            startMatch(t, away);
        if (!ok) ImGui::EndDisabled();
    } else if (tacticsReturn_ == Screen::Match) {
        if (tintButton("Resume Match", IM_COL32(70, 120, 150, 255), ImVec2(220, 40)))
            screen_ = Screen::Match;
    } else {
        if (tintButton("Back to Career", IM_COL32(196, 150, 40, 255), ImVec2(220, 40)))
            screen_ = Screen::Career;
    }

    ImGui::End();
}

void App::startMatch(Team* home, Team* away) {
    frames_.clear();
    playIdx_ = 0;
    playAccum_ = 0.0;
    playing_ = true;
    matchOver_ = false;
    halftimePause_ = false;
    halftimeIdx_ = 0;
    matchHome_ = home->name;
    matchAway_ = away->name;
    matchHomeTeam_ = home;
    matchAwayTeam_ = away;
    matchSubsUsed_ = 0;
    homeScorers_.clear();
    awayScorers_.clear();
    homePlayerStats_.clear();
    awayPlayerStats_.clear();

    // Initialize player stats for starting XI
    for (int pid : home->startingXI) {
        const Player* p = home->findPlayer(pid);
        if (p) {
            homePlayerStats_[p->shirtNumber] = PlayerMatchStats();
        }
    }
    for (int pid : away->startingXI) {
        const Player* p = away->findPlayer(pid);
        if (p) {
            awayPlayerStats_[p->shirtNumber] = PlayerMatchStats();
        }
    }

    MatchEngine engine(cfg_, static_cast<unsigned>(std::time(nullptr)));
    MatchEngine* ep = &engine;
    auto hook = [this, ep](const MatchEvent& e) {
        Frame f;
        f.text = e.text;
        f.key = e.key;
        f.minute = static_cast<int>(e.minute);  // already absolute 0-90
        f.pitch = ep->renderPitch();
        f.ballX = ep->ballX();  // Continuous position
        f.ballY = ep->ballY();  // Continuous position
        f.ballCol = ep->ballCol();  // Grid for compatibility
        f.ballRow = ep->ballRow();  // Grid for compatibility
        f.carrier = ep->carrierSide();
        f.stats = ep->stats();

        // Capture player positions
        auto playerSnaps = ep->getPlayerPositions();
        for (const auto& ps : playerSnaps) {
            Frame::PlayerPos pp;
            pp.shirtNumber = ps.shirtNumber;
            pp.x = ps.x;  // Continuous position
            pp.y = ps.y;  // Continuous position
            pp.col = ps.col;  // Grid for compatibility
            pp.row = ps.row;  // Grid for compatibility
            pp.side = ps.side;
            pp.isCarrier = ps.isCarrier;
            f.players.push_back(pp);
        }

        frames_.push_back(std::move(f));
    };
    MatchResult r = engine.simulate(*home, *away, false, hook);
    finalHG_ = r.homeGoals;
    finalAG_ = r.awayGoals;
    finalHS_ = r.homeShots;
    finalAS_ = r.awayShots;

    // Process events to track player stats and collect scorers
    int hg = 0, ag = 0;
    int lastMinute = 0;

    for (auto& f : frames_) {
        // Track minutes played for all active players
        if (f.minute > lastMinute) {
            for (auto& ps : homePlayerStats_) ps.second.minutesPlayed++;
            for (auto& ps : awayPlayerStats_) ps.second.minutesPlayed++;
            lastMinute = f.minute;
        }

        // Process goals and assists
        int h, a;
        if (f.text.rfind("GOAL!", 0) == 0 && parseScore(f.text, h, a)) {
            // Extract scorer: "GOAL! #<n> <Name> scores! (h-a)" or "GOAL! #<n> <Name> (assist: #<m> <AssistName>) scores! (h-a)"
            std::string who;
            int shirtNum = -1;
            int assistShirtNum = -1;

            // Find shirt number after "GOAL! #"
            size_t hashPos = f.text.find('#');
            if (hashPos != std::string::npos) {
                size_t numEnd = hashPos + 1;
                while (numEnd < f.text.size() && std::isdigit(f.text[numEnd])) numEnd++;
                if (numEnd > hashPos + 1) {
                    shirtNum = std::stoi(f.text.substr(hashPos + 1, numEnd - hashPos - 1));
                }
            }

            // Extract scorer name
            size_t s = f.text.find(' ');
            size_t e = f.text.find(" scores!");
            if (s != std::string::npos && e != std::string::npos && e > s) {
                who = f.text.substr(s + 1, e - s - 1);
                // Remove assist part if present
                size_t assistPos = who.find(" (assist:");
                if (assistPos != std::string::npos) {
                    // Extract assist shirt number
                    size_t assistHashPos = who.find('#', assistPos);
                    if (assistHashPos != std::string::npos) {
                        size_t assistNumEnd = assistHashPos + 1;
                        while (assistNumEnd < who.size() && std::isdigit(who[assistNumEnd])) 
                            assistNumEnd++;
                        if (assistNumEnd > assistHashPos + 1) {
                            assistShirtNum = std::stoi(who.substr(assistHashPos + 1, 
                                                       assistNumEnd - assistHashPos - 1));
                        }
                    }
                    who = who.substr(0, assistPos);
                }
            }

            char line[160];
            std::snprintf(line, sizeof(line), "%s  %d'", who.c_str(), f.minute);

            if (h > hg) {
                homeScorers_.emplace_back(f.minute, line);
                if (shirtNum >= 0) {
                    homePlayerStats_[shirtNum].goals++;
                    homePlayerStats_[shirtNum].shotsOnTarget++;
                }
                if (assistShirtNum >= 0) homePlayerStats_[assistShirtNum].assists++;
            } else if (a > ag) {
                awayScorers_.emplace_back(f.minute, line);
                if (shirtNum >= 0) {
                    awayPlayerStats_[shirtNum].goals++;
                    awayPlayerStats_[shirtNum].shotsOnTarget++;
                }
                if (assistShirtNum >= 0) awayPlayerStats_[assistShirtNum].assists++;
            }
            hg = h;
            ag = a;
        }

        // Parse other events for stats
        std::string& text = f.text;

        // Shots: "shoots" or "SHOT"
        if (text.find("shoots") != std::string::npos || text.find("SHOT") != std::string::npos) {
            size_t hashPos = text.find('#');
            if (hashPos != std::string::npos) {
                size_t numEnd = hashPos + 1;
                while (numEnd < text.size() && std::isdigit(text[numEnd])) numEnd++;
                if (numEnd > hashPos + 1) {
                    int shirtNum = std::stoi(text.substr(hashPos + 1, numEnd - hashPos - 1));
                    if (homePlayerStats_.find(shirtNum) != homePlayerStats_.end()) {
                        homePlayerStats_[shirtNum].shots++;
                        if (text.find("on target") != std::string::npos || 
                            text.find("saved") != std::string::npos) {
                            homePlayerStats_[shirtNum].shotsOnTarget++;
                        }
                    } else if (awayPlayerStats_.find(shirtNum) != awayPlayerStats_.end()) {
                        awayPlayerStats_[shirtNum].shots++;
                        if (text.find("on target") != std::string::npos || 
                            text.find("saved") != std::string::npos) {
                            awayPlayerStats_[shirtNum].shotsOnTarget++;
                        }
                    }
                }
            }
        }

        // Passes: "passes to" or "pass"
        if (text.find("passes") != std::string::npos || text.find("pass") != std::string::npos) {
            size_t hashPos = text.find('#');
            if (hashPos != std::string::npos) {
                size_t numEnd = hashPos + 1;
                while (numEnd < text.size() && std::isdigit(text[numEnd])) numEnd++;
                if (numEnd > hashPos + 1) {
                    int shirtNum = std::stoi(text.substr(hashPos + 1, numEnd - hashPos - 1));
                    bool completed = text.find("intercepted") == std::string::npos && 
                                   text.find("loses") == std::string::npos;

                    if (homePlayerStats_.find(shirtNum) != homePlayerStats_.end()) {
                        homePlayerStats_[shirtNum].passes++;
                        if (completed) homePlayerStats_[shirtNum].passesCompleted++;
                    } else if (awayPlayerStats_.find(shirtNum) != awayPlayerStats_.end()) {
                        awayPlayerStats_[shirtNum].passes++;
                        if (completed) awayPlayerStats_[shirtNum].passesCompleted++;
                    }
                }
            }
        }

        // Tackles: "tackles" or "tackle"
        if (text.find("tackle") != std::string::npos) {
            size_t hashPos = text.find('#');
            if (hashPos != std::string::npos) {
                size_t numEnd = hashPos + 1;
                while (numEnd < text.size() && std::isdigit(text[numEnd])) numEnd++;
                if (numEnd > hashPos + 1) {
                    int shirtNum = std::stoi(text.substr(hashPos + 1, numEnd - hashPos - 1));
                    if (homePlayerStats_.find(shirtNum) != homePlayerStats_.end()) {
                        homePlayerStats_[shirtNum].tackles++;
                    } else if (awayPlayerStats_.find(shirtNum) != awayPlayerStats_.end()) {
                        awayPlayerStats_[shirtNum].tackles++;
                    }
                }
            }
        }

        // Interceptions: "intercepts"
        if (text.find("intercept") != std::string::npos) {
            size_t hashPos = text.find('#');
            if (hashPos != std::string::npos) {
                size_t numEnd = hashPos + 1;
                while (numEnd < text.size() && std::isdigit(text[numEnd])) numEnd++;
                if (numEnd > hashPos + 1) {
                    int shirtNum = std::stoi(text.substr(hashPos + 1, numEnd - hashPos - 1));
                    if (homePlayerStats_.find(shirtNum) != homePlayerStats_.end()) {
                        homePlayerStats_[shirtNum].interceptions++;
                    } else if (awayPlayerStats_.find(shirtNum) != awayPlayerStats_.end()) {
                        awayPlayerStats_[shirtNum].interceptions++;
                    }
                }
            }
        }

        // Fouls: "fouls" or "foul"
        if (text.find("foul") != std::string::npos && text.find("Foul") == std::string::npos) {
            size_t hashPos = text.find('#');
            if (hashPos != std::string::npos) {
                size_t numEnd = hashPos + 1;
                while (numEnd < text.size() && std::isdigit(text[numEnd])) numEnd++;
                if (numEnd > hashPos + 1) {
                    int shirtNum = std::stoi(text.substr(hashPos + 1, numEnd - hashPos - 1));
                    if (homePlayerStats_.find(shirtNum) != homePlayerStats_.end()) {
                        homePlayerStats_[shirtNum].fouls++;
                    } else if (awayPlayerStats_.find(shirtNum) != awayPlayerStats_.end()) {
                        awayPlayerStats_[shirtNum].fouls++;
                    }
                }
            }
        }

        f.hg = hg;
        f.ag = ag;
    }

    // Find halftime (around minute 45)
    for (size_t i = 0; i < frames_.size(); ++i) {
        if (frames_[i].minute >= 45) {
            halftimeIdx_ = i;
            break;
        }
    }

    screen_ = Screen::Match;
}

void App::renderMatch() {
    // Advance playback timeline.
    if (playing_ && playIdx_ < frames_.size()) {
        playAccum_ += ImGui::GetIO().DeltaTime * speed_;
        while (playAccum_ >= 1.0 && playIdx_ < frames_.size()) {
            playAccum_ -= 1.0;
            ++playIdx_;
            // Auto-pause at halftime (only once when reaching the halftime index)
            if (!halftimePause_ && playIdx_ == halftimeIdx_ && halftimeIdx_ > 0) {
                halftimePause_ = true;
                playing_ = false;
                break;
            }
        }
    }
    if (playIdx_ >= frames_.size()) matchOver_ = true;

    // Space bar toggles play/pause, or starts 2nd half at halftime
    if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
        if (halftimePause_) {
            playing_ = true;
            halftimePause_ = false;
        } else {
            playing_ = !playing_;
        }
    }

    size_t cur = playIdx_ == 0 ? 0 : playIdx_ - 1;
    const Frame* f = frames_.empty() ? nullptr : &frames_[std::min(cur, frames_.size() - 1)];

    // Get next frame for interpolation
    size_t next = std::min(playIdx_, frames_.size() - 1);
    const Frame* nextF = (next > cur && next < frames_.size()) ? &frames_[next] : nullptr;
    float interpT = static_cast<float>(playAccum_);  // 0.0-1.0 between frames

    int hg = f ? f->hg : 0, ag = f ? f->ag : 0;
    int minute = f ? f->minute : 0;

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGuiWindowFlags wf = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                          ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                          ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(255, 255, 255, 255));  // White background for match screen
    ImGui::Begin("##match", nullptr, wf);

    const float fullW = ImGui::GetContentRegionAvail().x;

    // ---- Scoreboard: TEAM  H - A  TEAM, with the clock underneath ----
    ImDrawList* dl = ImGui::GetWindowDrawList();
    char score[64];
    std::snprintf(score, sizeof(score), "%d  -  %d", hg, ag);
    ImGui::SetWindowFontScale(2.0f);
    ImVec2 ssz = ImGui::CalcTextSize(score);
    ImGui::SetWindowFontScale(1.4f);
    ImVec2 hsz = ImGui::CalcTextSize(matchHome_.c_str());
    ImVec2 asz = ImGui::CalcTextSize(matchAway_.c_str());
    ImGui::SetWindowFontScale(1.0f);
    char clk[16];
    std::snprintf(clk, sizeof(clk), "%2d'", minute);
    ImVec2 csz = ImGui::CalcTextSize(clk);

    float cx = ImGui::GetCursorScreenPos().x + fullW * 0.5f;
    float y0 = ImGui::GetCursorScreenPos().y + 4;
    float barHeight = ssz.y + csz.y + 10;
    float x0 = ImGui::GetCursorScreenPos().x;

    // Home team name with background (color1) and text (color2) - using home colors
    ImU32 homeBg = matchHomeTeam_ && !matchHomeTeam_->homeColor1.empty()
        ? parseColor(matchHomeTeam_->homeColor1)
        : IM_COL32(140, 200, 255, 255);
    ImU32 homeText = matchHomeTeam_ && !matchHomeTeam_->homeColor2.empty()
        ? parseColor(matchHomeTeam_->homeColor2)
        : IM_COL32(255, 255, 255, 255);

    // Away team name with background (color1) and text (color2) - using home colors
    ImU32 awayBg = matchAwayTeam_ && !matchAwayTeam_->homeColor1.empty()
        ? parseColor(matchAwayTeam_->homeColor1)
        : IM_COL32(255, 200, 140, 255);
    ImU32 awayText = matchAwayTeam_ && !matchAwayTeam_->homeColor2.empty()
        ? parseColor(matchAwayTeam_->homeColor2)
        : IM_COL32(255, 255, 255, 255);

    // Calculate center section width (for score and timer)
    float centerWidth = ssz.x + 60;  // Score width plus padding
    float centerLeft = cx - centerWidth * 0.5f;
    float centerRight = cx + centerWidth * 0.5f;

    // Draw home team background filling from left edge to center section
    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(centerLeft, y0 + barHeight), homeBg);

    // Draw away team background filling from center section to right edge
    dl->AddRectFilled(ImVec2(centerRight, y0), ImVec2(x0 + fullW, y0 + barHeight), awayBg);

    // Draw dark green center section for score and timer
    ImU32 centerBg = IM_COL32(24, 80, 24, 255);  // Dark green
    ImU32 centerText = IM_COL32(255, 255, 255, 255);  // White
    dl->AddRectFilled(ImVec2(centerLeft, y0), ImVec2(centerRight, y0 + barHeight), centerBg);

    // Draw score and timer in white
    ImGui::SetWindowFontScale(2.0f);
    dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                ImVec2(cx - ssz.x * 0.5f, y0), centerText, score);
    ImGui::SetWindowFontScale(1.0f);
    dl->AddText(ImVec2(cx - csz.x * 0.5f, y0 + ssz.y + 2), centerText, clk);

    // Draw team names and goalscorers in team colors within the bar
    ImGui::SetWindowFontScale(1.4f);
    ImVec2 homeNamePos(x0 + 15, y0 + 6);
    dl->AddText(homeNamePos, homeText, matchHome_.c_str());

    ImVec2 awayNamePos(x0 + fullW - asz.x - 15, y0 + 6);
    dl->AddText(awayNamePos, awayText, matchAway_.c_str());

    // Draw goalscorers within team sections
    ImGui::SetWindowFontScale(0.9f);
    float scorerY = y0 + 6 + hsz.y + 4;  // Below team name
    float scorerX = x0 + 15;

    // Home team scorers (left side)
    for (const auto& sc : homeScorers_) {
        if (sc.first <= minute) {
            ImVec2 scorerSize = ImGui::CalcTextSize(sc.second.c_str());
            if (scorerY + scorerSize.y < y0 + barHeight - 2) {  // Only draw if it fits
                dl->AddText(ImVec2(scorerX, scorerY), homeText, sc.second.c_str());
                scorerY += scorerSize.y + 2;
            }
        }
    }

    // Away team scorers (right side, right-aligned)
    scorerY = y0 + 6 + asz.y + 4;
    for (const auto& sc : awayScorers_) {
        if (sc.first <= minute) {
            ImVec2 scorerSize = ImGui::CalcTextSize(sc.second.c_str());
            if (scorerY + scorerSize.y < y0 + barHeight - 2) {  // Only draw if it fits
                dl->AddText(ImVec2(x0 + fullW - scorerSize.x - 15, scorerY), awayText, sc.second.c_str());
                scorerY += scorerSize.y + 2;
            }
        }
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::Dummy(ImVec2(fullW, barHeight));
    ImGui::Separator();

    // ---- New Layout: [Team 1 - Full Height] | [Pitch + Commentary] | [Team 2 - Full Height] ----
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float bottomH = 54.0f;
    const float feedH = 180.0f;  // Commentary height
    const float tacticsH = 38.0f;

    float totalAvailH = ImGui::GetContentRegionAvail().y - bottomH - 10;

    // First calculate pitch size to determine how much space we have
    float pitchAvailH = totalAvailH - feedH - spacing;
    const float pitchRatio = 105.0f / 68.0f;

    // Start with maximum available width for pitch (leave some space for squads)
    float minSquadW = 180.0f;  // Minimum squad panel width
    float pitchMaxW = fullW - 2 * (minSquadW + spacing);
    float pitchMaxH = pitchAvailH;
    float pitchW, pitchH;

    // Calculate size maintaining aspect ratio
    if (pitchMaxW / pitchMaxH > pitchRatio) {
        // Limited by height
        pitchH = pitchMaxH;
        pitchW = pitchH * pitchRatio;
    } else {
        // Limited by width
        pitchW = pitchMaxW;
        pitchH = pitchW / pitchRatio;
    }

    // Calculate squad panel width to fill from edge to pitch
    float sideW = (fullW - pitchW - 2 * spacing) * 0.5f;

    // ---- Left: Team 1 lineup (full height) ----
    ImGui::BeginGroup();
    squadPanel("home_sq", matchHome_.c_str(), matchHomeTeam_, 
               ImVec2(sideW, totalAvailH - tacticsH - spacing),
               &homePlayerStats_,
               [this](const Player* p) { openPlayerDetail(p, Screen::Match); });
    bool homeTac = !matchOver_ && matchHomeTeam_;
    if (!homeTac) ImGui::BeginDisabled();
    if (tintButton("Go to Tactics##h", IM_COL32(150, 120, 60, 255), ImVec2(sideW, tacticsH))) {
        playing_ = false;
        openTactics(matchHomeTeam_, Screen::Match);
    }
    if (!homeTac) ImGui::EndDisabled();
    ImGui::EndGroup();
    ImGui::SameLine();

    // ---- Center: Pitch + Commentary and Stats below it ----
    ImGui::BeginGroup();

    ImVec2 pitchPos = ImGui::GetCursorScreenPos();
    drawPitch(pitchPos, ImVec2(pitchW, pitchH), f, nextF, interpT);
    ImGui::Dummy(ImVec2(pitchW, pitchH));

    // Split the space below pitch: Match Events (left half) and Statistics (right half)
    float halfPitchW = (pitchW - spacing) * 0.5f;

    // Match events feed (commentary) - left half
    ImGui::BeginChild("feed", ImVec2(halfPitchW, feedH), true);
    panelHeader("Match Events");
    size_t shown = playIdx_;
    size_t startI = shown > 40 ? shown - 40 : 0;
    for (size_t i = startI; i < shown && i < frames_.size(); ++i) {
        const Frame& ev = frames_[i];
        if (ev.key)
            ImGui::TextColored(ImVec4(1, 0.9f, 0.3f, 1), "%2d'  %s", ev.minute, ev.text.c_str());
        else
            ImGui::TextWrapped("%2d'  %s", ev.minute, ev.text.c_str());
    }
    if (playing_) ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    ImGui::SameLine();

    // Match statistics - right half
    ImGui::BeginChild("stats", ImVec2(halfPitchW, feedH), true);
    panelHeader("Match Statistics");

    if (f) {
        const MatchStats& stats = f->stats;

        // Possession
        long totalPoss = stats.possTicks[0] + stats.possTicks[1];
        int homePoss = totalPoss > 0 ? (int)((stats.possTicks[0] * 100) / totalPoss) : 50;
        int awayPoss = 100 - homePoss;

        ImGui::Spacing();
        ImGui::Text("Possession");
        ImGui::Columns(3, "poss", false);
        ImGui::SetColumnWidth(0, 50);
        ImGui::SetColumnWidth(1, halfPitchW - 120);
        ImGui::SetColumnWidth(2, 50);
        ImGui::Text("%d%%", homePoss);
        ImGui::NextColumn();
        // Draw possession bar
        float barW = ImGui::GetColumnWidth() - 10;
        ImVec2 barPos = ImGui::GetCursorScreenPos();
        float homeBarW = (barW * homePoss) / 100.0f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(barPos, ImVec2(barPos.x + homeBarW, barPos.y + 16), 
                         IM_COL32(120, 180, 255, 255));
        dl->AddRectFilled(ImVec2(barPos.x + homeBarW, barPos.y), 
                         ImVec2(barPos.x + barW, barPos.y + 16), 
                         IM_COL32(255, 180, 120, 255));
        ImGui::Dummy(ImVec2(barW, 16));
        ImGui::NextColumn();
        ImGui::Text("%d%%", awayPoss);
        ImGui::Columns(1);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Stats table
        ImGui::Columns(3, "stats_table", false);
        ImGui::SetColumnWidth(0, 60);
        ImGui::SetColumnWidth(1, halfPitchW - 140);
        ImGui::SetColumnWidth(2, 60);

        // Header
        ImGui::Text("Home");
        ImGui::NextColumn();
        ImGui::NextColumn();
        ImGui::Text("Away");
        ImGui::NextColumn();
        ImGui::Separator();

        // Shots
        ImGui::Text("%d", stats.shots[0]);
        ImGui::NextColumn();
        ImGui::Text("Shots");
        ImGui::NextColumn();
        ImGui::Text("%d", stats.shots[1]);
        ImGui::NextColumn();

        // On Target
        ImGui::Text("%d", stats.onTarget[0]);
        ImGui::NextColumn();
        ImGui::Text("On Target");
        ImGui::NextColumn();
        ImGui::Text("%d", stats.onTarget[1]);
        ImGui::NextColumn();

        // Passing accuracy
        int homePassPct = stats.passAtt[0] > 0 ? (stats.passOk[0] * 100) / stats.passAtt[0] : 0;
        int awayPassPct = stats.passAtt[1] > 0 ? (stats.passOk[1] * 100) / stats.passAtt[1] : 0;
        ImGui::Text("%d%%", homePassPct);
        ImGui::NextColumn();
        ImGui::Text("Pass Accuracy");
        ImGui::NextColumn();
        ImGui::Text("%d%%", awayPassPct);
        ImGui::NextColumn();

        // Corners
        ImGui::Text("%d", stats.corners[0]);
        ImGui::NextColumn();
        ImGui::Text("Corners");
        ImGui::NextColumn();
        ImGui::Text("%d", stats.corners[1]);
        ImGui::NextColumn();

        // Fouls
        ImGui::Text("%d", stats.fouls[0]);
        ImGui::NextColumn();
        ImGui::Text("Fouls");
        ImGui::NextColumn();
        ImGui::Text("%d", stats.fouls[1]);
        ImGui::NextColumn();

        ImGui::Columns(1);
    } else {
        ImGui::TextDisabled("No statistics available yet");
    }

    ImGui::EndChild();

    ImGui::EndGroup();
    ImGui::SameLine();

    // ---- Right: Team 2 lineup (full height) ----
    ImGui::BeginGroup();
    squadPanel("away_sq", matchAway_.c_str(), matchAwayTeam_, 
               ImVec2(sideW, totalAvailH - tacticsH - spacing),
               &awayPlayerStats_,
               [this](const Player* p) { openPlayerDetail(p, Screen::Match); });
    bool awayTac = !matchOver_ && matchAwayTeam_;
    if (!awayTac) ImGui::BeginDisabled();
    if (tintButton("Go to Tactics##a", IM_COL32(150, 120, 60, 255), ImVec2(sideW, tacticsH))) {
        playing_ = false;
        openTactics(matchAwayTeam_, Screen::Match);
    }
    if (!awayTac) ImGui::EndDisabled();
    ImGui::EndGroup();

    // ---- Bottom control bar ----
    if (matchOver_) {
        if (tintButton("Back to Menu", IM_COL32(40, 92, 178, 255), ImVec2(220, bottomH - 10)))
            screen_ = Screen::Main;
        ImGui::SameLine();
        if (tintButton("Watch Again", IM_COL32(86, 150, 38, 255), ImVec2(180, bottomH - 10))) {
            playIdx_ = 0;
            playing_ = true;
            matchOver_ = false;
            halftimePause_ = false;
        }
    } else if (halftimePause_) {
        // Halftime - show "Start 2nd Half" button
        ImGui::SetWindowFontScale(1.2f);
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "HALF-TIME");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::SameLine(0, 20);
        if (tintButton("Start 2nd Half  (space)", IM_COL32(86, 150, 38, 255),
                       ImVec2(fullW - 360, bottomH - 10))) {
            playing_ = true;
            halftimePause_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Skip to End", ImVec2(140, bottomH - 10))) {
            playIdx_ = frames_.size();
            matchOver_ = true;
            halftimePause_ = false;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        ImGui::SliderFloat("Speed", &speed_, 2.0f, 40.0f, "%.0f ev/s");
    } else {
        char bar[64];
        std::snprintf(bar, sizeof(bar), "%s  (space)", playing_ ? "Pause Match" : "Continue");
        if (tintButton(bar, IM_COL32(60, 120, 170, 255),
                       ImVec2(fullW - 360, bottomH - 10)))
            playing_ = !playing_;
        ImGui::SameLine();
        if (ImGui::Button("Skip to End", ImVec2(140, bottomH - 10))) {
            playIdx_ = frames_.size();
            matchOver_ = true;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        ImGui::SliderFloat("Speed", &speed_, 2.0f, 40.0f, "%.0f ev/s");
    }

    ImGui::PopStyleColor();  // Pop the white background color
    ImGui::End();
}

// Draws the live pitch as a graphical, mowed-grass field split into the three
// thirds from the mock-up (defence / midfield / attack), with the active third
// tinted in the possession side's colour and the ball drawn at its grid cell.
void App::drawPitch(ImVec2 p0, ImVec2 size, const Frame* f, const Frame* nextF, float t) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p1(p0.x + size.x, p0.y + size.y);

    // Mowed-grass stripes.
    const int stripes = 9;
    for (int i = 0; i < stripes; ++i) {
        float x0 = p0.x + size.x * i / stripes;
        float x1 = p0.x + size.x * (i + 1) / stripes;
        ImU32 col = (i % 2) ? IM_COL32(74, 142, 64, 255) : IM_COL32(62, 126, 54, 255);
        dl->AddRectFilled(ImVec2(x0, p0.y), ImVec2(x1, p1.y), col);
    }

    const ImU32 line = IM_COL32(232, 240, 230, 210);
    const float th = 2.0f, inset = 10.0f;
    ImVec2 a(p0.x + inset, p0.y + inset), b(p1.x - inset, p1.y - inset);
    float midx = (a.x + b.x) * 0.5f, midy = (a.y + b.y) * 0.5f;

    // Active-third highlight (drawn under the markings).
    if (f) {
        int third = (f->ballCol <= 4) ? 0 : (f->ballCol <= 9 ? 1 : 2);
        float tx0 = a.x + (b.x - a.x) * third / 3.0f;
        float tx1 = a.x + (b.x - a.x) * (third + 1) / 3.0f;
        ImU32 hl = (f->carrier == 0) ? IM_COL32(80, 150, 235, 55) : IM_COL32(235, 150, 70, 55);
        dl->AddRectFilled(ImVec2(tx0, a.y), ImVec2(tx1, b.y), hl);
    }

    // Boundary, third dividers, halfway line and centre circle.
    dl->AddRect(a, b, line, 0, 0, th);
    for (int i = 1; i <= 2; ++i) {
        float x = a.x + (b.x - a.x) * i / 3.0f;
        dl->AddLine(ImVec2(x, a.y), ImVec2(x, b.y), IM_COL32(232, 240, 230, 70), 1.5f);
    }
    dl->AddLine(ImVec2(midx, a.y), ImVec2(midx, b.y), line, th);
    float cr = (b.y - a.y) * 0.16f;
    dl->AddCircle(ImVec2(midx, midy), cr, line, 40, th);
    dl->AddCircleFilled(ImVec2(midx, midy), 3.0f, line);

    // Penalty boxes and goals at each end.
    float boxH = (b.y - a.y) * 0.5f, boxW = (b.x - a.x) * 0.12f;
    dl->AddRect(ImVec2(a.x, midy - boxH / 2), ImVec2(a.x + boxW, midy + boxH / 2), line, 0, 0, th);
    dl->AddRect(ImVec2(b.x - boxW, midy - boxH / 2), ImVec2(b.x, midy + boxH / 2), line, 0, 0, th);
    float goalH = (b.y - a.y) * 0.22f;
    dl->AddRectFilled(ImVec2(a.x - 5, midy - goalH / 2), ImVec2(a.x, midy + goalH / 2),
                      IM_COL32(240, 240, 240, 220));
    dl->AddRectFilled(ImVec2(b.x, midy - goalH / 2), ImVec2(b.x + 5, midy + goalH / 2),
                      IM_COL32(240, 240, 240, 220));

    if (!f) return;

    // Draw players at their continuous positions with smooth interpolation.
    for (size_t i = 0; i < f->players.size(); ++i) {
        const auto& p = f->players[i];

        // Use continuous positions for smoother rendering
        float x = p.x;  // Already in meters (0-105)
        float y = p.y;  // Already in meters (0-68)
        bool isCarrier = p.isCarrier;

        if (nextF && i < nextF->players.size()) {
            // Try to find the same player in next frame
            const Frame::PlayerPos* nextP = nullptr;
            for (const auto& np : nextF->players) {
                if (np.shirtNumber == p.shirtNumber && np.side == p.side) {
                    nextP = &np;
                    break;
                }
            }

            if (nextP) {
                // Interpolate continuous position
                x = p.x + (nextP->x - p.x) * t;
                y = p.y + (nextP->y - p.y) * t;
                // Use next frame's carrier status if we're past halfway
                if (t > 0.5f) isCarrier = nextP->isCarrier;
            }
        }

        // Map continuous meters to screen pixels
        float px = a.x + (b.x - a.x) * (x / 105.0f);
        float py = a.y + (b.y - a.y) * (y / 68.0f);

        // Get team colors based on side (home/away)
        ImU32 playerCol, trimCol;
        if (p.side == 0) {
            // Home team uses home colors
            playerCol = matchHomeTeam_ && !matchHomeTeam_->homeColor1.empty() 
                ? parseColor(matchHomeTeam_->homeColor1) 
                : IM_COL32(220, 50, 50, 255);  // Default red
            trimCol = matchHomeTeam_ && !matchHomeTeam_->homeColor2.empty()
                ? parseColor(matchHomeTeam_->homeColor2)
                : IM_COL32(255, 255, 255, 255);  // Default white
        } else {
            // Away team uses away colors
            playerCol = matchAwayTeam_ && !matchAwayTeam_->awayColor1.empty()
                ? parseColor(matchAwayTeam_->awayColor1)
                : IM_COL32(50, 100, 220, 255);  // Default blue
            trimCol = matchAwayTeam_ && !matchAwayTeam_->awayColor2.empty()
                ? parseColor(matchAwayTeam_->awayColor2)
                : IM_COL32(255, 255, 255, 255);  // Default white
        }

        // Highlight ball carrier with a bright ring
        if (isCarrier) {
            dl->AddCircleFilled(ImVec2(px, py), 12.0f, IM_COL32(255, 255, 100, 255));
        }

        // Draw player circle with team colors
        dl->AddCircleFilled(ImVec2(px, py), 10.0f, playerCol);
        dl->AddCircle(ImVec2(px, py), 10.0f, trimCol, 16, 2.0f);

        // Draw shirt number in trim color
        char numStr[4];
        std::snprintf(numStr, sizeof(numStr), "%d", p.shirtNumber);
        ImVec2 textSize = ImGui::CalcTextSize(numStr);
        dl->AddText(ImVec2(px - textSize.x * 0.5f, py - textSize.y * 0.5f),
                    trimCol, numStr);
    }

    // Ball at its continuous position with smooth interpolation.
    float ballX = f->ballX;  // Already in meters (0-105)
    float ballY = f->ballY;  // Already in meters (0-68)

    if (nextF) {
        // Interpolate continuous ball position
        ballX = f->ballX + (nextF->ballX - f->ballX) * t;
        ballY = f->ballY + (nextF->ballY - f->ballY) * t;
    }

    // Map continuous meters to screen pixels
    float bx = a.x + (b.x - a.x) * (ballX / 105.0f);
    float by = a.y + (b.y - a.y) * (ballY / 68.0f);
    ImU32 ring = (f->carrier == 0) ? IM_COL32(120, 180, 255, 255) : IM_COL32(255, 180, 110, 255);
    dl->AddCircleFilled(ImVec2(bx, by), 9.0f, ring);
    dl->AddCircleFilled(ImVec2(bx, by), 6.0f, IM_COL32(255, 255, 255, 255));
    dl->AddCircle(ImVec2(bx, by), 6.0f, IM_COL32(20, 20, 20, 255), 16, 1.5f);
}

void App::renderDatabase() {
    // Draw cycling background
    drawCyclingBackground();

    beginScreen("Database");
    if (ImGui::Button("< Back")) screen_ = Screen::Main;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(360);
    ImGui::InputTextWithHint("##search", "search players by name", dbSearch_,
                             sizeof(dbSearch_));
    ImGui::Spacing();

    std::vector<std::pair<const Team*, const Player*>> out;
    db_.searchPlayers(dbSearch_, out);
    int total = static_cast<int>(out.size());
    if (total > 300) out.resize(300);
    ImGui::TextDisabled("%d players matched%s", total,
                        total > 300 ? " (showing first 300)" : "");

    ImGuiTableFlags tf = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                         ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable;
    if (ImGui::BeginTable("players", 8, tf, ImVec2(0, 460))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Club");
        ImGui::TableSetupColumn("Pos");
        ImGui::TableSetupColumn("OVR");
        ImGui::TableSetupColumn("Pac");
        ImGui::TableSetupColumn("Sho");
        ImGui::TableSetupColumn("Pas");
        ImGui::TableSetupColumn("Tck");
        ImGui::TableHeadersRow();
        for (auto& pr : out) {
            const Team* t = pr.first;
            const Player* p = pr.second;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            // Make player name clickable
            if (ImGui::Selectable(p->name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                openPlayerDetail(p, Screen::Database);
            }
            ImGui::TableSetColumnIndex(1);
            // Make team name clickable
            if (t) {
                if (ImGui::Selectable(t->name.c_str(), false)) {
                    openTeamOverview(const_cast<Team*>(t), Screen::Database);
                }
            } else {
                ImGui::TextUnformatted("-");
            }
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(PosName(p->primaryPos).c_str());
            positionTooltip(*p);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.0f", playerOverall(*p));
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%d", p->attr.get("Pace"));
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%d", p->attr.get("Shooting"));
            ImGui::TableSetColumnIndex(6);
            ImGui::Text("%d", p->attr.get("Passing"));
            ImGui::TableSetColumnIndex(7);
            ImGui::Text("%d", p->attr.get("Tackling"));
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

// --------------------------------------------------------------------------
// Career
// --------------------------------------------------------------------------
void App::careerStart(int teamId) {
    Team* t = teamById(teamId);
    if (!t) return;
    careerTeam_ = teamId;
    careerLeagueName_ = t->league;
    careerActive_ = true;
    careerRound_ = 0;
    table_.clear();
    careerLog_.clear();
    fixtures_.clear();
    roundStart_.clear();

    std::vector<Team*> teams = db_.teamsInLeague(careerLeagueName_);
    std::vector<int> ids;
    for (Team* tm : teams)
        if (tm->squad.size() >= 7) ids.push_back(tm->id);
    for (int id : ids) {
        Standing s;
        s.teamId = id;
        s.name = teamById(id)->name;
        table_[id] = s;
    }
    if (ids.size() < 2) {
        careerActive_ = false;
        status_ = "Not enough playable teams in " + careerLeagueName_;
        return;
    }
    // Round-robin (circle method). Add a bye if odd.
    bool bye = ids.size() % 2 != 0;
    if (bye) ids.push_back(-1);
    int n = static_cast<int>(ids.size());
    int rounds = n - 1;
    std::vector<int> arr = ids;
    for (int r = 0; r < rounds; ++r) {
        roundStart_.push_back(fixtures_.size());
        for (int i = 0; i < n / 2; ++i) {
            int a = arr[i];
            int b = arr[n - 1 - i];
            if (a != -1 && b != -1) {
                if (r % 2 == 0)
                    fixtures_.emplace_back(a, b);
                else
                    fixtures_.emplace_back(b, a);
            }
        }
        // rotate (keep first fixed)
        int last = arr[n - 1];
        for (int i = n - 1; i > 1; --i) arr[i] = arr[i - 1];
        arr[1] = last;
    }
    roundStart_.push_back(fixtures_.size());
    screen_ = Screen::Career;
}

void App::careerAdvance() {
    if (!careerActive_) return;
    if (careerRound_ + 1 >= static_cast<int>(roundStart_.size())) return;
    size_t from = roundStart_[careerRound_];
    size_t to = roundStart_[careerRound_ + 1];
    for (size_t i = from; i < to; ++i) {
        Team* h = teamById(fixtures_[i].first);
        Team* a = teamById(fixtures_[i].second);
        if (!h || !a) continue;
        MatchEngine engine(cfg_, 5000u + static_cast<unsigned>(i) + careerRound_ * 131u);
        MatchResult r = engine.simulate(*h, *a);
        Standing& sh = table_[h->id];
        Standing& sa = table_[a->id];
        sh.p++; sa.p++;
        sh.gf += r.homeGoals; sh.ga += r.awayGoals;
        sa.gf += r.awayGoals; sa.ga += r.homeGoals;
        if (r.homeGoals > r.awayGoals) { sh.w++; sh.pts += 3; sa.l++; }
        else if (r.homeGoals < r.awayGoals) { sa.w++; sa.pts += 3; sh.l++; }
        else { sh.d++; sa.d++; sh.pts++; sa.pts++; }
        if (h->id == careerTeam_ || a->id == careerTeam_) {
            char line[160];
            std::snprintf(line, sizeof(line), "R%d: %s %d-%d %s", careerRound_ + 1,
                          h->name.c_str(), r.homeGoals, r.awayGoals, a->name.c_str());
            careerLog_.push_back(line);
        }
    }
    careerRound_++;
}

void App::careerSave() {
    std::ofstream o(dataDir_ + "/career.sav");
    if (!o.is_open()) { status_ = "Could not write save."; return; }
    o << careerTeam_ << "\n" << careerLeagueName_ << "\n" << careerRound_ << "\n";
    for (auto& kv : table_) {
        const Standing& s = kv.second;
        o << s.teamId << " " << s.p << " " << s.w << " " << s.d << " " << s.l
          << " " << s.gf << " " << s.ga << " " << s.pts << "\n";
    }
    status_ = "Career saved.";
}

void App::careerLoad() {
    std::ifstream in(dataDir_ + "/career.sav");
    if (!in.is_open()) { status_ = "No save found."; return; }
    int teamId, round;
    in >> teamId;
    in.ignore();
    std::string league;
    std::getline(in, league);
    in >> round;
    careerStart(teamId);
    if (!careerActive_) return;
    careerRound_ = round;
    int id;
    while (in >> id) {
        Standing& s = table_[id];
        s.teamId = id;
        in >> s.p >> s.w >> s.d >> s.l >> s.gf >> s.ga >> s.pts;
        Team* t = teamById(id);
        if (t) s.name = t->name;
    }
    status_ = "Career loaded.";
    screen_ = Screen::Career;
}

void App::renderCareer() {
    // Draw cycling background
    drawCyclingBackground();

    beginScreen("Career");
    if (ImGui::Button("< Back")) screen_ = Screen::Main;
    ImGui::SameLine();
    if (ImGui::Button("Load Save")) careerLoad();

    if (!careerActive_) {
        ImGui::Spacing();
        ImGui::TextWrapped("Pick a club to manage. You'll play a full round-robin "
                           "season in its league.");
        teamPicker("career", careerLeague_, careerTeam_, careerFilter_,
                   sizeof(careerFilter_));
        Team* t = teamById(careerTeam_);
        bool ok = t && t->squad.size() >= 7;
        if (!ok) ImGui::BeginDisabled();
        if (ImGui::Button("Start Career", ImVec2(200, 40))) careerStart(careerTeam_);
        if (!ok) ImGui::EndDisabled();
        ImGui::End();
        return;
    }

    Team* myteam = teamById(careerTeam_);
    int totalRounds = static_cast<int>(roundStart_.size()) - 1;
    ImGui::Text("Managing: %s   (%s)", myteam ? myteam->name.c_str() : "?",
                careerLeagueName_.c_str());
    ImGui::Text("Round %d / %d", careerRound_, totalRounds);
    ImGui::SameLine();
    bool done = careerRound_ >= totalRounds;
    if (done) ImGui::BeginDisabled();
    if (ImGui::Button("Advance Round")) careerAdvance();
    if (done) ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Tactics") && myteam) openTactics(myteam, Screen::Career);
    ImGui::SameLine();
    if (ImGui::Button("Save")) careerSave();
    if (done) ImGui::SameLine(), ImGui::TextColored(ImVec4(1, 0.9f, 0.3f, 1), "Season complete!");

    ImGui::Spacing();
    // Standings sorted by points then goal difference.
    std::vector<Standing> rows;
    rows.reserve(table_.size());
    for (auto& kv : table_) rows.push_back(kv.second);
    std::sort(rows.begin(), rows.end(), [](const Standing& a, const Standing& b) {
        if (a.pts != b.pts) return a.pts > b.pts;
        int ga = a.gf - a.ga, gb = b.gf - b.ga;
        if (ga != gb) return ga > gb;
        return a.gf > b.gf;
    });

    ImGui::Columns(2, "careercols", false);
    ImGui::SetColumnWidth(0, 560);
    ImGuiTableFlags tf = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                         ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("table", 9, tf, ImVec2(0, 460))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30);
        ImGui::TableSetupColumn("Club", ImGuiTableColumnFlags_WidthFixed, 220);
        const char* nums[] = {"P", "W", "D", "L", "GF", "GA", "Pts"};
        for (const char* c : nums)
            ImGui::TableSetupColumn(c, ImGuiTableColumnFlags_WidthFixed, 34);
        ImGui::TableHeadersRow();
        int pos = 1;
        for (const Standing& s : rows) {
            ImGui::TableNextRow();
            bool mine = s.teamId == careerTeam_;
            if (mine)
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                                       IM_COL32(40, 80, 40, 255));
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", pos++);
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(s.name.c_str());
            int vals[] = {s.p, s.w, s.d, s.l, s.gf, s.ga, s.pts};
            for (int i = 0; i < 7; ++i) {
                ImGui::TableSetColumnIndex(2 + i);
                ImGui::Text("%d", vals[i]);
            }
        }
        ImGui::EndTable();
    }
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1, 1), "Your results");
    ImGui::BeginChild("clog", ImVec2(0, 460), true);
    for (auto it = careerLog_.rbegin(); it != careerLog_.rend(); ++it)
        ImGui::TextWrapped("%s", it->c_str());
    ImGui::EndChild();
    ImGui::Columns(1);

    ImGui::End();
}

void App::renderData() {
    // Draw cycling background
    drawCyclingBackground();

    beginScreen("Data Sources");
    if (ImGui::Button("< Back")) screen_ = Screen::Main;
    ImGui::Spacing();
    ImGui::TextWrapped("Load your own Championship Manager / FM style exports. "
                       "Enter full paths to the players and clubs CSV files, then "
                       "press Load. Leave clubs blank to keep the current clubs.");
    ImGui::Spacing();
    ImGui::SetNextItemWidth(700);
    ImGui::InputTextWithHint("Players CSV", "e.g. D:\\DEV\\Docs\\PlayersDB.csv",
                             playersPath_, sizeof(playersPath_));
    ImGui::SetNextItemWidth(700);
    ImGui::InputTextWithHint("Clubs CSV", "e.g. D:\\DEV\\Docs\\ClubsDB.csv",
                             clubsPath_, sizeof(clubsPath_));
    ImGui::Spacing();
    if (ImGui::Button("Load", ImVec2(160, 36))) {
        Database fresh;
        bool ok = true;
        if (clubsPath_[0]) ok = fresh.loadTeams(clubsPath_);
        if (ok && playersPath_[0]) ok = fresh.loadPlayers(playersPath_);
        if (ok && !fresh.teams.empty()) {
            db_.teams = std::move(fresh.teams);
            leagues_ = db_.leagues();
            homeTeam_ = awayTeam_ = careerTeam_ = -1;
            careerActive_ = false;
            status_ = "Loaded " + std::to_string(db_.teams.size()) + " teams from custom files.";
        } else {
            status_ = "Could not load the given files.";
        }
    }
    ImGui::Spacing();
    ImGui::TextDisabled("%s", status_.c_str());
    ImGui::End();
}

void App::renderAbout() {
    // Draw cycling background
    drawCyclingBackground();

    beginScreen("About");
    if (ImGui::Button("< Back")) screen_ = Screen::Main;
    ImGui::Spacing();
    ImGui::TextWrapped(
        "Nostalgia Manager Simulation\n\n"
        "A retro football management game built on the GBNFM match engine: a "
        "two-phase (Desire then Execution) action model on a 9x13 pitch matrix, "
        "with zone-restricted actions, defensive pressure and configurable "
        "thresholds (data/engine.cfg).\n\n"
        "This window is a Dear ImGui front-end over the same engine, database "
        "loader (Championship Manager / FM CSV exports) and career logic.");
    ImGui::End();
}

void App::openPlayerDetail(const Player* player, Screen returnTo) {
    detailPlayer_ = player;
    detailReturn_ = returnTo;
    screen_ = Screen::PlayerDetail;
}

void App::openTeamOverview(Team* team, Screen returnTo) {
    TeamOverviewScreen::openTeamOverview(this, team, returnTo);
}

void App::renderTeamOverview() {
    TeamOverviewScreen::render(this);
}

void App::renderPlayerDetail() {
    drawCyclingBackground();

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGuiWindowFlags wf = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("##playerdetail", nullptr, wf);

    if (!detailPlayer_) {
        ImGui::Text("No player selected");
        if (ImGui::Button("< Back")) screen_ = detailReturn_;
        ImGui::End();
        return;
    }

    const Player& p = *detailPlayer_;

    // Header with back button and player name
    if (ImGui::Button("< Back", ImVec2(120, 40))) {
        screen_ = detailReturn_;
    }
    ImGui::SameLine();
    ImVec4 gold = ImVec4(0.95f, 0.92f, 0.82f, 1);
    ImGui::PushStyleColor(ImGuiCol_Text, gold);
    ImGui::SetWindowFontScale(2.0f);
    ImGui::Text("%s", p.name.c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Calculate layout dimensions
    float fullW = ImGui::GetContentRegionAvail().x;
    float fullH = ImGui::GetContentRegionAvail().y;
    float leftW = fullW * 0.6f;
    float rightW = fullW * 0.4f - 10;

    // Left column - Player info and attributes
    ImGui::BeginChild("player_info", ImVec2(leftW, fullH), true);
    ImGui::PushStyleColor(ImGuiCol_Text, gold);
    ImGui::Text("Player Information");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Basic info
    ImGui::Text("Primary Position: %s", PosName(p.primaryPos).c_str());
    ImGui::Text("Shirt Number: %d", p.shirtNumber);
    ImGui::Text("Overall: %.0f", playerOverall(p));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Positions by Proficiency
    ImGui::PushStyleColor(ImGuiCol_Text, gold);
    ImGui::Text("Positions by Proficiency");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Categorize positions
    std::vector<Position> preferred, natural, accomplished, competent, unconvincing;

    for (Position pos : p.playablePositions) {
        int rating = p.getPositionRating(pos);

        if (pos == p.primaryPos) {
            preferred.push_back(pos);
        } else if (rating == 100) {
            natural.push_back(pos);
        } else if (rating >= 70) {
            accomplished.push_back(pos);
        } else if (rating >= 40) {
            competent.push_back(pos);
        } else {
            unconvincing.push_back(pos);
        }
    }

    auto displayCategory = [](const char* label, const std::vector<Position>& positions, ImVec4 color) {
        if (!positions.empty()) {
            ImGui::TextColored(color, "%s", label);
            std::string posStr;
            for (size_t i = 0; i < positions.size(); ++i) {
                if (i > 0) posStr += ", ";
                posStr += PosName(positions[i]);
            }
            ImGui::TextWrapped("  %s", posStr.c_str());
            ImGui::Spacing();
        }
    };

    displayCategory("Preferred", preferred, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));  // Green
    displayCategory("Natural", natural, ImVec4(0.5f, 0.9f, 0.5f, 1.0f));      // Light green
    displayCategory("Accomplished", accomplished, ImVec4(0.8f, 0.9f, 0.4f, 1.0f));  // Yellow-green
    displayCategory("Competent", competent, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));  // Orange
    displayCategory("Unconvincing", unconvincing, ImVec4(1.0f, 0.5f, 0.3f, 1.0f));  // Red-orange

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Attributes
    ImGui::PushStyleColor(ImGuiCol_Text, gold);
    ImGui::Text("Attributes");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::Columns(2, "attrs", true);
    ImGui::Text("Pace: %d", p.attr.get("Pace"));
    ImGui::NextColumn();
    ImGui::Text("Shooting: %d", p.attr.get("Shooting"));
    ImGui::NextColumn();
    ImGui::Text("Passing: %d", p.attr.get("Passing"));
    ImGui::NextColumn();
    ImGui::Text("Tackling: %d", p.attr.get("Tackling"));
    ImGui::NextColumn();
    ImGui::Text("Heading: %d", p.attr.get("Heading"));
    ImGui::NextColumn();
    ImGui::Text("Stamina: %d", p.attr.get("Stamina"));
    ImGui::NextColumn();
    ImGui::Text("Technique: %d", p.attr.get("Technique"));
    ImGui::NextColumn();
    ImGui::Text("Strength: %d", p.attr.get("Strength"));
    ImGui::NextColumn();
    ImGui::Columns(1);

    ImGui::EndChild();
    ImGui::SameLine();

    // Right column - Smaller pitch visualization
    ImGui::BeginChild("player_pitch", ImVec2(rightW, fullH), true);

    ImGui::PushStyleColor(ImGuiCol_Text, gold);
    ImGui::Text("Position Map");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    // Make pitch smaller and centered
    float pitchRatio = 105.0f / 68.0f;
    float pitchW, pitchH;
    float maxW = canvasSize.x - 40;
    float maxH = canvasSize.y * 0.6f;  // Use less vertical space

    if (maxW / maxH > pitchRatio) {
        pitchH = maxH;
        pitchW = pitchH * pitchRatio;
    } else {
        pitchW = maxW;
        pitchH = pitchW / pitchRatio;
    }

    float offsetX = (canvasSize.x - pitchW) * 0.5f;
    float offsetY = 20;

    ImVec2 pitchMin(canvasPos.x + offsetX, canvasPos.y + offsetY);
    ImVec2 pitchMax(pitchMin.x + pitchW, pitchMin.y + pitchH);

    // Draw pitch background
    dl->AddRectFilled(pitchMin, pitchMax, IM_COL32(74, 132, 62, 255), 4.0f);

    // Draw stripes
    int stripes = 10;
    for (int s = 0; s < stripes; ++s) {
        if (s % 2 == 0) continue;
        float y0 = pitchMin.y + (pitchH * s) / stripes;
        float y1 = pitchMin.y + (pitchH * (s + 1)) / stripes;
        dl->AddRectFilled(ImVec2(pitchMin.x, y0), ImVec2(pitchMax.x, y1), IM_COL32(82, 142, 68, 255));
    }

    // Draw lines
    ImU32 lineCol = IM_COL32(255, 255, 255, 180);
    dl->AddRect(pitchMin, pitchMax, lineCol, 0, 0, 2.0f);
    dl->AddLine(ImVec2(pitchMin.x, pitchMin.y + pitchH * 0.5f),
                ImVec2(pitchMax.x, pitchMin.y + pitchH * 0.5f), lineCol, 2.0f);
    dl->AddCircle(ImVec2(pitchMin.x + pitchW * 0.5f, pitchMin.y + pitchH * 0.5f),
                  pitchW * 0.1f, lineCol, 32, 2.0f);

    // Helper to get position coordinates (normalized 0-1)
    auto getPosCoords = [](Position pos) -> std::pair<float, float> {
        float yPos = 0.5f, xPos = 0.5f;
        switch (pos) {
            case Position::GK:  yPos = 0.95f; xPos = 0.50f; break;
            case Position::DR:  yPos = 0.80f; xPos = 0.78f; break;
            case Position::DC:  yPos = 0.83f; xPos = 0.50f; break;
            case Position::DL:  yPos = 0.80f; xPos = 0.22f; break;
            case Position::WBR: yPos = 0.74f; xPos = 0.88f; break;
            case Position::WBL: yPos = 0.74f; xPos = 0.12f; break;
            case Position::DM:  yPos = 0.65f; xPos = 0.50f; break;
            case Position::MR:  yPos = 0.48f; xPos = 0.85f; break;
            case Position::MC:  yPos = 0.48f; xPos = 0.50f; break;
            case Position::ML:  yPos = 0.48f; xPos = 0.15f; break;
            case Position::AMR: yPos = 0.28f; xPos = 0.80f; break;
            case Position::AMC: yPos = 0.28f; xPos = 0.50f; break;
            case Position::AML: yPos = 0.28f; xPos = 0.20f; break;
            case Position::FR:  yPos = 0.08f; xPos = 0.75f; break;
            case Position::FC:  yPos = 0.08f; xPos = 0.50f; break;
            case Position::FL:  yPos = 0.08f; xPos = 0.25f; break;
        }
        return {xPos, yPos};
    };

    // Draw playable positions
    for (Position pos : p.playablePositions) {
        auto [xNorm, yNorm] = getPosCoords(pos);
        ImVec2 dotPos(pitchMin.x + pitchW * xNorm, pitchMin.y + pitchH * yNorm);

        // Primary position is larger and brighter
        bool isPrimary = (pos == p.primaryPos);
        float radius = isPrimary ? 16.0f : 12.0f;
        ImU32 dotColor = isPrimary ? IM_COL32(255, 220, 50, 255) : IM_COL32(100, 200, 255, 255);

        dl->AddCircleFilled(dotPos, radius, dotColor);
        dl->AddCircle(dotPos, radius, IM_COL32(255, 255, 255, 255), 32, 2.0f);

        // Label - smaller font for smaller pitch
        const char* label = PosName(pos).c_str();
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
        float oldScale = ImGui::GetFont()->Scale;
        ImGui::GetFont()->Scale = 0.85f;
        ImGui::PushFont(ImGui::GetFont());
        ImVec2 textSize = ImGui::CalcTextSize(label);
        ImVec2 textPos(dotPos.x - textSize.x * 0.5f, dotPos.y - textSize.y * 0.5f);
        dl->AddText(textPos, IM_COL32(0, 0, 0, 255), label);
        ImGui::PopFont();
        ImGui::GetFont()->Scale = oldScale;
        ImGui::PopStyleVar();
    }

    ImGui::Dummy(ImVec2(pitchW + offsetX * 2, pitchH + offsetY * 2));

    ImGui::Spacing();
    ImGui::Spacing();

    // Legend
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "Yellow");
    ImGui::SameLine();
    ImGui::Text("= Primary");

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Blue");
    ImGui::SameLine();
    ImGui::Text("= Can Play");

    ImGui::EndChild();

    ImGui::End();
}

}  // namespace nm
