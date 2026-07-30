#include "PlayerDetail.h"
#include "PlayerDetail.h"
#include "UIHelpers.h"
#include <algorithm>

namespace nm {

void PlayerDetailScreen::openPlayerDetail(App* app, const Player* player, App::Screen returnTo) {
    app->detailPlayer_ = player;
    app->detailReturn_ = returnTo;
    app->screen_ = App::Screen::PlayerDetail;

    // Find the team this player belongs to
    app->detailTeamId_ = -1;
    for (const auto& team : app->db_.teams) {
        for (const auto& p : team.squad) {
            if (&p == player) {
                app->detailTeamId_ = team.id;
                break;
            }
        }
        if (app->detailTeamId_ != -1) break;
    }
}

void PlayerDetailScreen::render(App* app) {
    app->drawCyclingBackground();
    app->beginScreen("Player Detail");

    const Player* p = app->detailPlayer_;
    if (ImGui::Button("< Back")) {
        app->screen_ = app->detailReturn_;
        ImGui::End();
        return;
    }

    if (!p) {
        ImGui::TextDisabled("No player selected.");
        ImGui::End();
        return;
    }

    // Player name and team
    ImGui::SameLine();
    ImGui::SetWindowFontScale(1.4f);
    ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.45f, 1), "%s", p->name.c_str());
    ImGui::SetWindowFontScale(1.0f);

    // Find and display team name
    if (app->detailTeamId_ != -1) {
        Team* team = app->teamById(app->detailTeamId_);
        if (team) {
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", team->name.c_str());
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const ImVec4 gold(0.86f, 0.78f, 0.55f, 1);
    float fullW = ImGui::GetContentRegionAvail().x;
    float fullH = ImGui::GetContentRegionAvail().y - 60;

    // Player info section
    ImGui::BeginChild("player_info", ImVec2(fullW * 0.5f, fullH), true);
    panelHeader("Player Information");

    ImGui::Spacing();
    ImGui::Text("Position: %s", playablePosStr(*p).c_str());
    ImGui::Text("Overall: %.1f", playerOverall(*p));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    panelHeader("Attributes");
    ImGui::Spacing();

    // Display attributes in a table
    if (ImGui::BeginTable("attributes", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Attribute", ImGuiTableColumnFlags_WidthFixed, fullW * 0.2f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        auto attrRow = [](const char* name, int value) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", name);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", value);
        };

        attrRow("Pace", p->attr.get("Pace"));
        attrRow("Shooting", p->attr.get("Shooting"));
        attrRow("Passing", p->attr.get("Passing"));
        attrRow("Dribbling", p->attr.get("Dribbling"));
        attrRow("Heading", p->attr.get("Heading"));
        attrRow("Stamina", p->attr.get("Stamina"));
        attrRow("Strength", p->attr.get("Strength"));
        attrRow("Jumping", p->attr.get("Jumping"));
        attrRow("Positioning", p->attr.get("Positioning"));
        attrRow("Off The Ball", p->attr.get("OffTheBall"));
        attrRow("Marking", p->attr.get("Marking"));
        attrRow("Tackling", p->attr.get("Tackling"));
        attrRow("Technique", p->attr.get("Technique"));
        attrRow("Creativity", p->attr.get("Creativity"));
        attrRow("Determination", p->attr.get("Determination"));
        attrRow("Influence", p->attr.get("Influence"));
        attrRow("Aggression", p->attr.get("Aggression"));
        attrRow("Flair", p->attr.get("Flair"));
        attrRow("Goalkeeping", p->attr.get("Goalkeeping"));

        ImGui::EndTable();
    }

    ImGui::EndChild();

    // Position visualization (right side)
    ImGui::SameLine();
    ImGui::BeginChild("position_viz", ImVec2(fullW * 0.48f, fullH), true);
    panelHeader("Position & Role");
    ImGui::Spacing();

    ImGui::Text("Primary Role: %s", [p]() {
        switch (p->role) {
            case Role::GK: return "Goalkeeper";
            case Role::D: return "Defender";
            case Role::DM: return "Defensive Midfielder";
            case Role::M: return "Midfielder";
            case Role::AM: return "Attacking Midfielder";
            case Role::F: return "Forward";
            default: return "Unknown";
        }
    }());

    ImGui::Spacing();
    ImGui::Text("Playable Positions:");
    ImGui::TextWrapped("%s", playablePosStr(*p).c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Simple pitch visualization showing player's position preference
    const float pitchW = fullW * 0.45f;
    const float pitchH = pitchW * 0.68f;
    ImVec2 pitchPos = ImGui::GetCursorScreenPos();

    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Draw pitch background
    draw->AddRectFilled(pitchPos, ImVec2(pitchPos.x + pitchW, pitchPos.y + pitchH), 
                        IM_COL32(34, 139, 34, 255));
    draw->AddRect(pitchPos, ImVec2(pitchPos.x + pitchW, pitchPos.y + pitchH), 
                  IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);

    // Draw center line
    draw->AddLine(ImVec2(pitchPos.x + pitchW * 0.5f, pitchPos.y),
                  ImVec2(pitchPos.x + pitchW * 0.5f, pitchPos.y + pitchH),
                  IM_COL32(255, 255, 255, 255), 2.0f);

    // Draw player position indicator based on role
    float playerX = pitchPos.x + pitchW * 0.5f;
    float playerY = pitchPos.y + pitchH * 0.5f;

    switch (p->role) {
        case Role::GK:
            playerY = pitchPos.y + pitchH * 0.9f;
            break;
        case Role::D:
            playerY = pitchPos.y + pitchH * 0.75f;
            break;
        case Role::DM:
            playerY = pitchPos.y + pitchH * 0.6f;
            break;
        case Role::M:
            playerY = pitchPos.y + pitchH * 0.5f;
            break;
        case Role::AM:
            playerY = pitchPos.y + pitchH * 0.35f;
            break;
        case Role::F:
            playerY = pitchPos.y + pitchH * 0.2f;
            break;
    }

    // Draw player circle
    draw->AddCircleFilled(ImVec2(playerX, playerY), 12.0f, IM_COL32(255, 215, 0, 255));
    draw->AddCircle(ImVec2(playerX, playerY), 12.0f, IM_COL32(0, 0, 0, 255), 0, 2.0f);

    ImGui::Dummy(ImVec2(pitchW, pitchH));

    ImGui::EndChild();

    ImGui::End();
}

}  // namespace nm
