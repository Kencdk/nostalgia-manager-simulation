#include "TeamOverview.h"
#include "UIHelpers.h"
#include <algorithm>
#include <cstdio>
#include <map>
#include <set>

namespace nm {

void TeamOverviewScreen::openTeamOverview(App* app, Team* team, App::Screen returnTo) {
    app->teamOverviewTeam_ = team;
    app->teamOverviewReturn_ = returnTo;
    app->screen_ = App::Screen::TeamOverview;
}

void TeamOverviewScreen::render(App* app) {
    app->drawStaticBackground(app->teamOverviewBg_);
    app->beginScreen("Team Overview", false);

    Team* t = app->teamOverviewTeam_;
    if (ImGui::Button("< Back")) {
        app->screen_ = app->teamOverviewReturn_;
        ImGui::End();
        return;
    }

    if (!t) {
        ImGui::TextDisabled("No team selected.");
        ImGui::End();
        return;
    }

    ImGui::SameLine();
    ImGui::SetWindowFontScale(1.4f);
    ImGui::TextColored(ImVec4(0.95f, 0.97f, 1.0f, 1), "%s", t->name.c_str());
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const ImVec4 gold(0.60f, 0.75f, 0.95f, 1);
    float fullW = ImGui::GetContentRegionAvail().x;
    float fullH = ImGui::GetContentRegionAvail().y - 60;

    // === TOP SECTION: Team info ===
    ImGui::BeginChild("team_info", ImVec2(fullW, 120), true);
    panelHeader("Team Information");

    ImGui::Columns(2, "team_stats", false);
    ImGui::SetColumnWidth(0, fullW * 0.5f);

    ImGui::Text("League: %s", t->league.c_str());
    ImGui::Text("Squad Size: %d players", static_cast<int>(t->squad.size()));
    ImGui::Text("Formation: %s", t->formation.empty() ? t->preferredFormation.c_str() : t->formation.c_str());

    ImGui::NextColumn();

    double totalOverall = 0.0;
    for (const auto& p : t->squad) totalOverall += playerOverall(p);
    double avgOverall = t->squad.empty() ? 0.0 : totalOverall / t->squad.size();

    ImGui::Text("Average Overall: %.1f", avgOverall);
    ImGui::Text("Home Kit: %s / %s", t->homeColor1.c_str(), t->homeColor2.c_str());
    ImGui::Text("Away Kit: %s / %s", t->awayColor1.c_str(), t->awayColor2.c_str());

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Spacing();

    // === TABS: Squad | Economy ===
    static int teamOverviewTab = 0;

    {
        const char* tabLabels[] = { "Squad", "Economy" };
        const int tabCount = 2;
        const float tabH = 34.0f;
        const float tabW = 160.0f;
        const float tabBarY = ImGui::GetCursorScreenPos().y;
        ImDrawList* dl = ImGui::GetWindowDrawList();

        for (int i = 0; i < tabCount; ++i) {
            ImVec2 tabMin(ImGui::GetCursorScreenPos().x + i * (tabW + 4), tabBarY);
            ImVec2 tabMax(tabMin.x + tabW, tabBarY + tabH);
            bool active = (teamOverviewTab == i);
            ImU32 bgCol = active ? IM_COL32(55, 80, 110, 255) : IM_COL32(35, 50, 70, 200);
            ImU32 fgCol = active ? IM_COL32(240, 245, 255, 255) : IM_COL32(160, 180, 210, 255);
            dl->AddRectFilled(tabMin, tabMax, bgCol, 4.0f);
            if (active) dl->AddRect(tabMin, tabMax, IM_COL32(90, 130, 180, 255), 4.0f, 0, 1.5f);
            ImVec2 ts = ImGui::CalcTextSize(tabLabels[i]);
            dl->AddText(ImVec2(tabMin.x + (tabW - ts.x) * 0.5f, tabMin.y + (tabH - ts.y) * 0.5f),
                        fgCol, tabLabels[i]);
            ImGui::SetCursorScreenPos(tabMin);
            ImGui::PushID(i);
            if (ImGui::InvisibleButton("##tab", ImVec2(tabW, tabH)))
                teamOverviewTab = i;
            ImGui::PopID();
        }
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x,
                                         tabBarY + tabH + 6));
    }

    float contentH = fullH - 130 - 34 - 6;  // remaining after team_info and tab bar

    // =========================================================================
    // TAB 0: SQUAD
    // =========================================================================
    if (teamOverviewTab == 0) {

    // Squad list section
    ImGui::BeginChild("squad_list", ImVec2(fullW, contentH), true);
    panelHeader("Squad");

    // Build sorted squad list
    std::vector<const Player*> sortedSquad;
    for (const auto& p : t->squad) sortedSquad.push_back(&p);

    auto getPositionScore = [](const Player* p) -> int {
        std::set<Role> roles;
        for (Position pos : p->playablePositions) roles.insert(RoleOf(pos));
        int baseScore = 0;
        switch (p->role) {
            case Role::GK: baseScore = 0; break;
            case Role::D:  baseScore = 1000; break;
            case Role::DM: baseScore = 2000; break;
            case Role::M:  baseScore = 3000; break;
            case Role::AM: baseScore = 4000; break;
            case Role::F:  baseScore = 5000; break;
        }
        int versatilityPenalty = 0;
        if (roles.count(Role::AM) && roles.count(Role::F)) {
            if (p->role == Role::AM) versatilityPenalty = 50;
            else if (p->role == Role::F) { baseScore = 4500; versatilityPenalty = 0; }
        } else {
            versatilityPenalty = (int)(roles.size() - 1) * 100;
        }
        Side side = SideOf(p->primaryPos);
        int sidePenalty = (side == Side::Right) ? 10 : (side == Side::Left) ? 20 : 0;
        int positionPenalty = (int)(p->playablePositions.size() > 3
                                    ? (p->playablePositions.size() - 3) * 5 : 0);
        return baseScore + versatilityPenalty + sidePenalty + positionPenalty;
    };

    std::sort(sortedSquad.begin(), sortedSquad.end(), [&getPositionScore](const Player* a, const Player* b) {
        int sa = getPositionScore(a), sb = getPositionScore(b);
        return sa != sb ? sa < sb : playerOverall(*a) > playerOverall(*b);
    });

    // Unified table: squad info + season stats + wage
    ImGuiTableFlags tfl = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                          ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
    float tableH = contentH - 46.0f;

    if (ImGui::BeginTable("squad_stats_table", 12, tfl, ImVec2(0, tableH))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        // Squad columns
        ImGui::TableSetupColumn("#",      ImGuiTableColumnFlags_WidthFixed,   32);
        ImGui::TableSetupColumn("Name",   ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Pos",    ImGuiTableColumnFlags_WidthFixed,   48);
        ImGui::TableSetupColumn("OVR",    ImGuiTableColumnFlags_WidthFixed,   40);
        ImGui::TableSetupColumn("Age",    ImGuiTableColumnFlags_WidthFixed,   36);
        // Season stats columns
        ImGui::TableSetupColumn("Apps",   ImGuiTableColumnFlags_WidthFixed,   40);
        ImGui::TableSetupColumn("Gls",    ImGuiTableColumnFlags_WidthFixed,   36);
        ImGui::TableSetupColumn("Ast",    ImGuiTableColumnFlags_WidthFixed,   36);
        ImGui::TableSetupColumn("CS",     ImGuiTableColumnFlags_WidthFixed,   32);
        ImGui::TableSetupColumn("YC",     ImGuiTableColumnFlags_WidthFixed,   32);
        ImGui::TableSetupColumn("RC",     ImGuiTableColumnFlags_WidthFixed,   32);
        ImGui::TableSetupColumn("Wage",   ImGuiTableColumnFlags_WidthFixed,   80);
        ImGui::TableHeadersRow();

        for (const Player* p : sortedSquad) {
            const auto& s = p->seasonStats;
            ImGui::TableNextRow();

            // # shirt
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", p->shirtNumber);

            // Name — clickable
            ImGui::TableSetColumnIndex(1);
            if (ImGui::Selectable(p->name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns))
                app->openPlayerDetail(p, App::Screen::TeamOverview);

            // Pos
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(cmPositionFormat(*p).c_str());
            if (ImGui::IsItemHovered()) positionTooltip(*p);

            // OVR
            ImGui::TableSetColumnIndex(3);
            {
                double ovr = playerOverall(*p);
                ImVec4 c = ovr >= 80 ? ImVec4(0.2f, 1.0f, 0.3f, 1)
                         : ovr >= 70 ? ImVec4(0.8f, 0.9f, 0.3f, 1)
                         : ovr >= 60 ? ImVec4(1.0f, 0.8f, 0.2f, 1)
                                     : ImVec4(1.0f, 0.5f, 0.2f, 1);
                ImGui::TextColored(c, "%.0f", ovr);
            }

            // Age
            ImGui::TableSetColumnIndex(4);
            if (p->age > 0) ImGui::Text("%d", p->age);
            else            ImGui::TextDisabled("-");

            // Apps
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%d", s.games);

            // Goals
            ImGui::TableSetColumnIndex(6);
            if (s.goals > 0)
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1), "%d", s.goals);
            else
                ImGui::TextDisabled("0");

            // Assists
            ImGui::TableSetColumnIndex(7);
            if (s.assists > 0)
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1), "%d", s.assists);
            else
                ImGui::TextDisabled("0");

            // Clean sheets (GK/D/DM only)
            ImGui::TableSetColumnIndex(8);
            if (p->role == Role::GK || p->role == Role::D || p->role == Role::DM) {
                if (s.cleanSheets > 0)
                    ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.4f, 1), "%d", s.cleanSheets);
                else
                    ImGui::TextDisabled("0");
            } else {
                ImGui::TextDisabled("-");
            }

            // Yellow cards
            ImGui::TableSetColumnIndex(9);
            if (s.yellowCards > 0)
                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1), "%d", s.yellowCards);
            else
                ImGui::TextDisabled("0");

            // Red cards
            ImGui::TableSetColumnIndex(10);
            if (s.redCards > 0)
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1), "%d", s.redCards);
            else
                ImGui::TextDisabled("0");

            // Wage
            ImGui::TableSetColumnIndex(11);
            {
                char buf[32];
                int w = p->wageDemand;
                if (w >= 1000) std::snprintf(buf, sizeof(buf), "\xC2\xA3%dK", w / 1000);
                else           std::snprintf(buf, sizeof(buf), "\xC2\xA3%d", w);
                ImGui::TextUnformatted(buf);
            }
        }
        ImGui::EndTable();
    }

    ImGui::EndChild();

    } // end Squad tab

    // =========================================================================
    // TAB 1: ECONOMY
    // =========================================================================
    else if (teamOverviewTab == 1) {
        ImGui::BeginChild("economy_tab", ImVec2(fullW, contentH), true);
        panelHeader("Club Economy");
        ImGui::Spacing();

        // Helper: format currency value as £xM / £xK / £x
        auto fmtMoney = [](int v, char* buf, size_t sz) {
            if (v >= 1000000)
                std::snprintf(buf, sz, "\xC2\xA3%.2fM", static_cast<double>(v) / 1000000.0);
            else if (v >= 1000)
                std::snprintf(buf, sz, "\xC2\xA3%.0fK", static_cast<double>(v) / 1000.0);
            else
                std::snprintf(buf, sz, "\xC2\xA3%d", v);
        };

        // ---- Budgets --------------------------------------------------------
        ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.6f, 1), "Budgets");
        ImGui::Separator();
        ImGui::Spacing();

        bool isPlayerTeam = app->careerActive_ && (t->id == app->careerTeam_);

        // Transfer budget
        {
            char buf[48];
            int tbv = isPlayerTeam ? app->careerTransferBudget_ : t->transferBudget;
            fmtMoney(tbv, buf, sizeof(buf));
            ImGui::Text("  Transfer Budget:"); ImGui::SameLine(220);
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1), "%s", buf);
        }

        // Wage budget
        {
            char buf[48];
            fmtMoney(t->wageBudget, buf, sizeof(buf));
            ImGui::Text("  Wage Budget (season):"); ImGui::SameLine(220);
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1), "%s", buf);
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // ---- Weekly wage bill -----------------------------------------------
        ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.6f, 1), "Wages");
        ImGui::Separator();
        ImGui::Spacing();

        int totalWeekly = 0;
        for (const auto& p : t->squad) totalWeekly += p.wageDemand;

        {
            char buf[48];
            fmtMoney(totalWeekly, buf, sizeof(buf));
            ImGui::Text("  Weekly Wage Bill:"); ImGui::SameLine(220);
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.4f, 1), "%s", buf);
        }
        {
            char buf[48];
            fmtMoney(totalWeekly * 52, buf, sizeof(buf));
            ImGui::Text("  Annual Wage Bill:"); ImGui::SameLine(220);
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.4f, 1), "%s", buf);
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // ---- Season income / expenses (only for player's team in career) ----
        if (isPlayerTeam) {
            ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.6f, 1), "Season Finances");
            ImGui::Separator();
            ImGui::Spacing();

            {
                char buf[48];
                fmtMoney(app->careerIncome_, buf, sizeof(buf));
                ImGui::Text("  Income:"); ImGui::SameLine(220);
                ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1), "%s", buf);
            }
            {
                char buf[48];
                fmtMoney(app->careerExpenses_, buf, sizeof(buf));
                ImGui::Text("  Expenses (wages):"); ImGui::SameLine(220);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1), "%s", buf);
            }
            {
                char buf[48];
                int net = app->careerIncome_ - app->careerExpenses_;
                fmtMoney(std::abs(net), buf, sizeof(buf));
                ImGui::Text("  Net:"); ImGui::SameLine(220);
                ImGui::TextColored(net >= 0 ? ImVec4(0.4f, 0.9f, 0.5f, 1)
                                            : ImVec4(1.0f, 0.4f, 0.4f, 1),
                                   "%s%s", net < 0 ? "-" : "+", buf);
            }

            ImGui::Spacing();
        }

        // ---- Highest earners ------------------------------------------------
        ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.6f, 1), "Top Earners");
        ImGui::Separator();
        ImGui::Spacing();

        std::vector<const Player*> byWage;
        for (const auto& p : t->squad) byWage.push_back(&p);
        std::sort(byWage.begin(), byWage.end(),
                  [](const Player* a, const Player* b){ return a->wageDemand > b->wageDemand; });

        ImGuiTableFlags tfl = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_SizingFixedFit;
        if (ImGui::BeginTable("top_earners", 3, tfl)) {
            ImGui::TableSetupColumn("Name",        ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Pos",         ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableSetupColumn("Weekly Wage", ImGuiTableColumnFlags_WidthFixed, 110);
            ImGui::TableHeadersRow();
            int shown = 0;
            for (const Player* p : byWage) {
                if (shown++ >= 10) break;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(p->name.c_str());
                ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(PosName(p->primaryPos).c_str());
                ImGui::TableSetColumnIndex(2);
                char buf[32]; fmtMoney(p->wageDemand, buf, sizeof(buf));
                ImGui::TextUnformatted(buf);
            }
            ImGui::EndTable();
        }

        ImGui::EndChild();
    } // end Economy tab

    // Action buttons at bottom
    ImGui::Spacing();

    if (tintButton("Edit Tactics", IM_COL32(86, 150, 38, 255), ImVec2(180, 46))) {
        app->openTactics(t, app->teamOverviewReturn_);
    }

    ImGui::SameLine();

    if (tintButton("Start Training", IM_COL32(60, 120, 180, 255), ImVec2(180, 46))) {
        // TODO: Implement training screen
        app->status_ = "Training screen not yet implemented";
    }

    ImGui::SameLine();

    if (tintButton("Transfers", IM_COL32(180, 120, 40, 255), ImVec2(180, 46))) {
        // TODO: Implement transfers screen
        app->status_ = "Transfers screen not yet implemented";
    }

    ImGui::End();
}

}  // namespace nm
