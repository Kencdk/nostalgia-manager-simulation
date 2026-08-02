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
#include "PlayerDetail.h"
#include "TeamOverview.h"
#include "CareerModeBase.h"
#include "MatchDay.h"
#include "Tactics.h"

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
void panelHeader(const char* title, ImU32 col = IM_COL32(70, 85, 110, 255)) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    float h = ImGui::GetTextLineHeight() + 10;
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), col, 4.0f);
    dl->AddRect(p, ImVec2(p.x + w, p.y + h), IM_COL32(100, 120, 150, 255), 4.0f, 0, 1.5f);
    ImVec2 ts = ImGui::CalcTextSize(title);
    dl->AddText(ImVec2(p.x + (w - ts.x) * 0.5f, p.y + 5), IM_COL32(240, 245, 250, 255), title);
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
    // Sort starters by formation position: GK, D, DM, M, AM, F then R/C/L
    auto positionSortKey = [](Position pos) -> int {
        int roleRank = 3;
        switch (pos) {
            case Position::GK: roleRank = 0; break;
            case Position::DR: case Position::DC: case Position::DL:
            case Position::WBR: case Position::WBL: roleRank = 1; break;
            case Position::DM: roleRank = 2; break;
            case Position::MR: case Position::MC: case Position::ML: roleRank = 3; break;
            case Position::AMR: case Position::AMC: case Position::AML: roleRank = 4; break;
            case Position::FR: case Position::FC: case Position::FL: roleRank = 5; break;
        }
        int sideRank = 1;
        switch (pos) {
            case Position::DR: case Position::WBR: case Position::MR:
            case Position::AMR: case Position::FR: sideRank = 0; break;
            case Position::DC: case Position::DM: case Position::MC:
            case Position::AMC: case Position::FC: case Position::GK: sideRank = 1; break;
            case Position::DL: case Position::WBL: case Position::ML:
            case Position::AML: case Position::FL: sideRank = 2; break;
        }
        return roleRank * 10 + sideRank;
    };

    std::vector<size_t> xiOrder(t->startingXI.size());
    for (size_t i = 0; i < xiOrder.size(); ++i) xiOrder[i] = i;
    std::sort(xiOrder.begin(), xiOrder.end(), [&](size_t a, size_t b) {
        Position posA = a < t->assignedPositions.size() ? t->assignedPositions[a] : Position::MC;
        Position posB = b < t->assignedPositions.size() ? t->assignedPositions[b] : Position::MC;
        return positionSortKey(posA) < positionSortKey(posB);
    });

    std::vector<const Player*> subs;
    for (const auto& p : t->squad) {
        bool isStarter = std::find(t->startingXI.begin(), t->startingXI.end(), p.id) != t->startingXI.end();
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

    auto line = [&](const Player* p, Position assignedPos) {
        ImGui::Columns(4, "player_stats", true);
        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.50f);
        ImGui::SetColumnWidth(1, ImGui::GetWindowWidth() * 0.15f);
        ImGui::SetColumnWidth(2, ImGui::GetWindowWidth() * 0.15f);
        ImGui::SetColumnWidth(3, ImGui::GetWindowWidth() * 0.20f);

        char nameLabel[256];
        std::snprintf(nameLabel, sizeof(nameLabel), "%2d %-3s %s",
                     p->shirtNumber, PosName(assignedPos).c_str(),
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
    for (size_t idx : xiOrder) {
        const Player* p = nullptr;
        for (const auto& pl : t->squad)
            if (pl.id == t->startingXI[idx]) { p = &pl; break; }
        if (!p) continue;
        Position assignedPos = idx < t->assignedPositions.size() ? t->assignedPositions[idx] : p->primaryPos;
        line(p, assignedPos);
    }
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.86f, 0.78f, 0.55f, 1), "Substitutes");
    int shown = 0;
    for (const Player* p : subs) {
        if (shown++ >= 5) break;
        line(p, p->primaryPos);
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
    c[ImGuiCol_WindowBg]        = col(45, 52, 64);          // dark blue-grey background
    c[ImGuiCol_ChildBg]         = col(55, 62, 74, 235);     // medium dark blue-grey
    c[ImGuiCol_PopupBg]         = col(50, 57, 69, 245);
    c[ImGuiCol_Border]          = col(90, 100, 120);        // light blue-grey border
    c[ImGuiCol_FrameBg]         = col(60, 68, 82);
    c[ImGuiCol_FrameBgHovered]  = col(70, 78, 95);
    c[ImGuiCol_FrameBgActive]   = col(80, 88, 105);
    c[ImGuiCol_TitleBgActive]   = col(60, 70, 88);
    c[ImGuiCol_Button]          = col(70, 85, 110);
    c[ImGuiCol_ButtonHovered]   = col(85, 100, 125);
    c[ImGuiCol_ButtonActive]    = col(60, 75, 100);
    c[ImGuiCol_Header]          = col(75, 90, 115);
    c[ImGuiCol_HeaderHovered]   = col(85, 100, 130);
    c[ImGuiCol_HeaderActive]    = col(70, 85, 110);
    c[ImGuiCol_TableHeaderBg]   = col(65, 78, 98);
    c[ImGuiCol_TableRowBg]      = col(50, 57, 69);
    c[ImGuiCol_TableRowBgAlt]   = col(55, 62, 74);
    c[ImGuiCol_TableBorderStrong] = col(90, 100, 120);
    c[ImGuiCol_TableBorderLight]  = col(70, 80, 95);
    c[ImGuiCol_Text]            = col(240, 245, 250);       // white text
    c[ImGuiCol_TextDisabled]    = col(140, 150, 165);
    c[ImGuiCol_CheckMark]       = col(120, 160, 200);
    c[ImGuiCol_SliderGrab]      = col(100, 130, 170);
    c[ImGuiCol_SliderGrabActive]= col(120, 150, 190);
    c[ImGuiCol_ScrollbarGrab]   = col(90, 105, 130);
    c[ImGuiCol_Separator]       = col(80, 95, 115);
}

bool App::init(const std::string& dataDir) {
    dataDir_ = dataDir;
    ApplyNostalgiaTheme();
    cfg_.loadFile(dataDir_ + "/engine.cfg");
    if (!db_.load(dataDir_)) {
        status_ = "Failed to load database from " + dataDir_;
        return false;
    }
    AppLoadTexture(dataDir_ + "/images/NMSstart.png", &menuBg_);
    // Load Player Details background from file
    AppLoadTexture(dataDir_ + "/images/Playerdetails.png", &playerDetailBg_);
    // Load Team Overview background from file
    AppLoadTexture(dataDir_ + "/images/Teambackground.png", &teamOverviewBg_);
    // Load Career Mode Base background from file
    AppLoadTexture(dataDir_ + "/images/Careermodebase.png", &careerModeBaseBg_);

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

    // Build sorted unique nations list
    nations_.clear();
    for (const auto& t : db_.teams) {
        if (!t.nation.empty() &&
            std::find(nations_.begin(), nations_.end(), t.nation) == nations_.end())
            nations_.push_back(t.nation);
    }
    std::sort(nations_.begin(), nations_.end());

    status_ = "Loaded " + std::to_string(db_.teams.size()) + " teams across " +
              std::to_string(leagues_.size()) + " leagues.";
    return true;
}

Team* App::teamById(int id) { return db_.findTeam(id); }

void App::beginScreen(const char* title, bool withBackground) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (!withBackground) flags |= ImGuiWindowFlags_NoBackground;
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

void App::drawStaticBackground(const AppTexture& texture) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 pos = vp->WorkPos;
    const ImVec2 size = vp->WorkSize;
    ImDrawList* bg = ImGui::GetBackgroundDrawList();

    if (texture.ok && texture.w > 0 && texture.h > 0) {
        // Scale image to cover the window
        float ws = size.x / size.y;
        float is = (float)texture.w / (float)texture.h;
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
        bg->AddImage(texture.id, pos, ImVec2(pos.x + size.x, pos.y + size.y), uv0, uv1);

        // Add semi-transparent dark overlay so text is readable
        bg->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), 
                         IM_COL32(0, 0, 0, 120));
    } else {
        // Fallback: gradient background if texture not loaded
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
        case Screen::CareerSetup: renderCareerSetup(); break;
        case Screen::CareerModeBase: renderCareerModeBase(); break;
        case Screen::MatchDay: renderMatchDay(); break;
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
    }

    // Title bar — same colour as the buttons
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    const float titleScale = 2.5f;
    ImGui::SetWindowFontScale(titleScale);

    const ImU32 barCol   = IM_COL32(70, 90, 120, 255);
    const ImU32 barEdge  = IM_COL32(100, 125, 160, 255);
    const ImU32 textCol  = IM_COL32(230, 235, 245, 255);
    const ImU32 subCol   = IM_COL32(180, 195, 215, 255);

    ImVec2 line1Size = ImGui::CalcTextSize("Nostalgia Manager Simulation");
    ImVec2 line2Size = ImGui::CalcTextSize("by TBGreenbear");

    const float barPadV = 14.0f;
    float barH = line1Size.y + line2Size.y + barPadV * 2.0f + 8.0f;
    ImVec2 barMin(pos.x, pos.y);
    ImVec2 barMax(pos.x + size.x, pos.y + barH);

    bg->AddRectFilled(barMin, barMax, barCol);
    bg->AddRect(barMin, barMax, barEdge, 0.0f, 0, 2.0f);

    float line1Y = pos.y + barPadV;
    float line2Y = line1Y + line1Size.y + 6.0f;

    bg->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                ImVec2(pos.x + (size.x - line1Size.x) * 0.5f, line1Y),
                textCol, "Nostalgia Manager Simulation");

    ImGui::SetWindowFontScale(1.6f);
    ImVec2 sub2Size = ImGui::CalcTextSize("by TBGreenbear");
    bg->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                ImVec2(pos.x + (size.x - sub2Size.x) * 0.5f, line2Y),
                subCol, "by TBGreenbear");

    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();

    beginFullscreen("##main", false);

    // Centred vertical stack of glossy coloured buttons, as in the mock-up.
    const ImVec2 bsz(430, 58);
    const float gap = 14.0f;
    struct Item { const char* label; ImU32 col; Screen scr; };
    const Item items[] = {
        {"Friendly Match", IM_COL32(70, 90, 120, 255), Screen::Friendly},
        {"Career Game", IM_COL32(70, 90, 120, 255), Screen::CareerSetup},
        {"Load Game", IM_COL32(70, 90, 120, 255), Screen::Career},
        {"Edit Database", IM_COL32(70, 90, 120, 255), Screen::Database},
        {"Exit", IM_COL32(70, 90, 120, 255), Screen::Main},
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

void App::teamPicker(const char* id, int& countryIdx, int& leagueIdx, int& teamId,
                     char* filter, size_t filterSz) {
    ImGui::PushID(id);
    if (nations_.empty()) {
        ImGui::TextDisabled("No teams loaded.");
        ImGui::PopID();
        return;
    }

    // Clamp indices
    if (countryIdx < 0 || countryIdx >= static_cast<int>(nations_.size())) countryIdx = 0;

    // --- Country combo ---
    if (ImGui::BeginCombo("Country", nations_[countryIdx].c_str())) {
        for (int i = 0; i < static_cast<int>(nations_.size()); ++i) {
            bool sel = (i == countryIdx);
            if (ImGui::Selectable(nations_[i].c_str(), sel)) {
                if (i != countryIdx) {
                    countryIdx = i;
                    leagueIdx = 0;
                    teamId = -1;
                }
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Build leagues for this country
    const std::string& nation = nations_[countryIdx];
    std::vector<std::string> countryLeagues;
    for (const auto& t : db_.teams)
        if (t.nation == nation &&
            std::find(countryLeagues.begin(), countryLeagues.end(), t.league) == countryLeagues.end())
            countryLeagues.push_back(t.league);
    std::sort(countryLeagues.begin(), countryLeagues.end());

    if (leagueIdx < 0 || leagueIdx >= static_cast<int>(countryLeagues.size())) leagueIdx = 0;

    // --- League combo ---
    const char* leagueLabel = countryLeagues.empty() ? "(none)" : countryLeagues[leagueIdx].c_str();
    if (!countryLeagues.empty() && ImGui::BeginCombo("League", leagueLabel)) {
        for (int i = 0; i < static_cast<int>(countryLeagues.size()); ++i) {
            bool sel = (i == leagueIdx);
            if (ImGui::Selectable(countryLeagues[i].c_str(), sel)) {
                if (i != leagueIdx) {
                    leagueIdx = i;
                    teamId = -1;
                }
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // --- Team list ---
    ImGui::InputTextWithHint("Filter", "type to filter teams", filter, filterSz);

    std::vector<Team*> teams;
    if (!countryLeagues.empty()) {
        const std::string& league = countryLeagues[leagueIdx];
        for (auto& t : db_.teams)
            if (t.nation == nation && t.league == league)
                teams.push_back(&t);
    }
    std::sort(teams.begin(), teams.end(), [](Team* a, Team* b) { return a->name < b->name; });

    ImGui::BeginChild("teamlist", ImVec2(360, 200), true);
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
    teamPicker("home", homeCountry_, homeLeague_, homeTeam_, homeFilter_, sizeof(homeFilter_));
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1, 0.7f, 0.5f, 1), "Away");
    teamPicker("away", awayCountry_, awayLeague_, awayTeam_, awayFilter_, sizeof(awayFilter_));
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
    TacticsScreen::render(this);
}

void App::startMatch(Team* home, Team* away) {
    frames_.clear();
    playIdx_ = 0;
    playAccum_ = 0.0;
    playing_ = true;
    matchOver_ = false;
    halftimePause_ = false;
    halftimeIdx_ = 0;
    matchTab_ = 1;
    statsComputedIdx_ = 0;
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
        if (p) homePlayerStats_[p->shirtNumber] = PlayerMatchStats();
    }
    for (int pid : away->startingXI) {
        const Player* p = away->findPlayer(pid);
        if (p) awayPlayerStats_[p->shirtNumber] = PlayerMatchStats();
    }

    MatchEngine engine(cfg_, static_cast<unsigned>(std::time(nullptr)));
    MatchEngine* ep = &engine;
    auto hook = [this, ep](const MatchEvent& e) {
        Frame f;
        f.text = e.text;
        f.key = e.key;
        f.minute = static_cast<int>(e.minute);
        f.pitch = ep->renderPitch();
        f.ballX = ep->ballX();
        f.ballY = ep->ballY();
        f.ballCol = ep->ballCol();
        f.ballRow = ep->ballRow();
        f.carrier = ep->carrierSide();
        f.stats = ep->stats();
        auto playerSnaps = ep->getPlayerPositions();
        for (const auto& ps : playerSnaps) {
            Frame::PlayerPos pp;
            pp.shirtNumber = ps.shirtNumber;
            pp.x = ps.x; pp.y = ps.y;
            pp.col = ps.col; pp.row = ps.row;
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

    // Assign per-frame cumulative scores (needed for scoreboard rendering)
    int hg = 0, ag = 0;
    for (auto& f : frames_) {
        int h, a;
        if (f.text.rfind("GOAL!", 0) == 0 && parseScore(f.text, h, a)) {
            hg = h; ag = a;
        }
        f.hg = hg;
        f.ag = ag;
    }

    // Find halftime (around minute 45)
    for (size_t i = 0; i < frames_.size(); ++i) {
        if (frames_[i].minute >= 45) { halftimeIdx_ = i; break; }
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

    // Advance player stats up to the current playback position
    {
        // Helper to extract the first shirt number from a text string
        auto extractShirt = [](const std::string& text) -> int {
            size_t h = text.find('#');
            if (h == std::string::npos) return -1;
            size_t e = h + 1;
            while (e < text.size() && std::isdigit(text[e])) ++e;
            return (e > h + 1) ? std::stoi(text.substr(h + 1, e - h - 1)) : -1;
        };
        auto extractSecondShirt = [](const std::string& text) -> int {
            size_t h = text.find('#');
            if (h == std::string::npos) return -1;
            h = text.find('#', h + 1);
            if (h == std::string::npos) return -1;
            size_t e = h + 1;
            while (e < text.size() && std::isdigit(text[e])) ++e;
            return (e > h + 1) ? std::stoi(text.substr(h + 1, e - h - 1)) : -1;
        };

        int lastMinute = 0;
        if (statsComputedIdx_ > 0)
            lastMinute = frames_[statsComputedIdx_ - 1].minute;

        while (statsComputedIdx_ < playIdx_ && statsComputedIdx_ < frames_.size()) {
            const Frame& fr = frames_[statsComputedIdx_];
            const std::string& text = fr.text;

            // Minutes played
            if (fr.minute > lastMinute) {
                for (auto& ps : homePlayerStats_) ps.second.minutesPlayed++;
                for (auto& ps : awayPlayerStats_) ps.second.minutesPlayed++;
                lastMinute = fr.minute;
            }

            // Goals / assists
            int h, a;
            if (text.rfind("GOAL!", 0) == 0 && parseScore(text, h, a)) {
                int sn = extractShirt(text);
                int an = -1;
                // Assist: second # in text
                size_t assistTag = text.find("(assist:");
                if (assistTag != std::string::npos) {
                    size_t ah = text.find('#', assistTag);
                    if (ah != std::string::npos) {
                        size_t ae = ah + 1;
                        while (ae < text.size() && std::isdigit(text[ae])) ++ae;
                        if (ae > ah + 1) an = std::stoi(text.substr(ah + 1, ae - ah - 1));
                    }
                }
                // Scorer name for scorers list
                std::string who;
                size_t sp = text.find(' '), ep2 = text.find(" scores!");
                if (sp != std::string::npos && ep2 != std::string::npos && ep2 > sp) {
                    who = text.substr(sp + 1, ep2 - sp - 1);
                    size_t ap2 = who.find(" (assist:");
                    if (ap2 != std::string::npos) who = who.substr(0, ap2);
                }
                char scorerLine[160];
                std::snprintf(scorerLine, sizeof(scorerLine), "%s  %d'", who.c_str(), fr.minute);

                int prevHg = statsComputedIdx_ > 0 ? frames_[statsComputedIdx_ - 1].hg : 0;
                int prevAg = statsComputedIdx_ > 0 ? frames_[statsComputedIdx_ - 1].ag : 0;
                if (h > prevHg) {
                    homeScorers_.emplace_back(fr.minute, scorerLine);
                    if (sn >= 0 && homePlayerStats_.count(sn)) {
                        homePlayerStats_[sn].goals++;
                        homePlayerStats_[sn].shots++;
                        homePlayerStats_[sn].shotsOnTarget++;
                    }
                    if (an >= 0 && homePlayerStats_.count(an)) homePlayerStats_[an].assists++;
                } else if (a > prevAg) {
                    awayScorers_.emplace_back(fr.minute, scorerLine);
                    if (sn >= 0 && awayPlayerStats_.count(sn)) {
                        awayPlayerStats_[sn].goals++;
                        awayPlayerStats_[sn].shots++;
                        awayPlayerStats_[sn].shotsOnTarget++;
                    }
                    if (an >= 0 && awayPlayerStats_.count(an)) awayPlayerStats_[an].assists++;
                }
            }

            // Shots
            if (text.find("shoots") != std::string::npos || text.find("lets fly") != std::string::npos ||
                text.find("rises for a header") != std::string::npos) {
                int sn = extractShirt(text);
                bool onTgt = text.find("SAVED") != std::string::npos || text.find("saved") != std::string::npos;
                if (sn >= 0) {
                    if (homePlayerStats_.count(sn)) {
                        homePlayerStats_[sn].shots++;
                        if (onTgt) homePlayerStats_[sn].shotsOnTarget++;
                    } else if (awayPlayerStats_.count(sn)) {
                        awayPlayerStats_[sn].shots++;
                        if (onTgt) awayPlayerStats_[sn].shotsOnTarget++;
                    }
                }
            }

            // Passes — count attempt on both success and failure events
            // Success: "passes to", "plays a long ball to", "clears under pressure", "launches it forward"
            // Failure: "misplaces a pass", "over-hits a long ball"
            {
                bool isPassSuccess = text.find("passes to") != std::string::npos ||
                                     text.find("plays a long ball to") != std::string::npos;
                bool isPassFailure = text.find("misplaces a pass") != std::string::npos ||
                                     text.find("over-hits a long ball") != std::string::npos;
                if (isPassSuccess || isPassFailure) {
                    int sn = extractShirt(text);
                    if (sn >= 0) {
                        if (homePlayerStats_.count(sn)) {
                            homePlayerStats_[sn].passes++;
                            if (isPassSuccess) homePlayerStats_[sn].passesCompleted++;
                        } else if (awayPlayerStats_.count(sn)) {
                            awayPlayerStats_[sn].passes++;
                            if (isPassSuccess) awayPlayerStats_[sn].passesCompleted++;
                        }
                    }
                }
            }

            // Tackles
            if (text.find("tackle") != std::string::npos) {
                int sn = extractShirt(text);
                if (sn >= 0) {
                    if (homePlayerStats_.count(sn)) homePlayerStats_[sn].tackles++;
                    else if (awayPlayerStats_.count(sn)) awayPlayerStats_[sn].tackles++;
                }
            }

            // Interceptions
            if (text.find("intercept") != std::string::npos) {
                int sn = extractShirt(text);
                if (sn >= 0) {
                    if (homePlayerStats_.count(sn)) homePlayerStats_[sn].interceptions++;
                    else if (awayPlayerStats_.count(sn)) awayPlayerStats_[sn].interceptions++;
                }
            }

            // Fouls
            if (text.find("foul") != std::string::npos && text.find("Foul") == std::string::npos) {
                int sn = extractShirt(text);
                if (sn >= 0) {
                    if (homePlayerStats_.count(sn)) homePlayerStats_[sn].fouls++;
                    else if (awayPlayerStats_.count(sn)) awayPlayerStats_[sn].fouls++;
                }
            }

            ++statsComputedIdx_;
        }
    }

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

    // Away team name with background (awayColor1) and text (awayColor2)
    ImU32 awayBg = matchAwayTeam_ && !matchAwayTeam_->awayColor1.empty()
        ? parseColor(matchAwayTeam_->awayColor1)
        : IM_COL32(255, 200, 140, 255);
    ImU32 awayText = matchAwayTeam_ && !matchAwayTeam_->awayColor2.empty()
        ? parseColor(matchAwayTeam_->awayColor2)
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

    // ---- Tab bar: [Home Tactics] | [Match] | [Statistics] | [Away Tactics] ----
    {
        auto isHumanTeam = [&](Team* team) -> bool {
            if (!team) return false;
            if (!careerActive_) return true;
            return team->id == careerTeam_;
        };

        // Build labels as persistent strings to avoid dangling c_str().
        std::string homeLabel = matchHome_ + " Tactics";
        std::string awayLabel = matchAway_ + " Tactics";

        struct TabInfo { const char* label; bool disabled; };
        TabInfo tabs[] = {
            { homeLabel.c_str(),  !isHumanTeam(matchHomeTeam_) },
            { "Match",            false },
            { "Statistics",       false },
            { awayLabel.c_str(),  !isHumanTeam(matchAwayTeam_) },
        };
        const int tabCount = 4;
        const float tabH = 32.0f;
        const float tabW = fullW / tabCount;
        const float tabBarY = ImGui::GetCursorScreenPos().y;
        ImDrawList* tabDl = ImGui::GetWindowDrawList();

        for (int i = 0; i < tabCount; ++i) {
            ImVec2 tabMin(ImGui::GetCursorScreenPos().x + i * tabW, tabBarY);
            ImVec2 tabMax(tabMin.x + tabW - 2, tabBarY + tabH);
            bool active = (matchTab_ == i);
            bool disabled = tabs[i].disabled;
            ImU32 tabBg = disabled  ? IM_COL32(40, 40, 40, 160)
                        : active    ? IM_COL32(40, 80, 40, 255)
                                    : IM_COL32(30, 50, 30, 200);
            ImU32 tabFg = disabled  ? IM_COL32(100, 100, 100, 255)
                        : active    ? IM_COL32(255, 255, 255, 255)
                                    : IM_COL32(180, 200, 180, 255);
            tabDl->AddRectFilled(tabMin, tabMax, tabBg, 4.0f);
            if (active) tabDl->AddRect(tabMin, tabMax, IM_COL32(100, 180, 100, 255), 4.0f);
            ImVec2 textSz = ImGui::CalcTextSize(tabs[i].label);
            tabDl->AddText(ImVec2(tabMin.x + (tabW - 2 - textSz.x) * 0.5f,
                                  tabMin.y + (tabH - textSz.y) * 0.5f),
                           tabFg, tabs[i].label);
            ImGui::SetCursorScreenPos(tabMin);
            ImGui::PushID(i);
            if (!disabled && ImGui::InvisibleButton("##tab", ImVec2(tabW - 2, tabH))) {
                if (i == 0) {
                    playing_ = false;
                    openTactics(matchHomeTeam_, Screen::Match);
                } else if (i == 3) {
                    playing_ = false;
                    openTactics(matchAwayTeam_, Screen::Match);
                } else {
                    matchTab_ = i;
                }
            }
            ImGui::PopID();
        }
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, tabBarY + tabH + 4));
    }
    ImGui::Separator();

    if (matchTab_ == 1) {
    // ---- New Layout: [Team 1 - Full Height] | [Pitch + Commentary] | [Team 2 - Full Height] ----
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float bottomH = 54.0f;
    const float feedH = 180.0f;  // Commentary height

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
               ImVec2(sideW, totalAvailH),
               &homePlayerStats_,
               [this](const Player* p) { openPlayerDetail(p, Screen::Match); });
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
               ImVec2(sideW, totalAvailH),
               &awayPlayerStats_,
               [this](const Player* p) { openPlayerDetail(p, Screen::Match); });
    ImGui::EndGroup();

    // ---- Bottom control bar ----
    if (matchOver_) {
        // Check if this is a career mode match
        if (careerMatchPending_) {
            if (tintButton("Continue Career", IM_COL32(40, 92, 178, 255), ImVec2(220, bottomH - 10))) {
                // Process player match result and simulate remaining fixtures
                // First, update standings for player's match (already in frames_/finalHG_/finalAG_)
                Team* home = matchHomeTeam_;
                Team* away = matchAwayTeam_;
                if (home && away) {
                    Standing& sh = table_[home->id];
                    Standing& sa = table_[away->id];
                    sh.p++; sa.p++;
                    sh.gf += finalHG_; sh.ga += finalAG_;
                    sa.gf += finalAG_; sa.ga += finalHG_;
                    if (finalHG_ > finalAG_) { sh.w++; sh.pts += 3; sa.l++; }
                    else if (finalHG_ < finalAG_) { sa.w++; sa.pts += 3; sh.l++; }
                    else { sh.d++; sa.d++; sh.pts++; sa.pts++; }

                    // Log the result
                    char line[160];
                    std::snprintf(line, sizeof(line), "R%d: %s %d-%d %s", careerRound_ + 1,
                                  home->name.c_str(), finalHG_, finalAG_, away->name.c_str());
                    careerLog_.push_back(line);
                }

                // Now simulate remaining matches and advance round
                careerFinishRound();
            }
        } else {
            if (tintButton("Back to Menu", IM_COL32(40, 92, 178, 255), ImVec2(220, bottomH - 10)))
                screen_ = Screen::Main;
        }
        ImGui::SameLine();
        if (tintButton("Watch Again", IM_COL32(86, 150, 38, 255), ImVec2(180, bottomH - 10))) {
            playIdx_ = 0;
            statsComputedIdx_ = 0;
            homeScorers_.clear();
            awayScorers_.clear();
            for (auto& ps : homePlayerStats_) ps.second = PlayerMatchStats();
            for (auto& ps : awayPlayerStats_) ps.second = PlayerMatchStats();
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

    } else if (matchTab_ == 2) {
        // ---- Statistics tab ----
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float availH = ImGui::GetContentRegionAvail().y - 4;
        const float halfW = (fullW - spacing) * 0.5f;

        const auto renderPlayerStatsTable = [&](const char* id, const std::string& teamName,
                                                 Team* team,
                                                 const std::map<int, PlayerMatchStats>& stats) {
            ImGui::BeginChild(id, ImVec2(halfW, availH), true);
            const ImVec4 header(0.60f, 0.75f, 0.95f, 1);
            ImGui::TextColored(header, "%s", teamName.c_str());
            ImGui::Separator();
            ImGui::Spacing();

            // Column widths
            const float nameW  = halfW * 0.28f;
            const float numW   = halfW * 0.08f;
            ImGui::Columns(10, (std::string(id) + "_cols").c_str(), true);
            ImGui::SetColumnWidth(0, nameW);
            for (int c = 1; c <= 9; ++c) ImGui::SetColumnWidth(c, numW);

            ImGui::TextColored(header, "Name");       ImGui::NextColumn();
            ImGui::TextColored(header, "Min");        ImGui::NextColumn();
            ImGui::TextColored(header, "Gls");        ImGui::NextColumn();
            ImGui::TextColored(header, "Ast");        ImGui::NextColumn();
            ImGui::TextColored(header, "Sh");         ImGui::NextColumn();
            ImGui::TextColored(header, "SoT");        ImGui::NextColumn();
            ImGui::TextColored(header, "Pas");        ImGui::NextColumn();
            ImGui::TextColored(header, "Pac%");       ImGui::NextColumn();
            ImGui::TextColored(header, "Tck");        ImGui::NextColumn();
            ImGui::TextColored(header, "Fls");        ImGui::NextColumn();
            ImGui::Separator();

            // Build sorted list: starters first (in XI order), then subs
            if (team) {
                auto renderRow = [&](const Player* p) {
                    auto it = stats.find(p->shirtNumber);
                    const PlayerMatchStats& ps = (it != stats.end()) ? it->second : PlayerMatchStats{};
                    int passPct = ps.passes > 0 ? (ps.passesCompleted * 100) / ps.passes : 0;
                    // Match rating
                    float rating = 0.0f;
                    if (ps.minutesPlayed > 0) {
                        rating = 6.0f;
                        rating += ps.goals * 1.5f;
                        rating += ps.assists * 1.0f;
                        if (ps.shots > 0) rating += (ps.shots * 0.1f) + ((float)ps.shotsOnTarget / ps.shots * 0.5f);
                        if (ps.passes > 0) rating += ((float)ps.passesCompleted / ps.passes - 0.7f) * 2.0f;
                        rating += ps.tackles * 0.2f;
                        rating -= ps.fouls * 0.3f;
                        if (rating < 1.0f) rating = 1.0f;
                        if (rating > 10.0f) rating = 10.0f;
                    }
                    // Colour the name by rating
                    ImVec4 nameCol = (rating >= 7.5f) ? ImVec4(0.3f, 1.0f, 0.4f, 1)
                                   : (rating >= 6.5f) ? ImVec4(1, 1, 1, 1)
                                   : (rating > 0.0f)  ? ImVec4(1, 0.5f, 0.4f, 1)
                                   :                    ImVec4(0.6f, 0.6f, 0.6f, 1);
                    char nameLabel[80];
                    if (rating > 0.0f)
                        std::snprintf(nameLabel, sizeof(nameLabel), "#%d %s  %.1f",
                                      p->shirtNumber, shortName(p->name).c_str(), rating);
                    else
                        std::snprintf(nameLabel, sizeof(nameLabel), "#%d %s",
                                      p->shirtNumber, shortName(p->name).c_str());
                    ImGui::TextColored(nameCol, "%s", nameLabel);    ImGui::NextColumn();
                    ImGui::Text("%d", ps.minutesPlayed);             ImGui::NextColumn();
                    ImGui::Text("%d", ps.goals);                     ImGui::NextColumn();
                    ImGui::Text("%d", ps.assists);                   ImGui::NextColumn();
                    ImGui::Text("%d", ps.shots);                     ImGui::NextColumn();
                    ImGui::Text("%d", ps.shotsOnTarget);             ImGui::NextColumn();
                    ImGui::Text("%d", ps.passes);                    ImGui::NextColumn();
                    if (ps.passes > 0) ImGui::Text("%d%%", passPct); else ImGui::Text("-");
                    ImGui::NextColumn();
                    ImGui::Text("%d", ps.tackles);                   ImGui::NextColumn();
                    ImGui::Text("%d", ps.fouls);                     ImGui::NextColumn();
                };

                // Starters sorted by formation position
                if (team) {
                    auto statsPosKey = [](Position pos) -> int {
                        int rr = 3;
                        switch (pos) {
                            case Position::GK: rr = 0; break;
                            case Position::DR: case Position::DC: case Position::DL:
                            case Position::WBR: case Position::WBL: rr = 1; break;
                            case Position::DM: rr = 2; break;
                            case Position::MR: case Position::MC: case Position::ML: rr = 3; break;
                            case Position::AMR: case Position::AMC: case Position::AML: rr = 4; break;
                            case Position::FR: case Position::FC: case Position::FL: rr = 5; break;
                        }
                        int sr = 1;
                        switch (pos) {
                            case Position::DR: case Position::WBR: case Position::MR:
                            case Position::AMR: case Position::FR: sr = 0; break;
                            case Position::DC: case Position::DM: case Position::MC:
                            case Position::AMC: case Position::FC: case Position::GK: sr = 1; break;
                            case Position::DL: case Position::WBL: case Position::ML:
                            case Position::AML: case Position::FL: sr = 2; break;
                        }
                        return rr * 10 + sr;
                    };
                    std::vector<size_t> statsXiOrder(team->startingXI.size());
                    for (size_t i = 0; i < statsXiOrder.size(); ++i) statsXiOrder[i] = i;
                    std::sort(statsXiOrder.begin(), statsXiOrder.end(), [&](size_t a, size_t b) {
                        Position pa = a < team->assignedPositions.size() ? team->assignedPositions[a] : Position::MC;
                        Position pb = b < team->assignedPositions.size() ? team->assignedPositions[b] : Position::MC;
                        return statsPosKey(pa) < statsPosKey(pb);
                    });
                    for (size_t idx : statsXiOrder) {
                        const Player* p = team->findPlayer(team->startingXI[idx]);
                        if (p) renderRow(p);
                    }
                }
                // Subs (any squad member not in XI that has minutes played)
                ImGui::Separator();
                for (const auto& pl : team->squad) {
                    bool starter = std::find(team->startingXI.begin(), team->startingXI.end(), pl.id) != team->startingXI.end();
                    if (!starter) {
                        auto it = stats.find(pl.shirtNumber);
                        if (it != stats.end() && it->second.minutesPlayed > 0) renderRow(&pl);
                    }
                }
            }
            ImGui::Columns(1);
            ImGui::EndChild();
        };

        renderPlayerStatsTable("stats_home", matchHome_, matchHomeTeam_, homePlayerStats_);
        ImGui::SameLine();
        renderPlayerStatsTable("stats_away", matchAway_, matchAwayTeam_, awayPlayerStats_);
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

    // Initialize calendar (start of season: August 1997)
    currentYear_ = 1997;
    currentMonth_ = 8;
    currentDay_ = 1;
    calendarViewYear_ = currentYear_;
    calendarViewMonth_ = currentMonth_;

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

void App::careerAdvanceToPlayerMatch() {
    if (!careerActive_) return;
    if (careerRound_ + 1 >= static_cast<int>(roundStart_.size())) return;

    size_t from = roundStart_[careerRound_];
    size_t to = roundStart_[careerRound_ + 1];

    // Find player's match in this round
    bool foundPlayerMatch = false;
    for (size_t i = from; i < to; ++i) {
        Team* h = teamById(fixtures_[i].first);
        Team* a = teamById(fixtures_[i].second);
        if (!h || !a) continue;

        if (h->id == careerTeam_ || a->id == careerTeam_) {
            // Found player's match - store it and go to tactics
            careerPlayerMatchIdx_ = i;
            careerMatchPending_ = true;
            foundPlayerMatch = true;

            // Determine which team is the player's team
            Team* playerTeam = (h->id == careerTeam_) ? h : a;

            // Open tactics for player's team
            TacticsScreen::openTactics(this, playerTeam, Screen::CareerModeBase);
            return;
        }
    }

    // If no player match found (shouldn't happen), just simulate all matches
    if (!foundPlayerMatch) {
        careerAdvance();
        screen_ = Screen::CareerModeBase;
    }
}

void App::careerFinishRound() {
    if (!careerActive_) return;
    if (careerRound_ + 1 >= static_cast<int>(roundStart_.size())) return;

    size_t from = roundStart_[careerRound_];
    size_t to = roundStart_[careerRound_ + 1];

    // Simulate all matches EXCEPT the player's match (which was already played)
    for (size_t i = from; i < to; ++i) {
        Team* h = teamById(fixtures_[i].first);
        Team* a = teamById(fixtures_[i].second);
        if (!h || !a) continue;

        // Skip the player's match (already played)
        if (i == careerPlayerMatchIdx_ && careerMatchPending_) {
            continue;
        }

        // Simulate this match
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

        // Log result if it involves player's team (shouldn't happen here)
        if (h->id == careerTeam_ || a->id == careerTeam_) {
            char line[160];
            std::snprintf(line, sizeof(line), "R%d: %s %d-%d %s", careerRound_ + 1,
                          h->name.c_str(), r.homeGoals, r.awayGoals, a->name.c_str());
            careerLog_.push_back(line);
        }
    }

    careerRound_++;
    careerMatchPending_ = false;
    screen_ = Screen::CareerModeBase;
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
    screen_ = Screen::CareerModeBase;
}

void App::renderCareer() {
    // This screen is now only used for loading saved games
    // For new games, use CareerSetup screen
    // Draw cycling background
    drawCyclingBackground();

    beginScreen("Load Career");
    if (ImGui::Button("< Back")) {
        screen_ = Screen::Main;
        ImGui::End();
        return;
    }

    ImGui::Spacing();
    ImGui::TextWrapped("Load a previously saved career game.");
    ImGui::Spacing();

    if (ImGui::Button("Load Save", ImVec2(200, 50))) {
        careerLoad();
    }

    ImGui::End();
}

void App::renderCareerSetup() {
    // Draw cycling background
    drawCyclingBackground();

    beginScreen("Career Mode Setup");
    if (ImGui::Button("< Back")) {
        screen_ = Screen::Main;
        ImGui::End();
        return;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Manager name input
    ImGui::TextColored(ImVec4(0.95f, 0.97f, 1.0f, 1), "Manager Setup");
    ImGui::Spacing();
    ImGui::Text("Enter your name:");
    ImGui::SetNextItemWidth(400);
    ImGui::InputTextWithHint("##managername", "Your name", managerName_, sizeof(managerName_));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Team selection
    ImGui::TextColored(ImVec4(0.95f, 0.97f, 1.0f, 1), "Select Your Team");
    ImGui::Spacing();
    ImGui::TextWrapped("Pick a club to manage. You'll play a full round-robin season in its league.");
    ImGui::Spacing();

    teamPicker("careersetup", careerCountry_, careerLeague_, careerTeam_, careerFilter_, sizeof(careerFilter_));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Start button
    Team* t = teamById(careerTeam_);
    bool hasName = std::strlen(managerName_) > 0;
    bool hasTeam = t && t->squad.size() >= 7;
    bool canStart = hasName && hasTeam;

    if (!canStart) ImGui::BeginDisabled();

    if (ImGui::Button("Done", ImVec2(200, 50))) {
        careerStart(careerTeam_);
        screen_ = Screen::CareerModeBase;
    }

    if (!canStart) ImGui::EndDisabled();

    if (!hasName) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1), "Please enter your name");
    } else if (!hasTeam) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1), "Please select a valid team");
    }

    ImGui::End();
}

void App::renderCareerModeBase() {
    CareerModeBaseScreen::render(this);
}

void App::renderMatchDay() {
    MatchDayScreen::render(this);
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
    PlayerDetailScreen::openPlayerDetail(this, player, returnTo);
}

void App::openTeamOverview(Team* team, Screen returnTo) {
    TeamOverviewScreen::openTeamOverview(this, team, returnTo);
}

void App::renderTeamOverview() {
    TeamOverviewScreen::render(this);
}

void App::renderPlayerDetail() {
    PlayerDetailScreen::render(this);
}

}  // namespace nm
