#pragma once
#include <functional>
#include <map>
#include <string>
#include "imgui.h"
#include "../core/Player.h"
#include "../core/Team.h"
#include "App.h"

namespace nm {

// Utility functions for string manipulation and comparisons
std::string lower(std::string s);
bool contains(const std::string& hay, const std::string& needle);
bool parseScore(const std::string& text, int& h, int& a);

// Player utility functions
double playerOverall(const Player& p);
std::string shortName(const std::string& full);
std::string playablePosStr(const Player& p);
std::string playablePosByProficiency(const Player& p);  // Categorized by proficiency
std::string cmPositionFormat(const Player& p);  // Championship Manager style format (e.g., "M C/R", "M/AM R/L, F C/R/L")
void positionTooltip(const Player& p);

// Color and visual utilities
ImU32 shade(ImU32 c, float m);
ImU32 parseColor(const std::string& colorName);

// UI widgets
bool tintButton(const char* label, ImU32 base, const ImVec2& size);
void panelHeader(const char* title, ImU32 col = IM_COL32(120, 70, 40, 255));
void squadPanel(const char* id, const char* title, const Team* t, const ImVec2& size,
                const std::map<int, App::PlayerMatchStats>* stats = nullptr,
                std::function<void(const Player*)> onPlayerClick = nullptr);

// Tactics helpers
const Player* bestStarterFor(Team* t, const char* attr, bool outfieldOnly);
void tacticRow(const char* label, const std::string& value);
extern const char* const kFormations[10];
constexpr int kMaxMatchSubs = 3;
constexpr int kNumFormations = 10;

}  // namespace nm
