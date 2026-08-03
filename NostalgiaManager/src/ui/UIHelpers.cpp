#include "UIHelpers.h"
#include <algorithm>
#include <cctype>
#include <map>
#include "core/Player.h"

namespace nm {

const char* const kFormations[] = {"4-4-2",   "4-3-3", "3-5-2",   "4-5-1",
                                   "5-3-2",   "4-4-1-1", "3-4-3", "4-2-3-1",
                                   "5-4-1",   "4-1-4-1"};

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

bool contains(const std::string& hay, const std::string& needle) {
    if (needle.empty()) return true;
    return lower(hay).find(lower(needle)) != std::string::npos;
}

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

std::string shortName(const std::string& full) {
    size_t sp = full.find(' ');
    if (sp == std::string::npos || sp == 0) return full;
    std::string surname = full.substr(sp + 1);
    while (!surname.empty() && surname.front() == ' ') surname.erase(surname.begin());
    if (surname.empty()) return full;
    return std::string(1, full[0]) + "." + surname;
}

std::string playablePosStr(const Player& p) {
    std::string s;
    for (Position pos : p.playablePositions) {
        if (!s.empty()) s += ", ";
        s += PosName(pos);
    }
    return s.empty() ? PosName(p.primaryPos) : s;
}

std::string playablePosByProficiency(const Player& p) {
    // Categorize positions by rating:
    // Preferred: primary position
    // Natural: rating = 100
    // Accomplished: rating 70-99
    // Competent: rating 40-69
    // Unconvincing: rating 1-39

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

    std::string result;

    auto addCategory = [&](const std::string& label, const std::vector<Position>& positions) {
        if (!positions.empty()) {
            if (!result.empty()) result += "\n";
            result += label + ": ";
            for (size_t i = 0; i < positions.size(); ++i) {
                if (i > 0) result += ", ";
                result += PosName(positions[i]);
            }
        }
    };

    addCategory("Preferred", preferred);
    addCategory("Natural", natural);
    addCategory("Accomplished", accomplished);
    addCategory("Competent", competent);
    addCategory("Unconvincing", unconvincing);

    return result.empty() ? "No positions" : result;
}

void positionTooltip(const Player& p) {
    if (!ImGui::IsItemHovered()) return;
    ImGui::BeginTooltip();
    ImGui::Text("Positions by proficiency:");
    ImGui::Separator();

    // Display categorized positions
    std::string categorized = playablePosByProficiency(p);

    // Split by newlines and display each category
    size_t start = 0;
    size_t end = categorized.find('\n');
    while (end != std::string::npos) {
        std::string line = categorized.substr(start, end - start);
        ImGui::TextUnformatted(line.c_str());
        start = end + 1;
        end = categorized.find('\n', start);
    }
    // Last line
    if (start < categorized.size()) {
        ImGui::TextUnformatted(categorized.substr(start).c_str());
    }

    ImGui::EndTooltip();
}

ImU32 shade(ImU32 c, float m) {
    ImVec4 v = ImGui::ColorConvertU32ToFloat4(c);
    auto cl = [](float x) { return x < 0 ? 0.f : (x > 1 ? 1.f : x); };
    return ImGui::ColorConvertFloat4ToU32(ImVec4(cl(v.x * m), cl(v.y * m), cl(v.z * m), v.w));
}

ImU32 parseColor(const std::string& colorName) {
    std::string lower = colorName;
    for (char& c : lower) c = std::tolower(c);

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

    return IM_COL32(240, 240, 240, 255);
}

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

void panelHeader(const char* title, ImU32 col) {
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

void squadPanel(const char* id, const char* title, const Team* t, const ImVec2& size,
                const std::map<int, App::PlayerMatchStats>* stats,
                std::function<void(const Player*)> onPlayerClick) {
    ImGui::BeginChild(id, size, true);
    panelHeader(title);
    if (!t) {
        ImGui::TextDisabled("-");
        ImGui::EndChild();
        return;
    }

    std::vector<const Player*> starters, subs;
    for (int pid : t->startingXI)
        for (const auto& p : t->squad)
            if (p.id == pid) { starters.push_back(&p); break; }
    for (const auto& p : t->squad) {
        bool isStarter = std::find(t->startingXI.begin(), t->startingXI.end(), p.id) !=
                         t->startingXI.end();
        if (!isStarter) subs.push_back(&p);
    }

    auto calculateRating = [](const App::PlayerMatchStats& ps) -> float {
        if (ps.minutesPlayed < 1) return 0.0f;
        float rating = 6.0f;
        rating += ps.goals * 1.5f;
        rating += ps.assists * 1.0f;
        if (ps.shots > 0) {
            float shotAccuracy = (float)ps.shotsOnTarget / ps.shots;
            rating += (ps.shots * 0.1f) + (shotAccuracy * 0.5f);
        }
        if (ps.passes > 0) {
            float passAccuracy = (float)ps.passesCompleted / ps.passes;
            rating += (passAccuracy - 0.7f) * 2.0f;
        }
        rating += ps.tackles * 0.2f;
        rating += ps.interceptions * 0.2f;
        rating -= ps.fouls * 0.3f;
        if (rating < 1.0f) rating = 1.0f;
        if (rating > 10.0f) rating = 10.0f;
        return rating;
    };

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.86f, 0.78f, 0.55f, 1));
    ImGui::Columns(4, "player_headers", true);
    ImGui::SetColumnWidth(0, size.x * 0.50f);
    ImGui::SetColumnWidth(1, size.x * 0.15f);
    ImGui::SetColumnWidth(2, size.x * 0.15f);
    ImGui::SetColumnWidth(3, size.x * 0.20f);
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

        char nameLabel[256];
        std::string posStr = cmPositionFormat(*p);
        std::snprintf(nameLabel, sizeof(nameLabel), "%2d %-8s %s", 
                     p->shirtNumber, posStr.c_str(),
                     shortName(p->name).c_str());

        if (onPlayerClick) {
            if (ImGui::Selectable(nameLabel, false)) {
                onPlayerClick(p);
            }
        } else {
            ImGui::Text("%s", nameLabel);
        }
        ImGui::NextColumn();

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

void tacticRow(const char* label, const std::string& value) {
    ImGui::TextColored(ImVec4(0.78f, 0.70f, 0.50f, 1), "%s", label);
    ImGui::SameLine(150);
    ImGui::TextColored(ImVec4(0.95f, 0.92f, 0.82f, 1), "%s", value.c_str());
}

void drawCalendar(int& viewYear, int& viewMonth, int currentYear, int currentMonth, int currentDay,
                  const std::vector<std::pair<int, std::string>>& events) {
    // Helper to get days in month
    auto getDaysInMonth = [](int year, int month) -> int {
        if (month == 2) {
            // Leap year check
            if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
                return 29;
            return 28;
        }
        if (month == 4 || month == 6 || month == 9 || month == 11)
            return 30;
        return 31;
    };

    // Helper to get first day of month (0 = Sunday, 1 = Monday, etc.)
    auto getFirstDayOfMonth = [](int year, int month) -> int {
        // Zeller's congruence for Gregorian calendar
        if (month < 3) {
            month += 12;
            year--;
        }
        int q = 1;  // day of month
        int m = month;
        int k = year % 100;
        int j = year / 100;
        int h = (q + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
        // Convert Saturday = 0 to Sunday = 0, Monday = 1, etc.
        return (h + 6) % 7;
    };

    const char* monthNames[] = {"January", "February", "March", "April", "May", "June",
                                "July", "August", "September", "October", "November", "December"};

    // Navigation buttons
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.4f, 0.5f, 0.8f));
    if (ImGui::ArrowButton("##prev_month", ImGuiDir_Left)) {
        viewMonth--;
        if (viewMonth < 1) {
            viewMonth = 12;
            viewYear--;
        }
    }
    ImGui::SameLine();

    ImGui::Text("%s %d", monthNames[viewMonth - 1], viewYear);

    ImGui::SameLine();
    if (ImGui::ArrowButton("##next_month", ImGuiDir_Right)) {
        viewMonth++;
        if (viewMonth > 12) {
            viewMonth = 1;
            viewYear++;
        }
    }
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // Calendar grid
    const char* dayNames[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    float cellSize = 30.0f;

    // Day headers
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.85f, 0.6f, 1));
    for (int i = 0; i < 7; ++i) {
        ImGui::Text("%s", dayNames[i]);
        if (i < 6) ImGui::SameLine();
    }
    ImGui::PopStyleColor();

    ImGui::Separator();

    int daysInMonth = getDaysInMonth(viewYear, viewMonth);
    int firstDay = getFirstDayOfMonth(viewYear, viewMonth);

    // Draw calendar cells
    int day = 1;
    for (int week = 0; week < 6 && day <= daysInMonth; ++week) {
        for (int dow = 0; dow < 7; ++dow) {
            if (week == 0 && dow < firstDay) {
                // Empty cell before month starts
                ImGui::Text("  ");
            } else if (day <= daysInMonth) {
                bool isToday = (viewYear == currentYear && viewMonth == currentMonth && day == currentDay);
                bool hasEvent = false;

                // Check if this day has events
                for (const auto& event : events) {
                    if (event.first == day) {
                        hasEvent = true;
                        break;
                    }
                }

                if (isToday) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.3f, 1));
                    ImGui::Text("%2d", day);
                    ImGui::PopStyleColor();
                } else if (hasEvent) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.5f, 1));
                    ImGui::Text("%2d", day);
                    ImGui::PopStyleColor();
                } else {
                    ImGui::Text("%2d", day);
                }

                // Tooltip for events
                if (hasEvent && ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    for (const auto& event : events) {
                        if (event.first == day) {
                            ImGui::TextUnformatted(event.second.c_str());
                        }
                    }
                    ImGui::EndTooltip();
                }

                day++;
            } else {
                // Empty cell after month ends
                ImGui::Text("  ");
            }

            if (dow < 6) ImGui::SameLine();
        }
    }
}

}  // namespace nm
