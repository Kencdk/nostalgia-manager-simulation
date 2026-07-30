#include "TeamOverview.h"
#include "UIHelpers.h"
#include <algorithm>
#include <map>

namespace nm {

void TeamOverviewScreen::openTeamOverview(App* app, Team* team, App::Screen returnTo) {
    app->teamOverviewTeam_ = team;
    app->teamOverviewReturn_ = returnTo;
    app->screen_ = App::Screen::TeamOverview;
}

void TeamOverviewScreen::render(App* app) {
    app->drawCyclingBackground();
    app->beginScreen("Team Overview");

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
    ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.45f, 1), "%s", t->name.c_str());
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const ImVec4 gold(0.86f, 0.78f, 0.55f, 1);
    float fullW = ImGui::GetContentRegionAvail().x;
    float fullH = ImGui::GetContentRegionAvail().y - 60;

    // Team info section
    ImGui::BeginChild("team_info", ImVec2(fullW, 120), true);
    panelHeader("Team Information");

    ImGui::Columns(2, "team_stats", false);
    ImGui::SetColumnWidth(0, fullW * 0.5f);

    ImGui::Text("League: %s", t->league.c_str());
    ImGui::Text("Squad Size: %d players", static_cast<int>(t->squad.size()));
    ImGui::Text("Formation: %s", t->formation.empty() ? t->preferredFormation.c_str() : t->formation.c_str());

    ImGui::NextColumn();

    // Calculate average overall
    double totalOverall = 0.0;
    for (const auto& p : t->squad) {
        totalOverall += playerOverall(p);
    }
    double avgOverall = t->squad.empty() ? 0.0 : totalOverall / t->squad.size();

    ImGui::Text("Average Overall: %.1f", avgOverall);
    ImGui::Text("Home Kit: %s / %s", t->homeColor1.c_str(), t->homeColor2.c_str());
    ImGui::Text("Away Kit: %s / %s", t->awayColor1.c_str(), t->awayColor2.c_str());

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Spacing();

    // Squad list section
    ImGui::BeginChild("squad_list", ImVec2(fullW, fullH - 130), true);
    panelHeader("Squad");

    // Organize players by position group
    // Combine DM, M, and AM into a single Midfielders group
    std::map<std::string, std::vector<const Player*>> playersByGroup;

    for (const auto& p : t->squad) {
        std::string group;
        switch (p.role) {
            case Role::GK:
                group = "Goalkeepers";
                break;
            case Role::D:
                group = "Defenders";
                break;
            case Role::DM:
            case Role::M:
            case Role::AM:
                group = "Midfielders";
                break;
            case Role::F:
                group = "Forwards";
                break;
        }
        playersByGroup[group].push_back(&p);
    }

    // Sort each group by overall rating (descending)
    for (auto& [group, players] : playersByGroup) {
        std::sort(players.begin(), players.end(), [](const Player* a, const Player* b) {
            return playerOverall(*a) > playerOverall(*b);
        });
    }

    // Define display order
    const std::vector<std::string> groupOrder = {"Goalkeepers", "Defenders", "Midfielders", "Forwards"};

    // Table header
    ImGui::PushStyleColor(ImGuiCol_Text, gold);
    ImGui::Columns(6, "squad_header", true);
    ImGui::SetColumnWidth(0, 60);   // Shirt #
    ImGui::SetColumnWidth(1, 70);   // Pos
    ImGui::SetColumnWidth(2, fullW * 0.35f);  // Name
    ImGui::SetColumnWidth(3, 70);   // OVR
    ImGui::SetColumnWidth(4, 70);   // Age
    ImGui::SetColumnWidth(5, fullW * 0.2f);   // Actions

    ImGui::Text("#");
    ImGui::NextColumn();
    ImGui::Text("Pos");
    ImGui::NextColumn();
    ImGui::Text("Name");
    ImGui::NextColumn();
    ImGui::Text("OVR");
    ImGui::NextColumn();
    ImGui::Text("Age");
    ImGui::NextColumn();
    ImGui::Text("");
    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::Columns(1);
    ImGui::PopStyleColor();

    // Display players grouped by position group
    for (const auto& groupName : groupOrder) {
        if (playersByGroup.find(groupName) == playersByGroup.end() || 
            playersByGroup[groupName].empty()) continue;

        const auto& players = playersByGroup[groupName];

        // Group header
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.85f, 0.6f, 1));
        ImGui::Text("%s (%d)", groupName.c_str(), static_cast<int>(players.size()));
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // Players in this group
        for (const Player* p : players) {
            ImGui::Columns(6, "squad_row", true);
            ImGui::SetColumnWidth(0, 60);
            ImGui::SetColumnWidth(1, 70);
            ImGui::SetColumnWidth(2, fullW * 0.35f);
            ImGui::SetColumnWidth(3, 70);
            ImGui::SetColumnWidth(4, 70);
            ImGui::SetColumnWidth(5, fullW * 0.2f);

            // Shirt number
            ImGui::Text("%2d", p->shirtNumber);
            ImGui::NextColumn();

            // Position
            ImGui::Text("%s", PosName(p->primaryPos).c_str());
            if (ImGui::IsItemHovered()) {
                positionTooltip(*p);
            }
            ImGui::NextColumn();

            // Name - clickable
            if (ImGui::Selectable(p->name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                app->openPlayerDetail(p, App::Screen::TeamOverview);
            }
            ImGui::NextColumn();

            // Overall rating
            double ovr = playerOverall(*p);
            // Color code by rating
            ImVec4 ratingColor;
            if (ovr >= 80) ratingColor = ImVec4(0.2f, 1.0f, 0.3f, 1.0f);      // Green
            else if (ovr >= 70) ratingColor = ImVec4(0.8f, 0.9f, 0.3f, 1.0f);  // Yellow-green
            else if (ovr >= 60) ratingColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);  // Yellow
            else ratingColor = ImVec4(1.0f, 0.5f, 0.2f, 1.0f);                  // Orange

            ImGui::TextColored(ratingColor, "%.0f", ovr);
            ImGui::NextColumn();

            // Age (if available - not in current data model, so we show a placeholder)
            ImGui::TextDisabled("-");
            ImGui::NextColumn();

            // Actions - empty for now (could add buttons for transfer, etc.)
            ImGui::NextColumn();

            ImGui::Columns(1);
        }
    }

    ImGui::EndChild();

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
