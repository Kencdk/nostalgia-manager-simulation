#include "TeamOverview.h"
#include "TeamOverview.h"
#include "UIHelpers.h"
#include <algorithm>
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

    // Create a copy of squad for sorting
    std::vector<const Player*> sortedSquad;
    for (const auto& p : t->squad) {
        sortedSquad.push_back(&p);
    }

    // Helper to calculate position score for sorting
    // Lower score = more defensive/specialized
    auto getPositionScore = [](const Player* p) -> int {
        // Count roles the player can play
        std::set<Role> roles;
        for (Position pos : p->playablePositions) {
            roles.insert(RoleOf(pos));
        }

        // Base score by primary role
        int baseScore = 0;
        switch (p->role) {
            case Role::GK: baseScore = 0; break;
            case Role::D:  baseScore = 1000; break;
            case Role::DM: baseScore = 2000; break;
            case Role::M:  baseScore = 3000; break;
            case Role::AM: baseScore = 4000; break;
            case Role::F:  baseScore = 5000; break;
        }

        // Add penalty for versatility (can play multiple roles)
        // More versatile = higher score = later in list
        // BUT: Special case for AM/F - they should come before pure F
        int versatilityPenalty = 0;
        if (roles.count(Role::AM) && roles.count(Role::F)) {
            // Player can play both AM and F
            if (p->role == Role::AM) {
                // AM who can play F: slightly increase from base AM score
                versatilityPenalty = 50;
            } else if (p->role == Role::F) {
                // F who can play AM: reduce score to come before pure F
                baseScore = 4500;  // Between AM and F
                versatilityPenalty = 0;
            }
        } else {
            versatilityPenalty = (roles.size() - 1) * 100;
        }

        // Add penalty for wide positions (centre before wide)
        Side side = SideOf(p->primaryPos);
        int sidePenalty = 0;
        if (side == Side::Right) sidePenalty = 10;
        else if (side == Side::Left) sidePenalty = 20;

        // Count total positions (more positions = more versatile = later)
        int positionCount = p->playablePositions.size();
        int positionPenalty = (positionCount > 3) ? (positionCount - 3) * 5 : 0;

        return baseScore + versatilityPenalty + sidePenalty + positionPenalty;
    };

    // Sort players by position score, then by overall rating
    std::sort(sortedSquad.begin(), sortedSquad.end(), [&getPositionScore](const Player* a, const Player* b) {
        int scoreA = getPositionScore(a);
        int scoreB = getPositionScore(b);

        if (scoreA != scoreB) {
            return scoreA < scoreB;
        }

        // Same position score, sort by overall rating (descending)
        return playerOverall(*a) > playerOverall(*b);
    });

    // Table header
    ImGui::PushStyleColor(ImGuiCol_Text, gold);
    ImGui::Columns(6, "squad_header", true);
    ImGui::SetColumnWidth(0, 60);   // Shirt #
    ImGui::SetColumnWidth(1, fullW * 0.35f);  // Name
    ImGui::SetColumnWidth(2, 200);  // Pos
    ImGui::SetColumnWidth(3, 70);   // OVR
    ImGui::SetColumnWidth(4, 70);   // Age
    ImGui::SetColumnWidth(5, fullW * 0.2f);   // Actions

    ImGui::Text("#");
    ImGui::NextColumn();
    ImGui::Text("Name");
    ImGui::NextColumn();
    ImGui::Text("Pos");
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

    // Display all players in sorted order
    for (const Player* p : sortedSquad) {
        ImGui::Columns(6, "squad_row", true);
        ImGui::SetColumnWidth(0, 60);
        ImGui::SetColumnWidth(1, fullW * 0.35f);
        ImGui::SetColumnWidth(2, 200);
        ImGui::SetColumnWidth(3, 70);
        ImGui::SetColumnWidth(4, 70);
        ImGui::SetColumnWidth(5, fullW * 0.2f);

        // Shirt number
        ImGui::Text("%2d", p->shirtNumber);
        ImGui::NextColumn();

        // Name - clickable
        if (ImGui::Selectable(p->name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
            app->openPlayerDetail(p, App::Screen::TeamOverview);
        }
        ImGui::NextColumn();

        // Position
        ImGui::Text("%s", cmPositionFormat(*p).c_str());
        if (ImGui::IsItemHovered()) {
            positionTooltip(*p);
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
