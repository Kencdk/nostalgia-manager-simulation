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
    app->drawStaticBackground(app->playerDetailBg_);
    app->beginScreen("Player Detail", false);

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
    ImGui::TextColored(ImVec4(0.95f, 0.97f, 1.0f, 1), "%s", p->name.c_str());
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

    const ImVec4 gold(0.60f, 0.75f, 0.95f, 1);
    float fullW = ImGui::GetContentRegionAvail().x;
    float fullH = ImGui::GetContentRegionAvail().y - 60;

    // === TOP SECTION: Player Bio Information ===
    ImGui::BeginChild("player_bio", ImVec2(fullW, fullH * 0.18f), true);
    panelHeader("Player Information");
    ImGui::Spacing();

    // Left column: Basic info
    ImGui::BeginGroup();
    ImGui::Text("Position: %s", cmPositionFormat(*p).c_str());
    ImGui::Text("Overall Rating: %.1f", playerOverall(*p));
    if (p->shirtNumber > 0) {
        ImGui::Text("Shirt Number: %d", p->shirtNumber);
    }
    ImGui::EndGroup();

    // Right column: Bio info
    ImGui::SameLine(fullW * 0.35f);
    ImGui::BeginGroup();
    if (p->age > 0) {
        ImGui::Text("Age: %d", p->age);
    }
    if (!p->dateOfBirth.empty()) {
        ImGui::Text("Birthday: %s", p->dateOfBirth.c_str());
    }
    ImGui::EndGroup();

    // Far right column: International
    ImGui::SameLine(fullW * 0.65f);
    ImGui::BeginGroup();
    if (!p->nationality.empty()) {
        ImGui::Text("Nationality: %s", p->nationality.c_str());
    }
    if (p->internationalCaps > 0) {
        ImGui::Text("Caps: %d", p->internationalCaps);
        if (p->internationalGoals > 0) {
            ImGui::SameLine();
            ImGui::Text("| Goals: %d", p->internationalGoals);
        }
    }
    ImGui::EndGroup();

    ImGui::EndChild();

    ImGui::Spacing();

    // === BOTTOM SECTION: Attributes (left half) and Position/Pitch (right half) ===
    float halfW = fullW * 0.49f;
    float bottomH = fullH * 0.78f;

    // Left half: Categorized attributes (compact)
    float attrSectionH = bottomH * 0.55f;  // Use only 55% of bottom height for attributes
    ImGui::BeginChild("player_attributes", ImVec2(halfW, attrSectionH), true, ImGuiWindowFlags_NoScrollbar);
    panelHeader("Attributes");

    // Helper lambda for attribute rows
    auto attrRow = [](const char* name, int value) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s", name);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", value);
    };

    // Display attributes in 4 columns side by side (2 rows of 2)
    float categoryWidth = halfW * 0.48f;
    float categoryRowH = (attrSectionH - 35) * 0.485f;  // Adjusted for tighter fit

    // === Top row: Technical and Physical ===
    // Technical
    ImGui::BeginChild("technical", ImVec2(categoryWidth, categoryRowH), true, ImGuiWindowFlags_NoScrollbar);
    panelHeader("Technical", IM_COL32(70, 85, 110, 255));

    if (ImGui::BeginTable("tech_attrs", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Attr", ImGuiTableColumnFlags_WidthFixed, categoryWidth * 0.55f);
        ImGui::TableSetupColumn("Val", ImGuiTableColumnFlags_WidthStretch);

        attrRow("Passing", p->attr.get("Passing"));
        attrRow("Shooting", p->attr.get("Shooting"));
        attrRow("Technique", p->attr.get("Technique"));
        attrRow("Dribbling", p->attr.get("Dribbling"));
        attrRow("Heading", p->attr.get("Heading"));
        if (p->role == Role::GK) {
            attrRow("GK", p->attr.get("Goalkeeping"));
        }

        ImGui::EndTable();
    }
    ImGui::EndChild();

    // Physical
    ImGui::SameLine();
    ImGui::BeginChild("physical", ImVec2(categoryWidth, categoryRowH), true, ImGuiWindowFlags_NoScrollbar);
    panelHeader("Physical", IM_COL32(70, 85, 110, 255));

    if (ImGui::BeginTable("phys_attrs", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Attr", ImGuiTableColumnFlags_WidthFixed, categoryWidth * 0.55f);
        ImGui::TableSetupColumn("Val", ImGuiTableColumnFlags_WidthStretch);

        attrRow("Pace", p->attr.get("Pace"));
        attrRow("Stamina", p->attr.get("Stamina"));
        attrRow("Strength", p->attr.get("Strength"));
        attrRow("Jumping", p->attr.get("Jumping"));

        ImGui::EndTable();
    }
    ImGui::EndChild();

    // === Bottom row: Tactical and Mental ===
    // Tactical
    ImGui::BeginChild("tactical", ImVec2(categoryWidth, categoryRowH), true, ImGuiWindowFlags_NoScrollbar);
    panelHeader("Tactical", IM_COL32(70, 85, 110, 255));

    if (ImGui::BeginTable("tact_attrs", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Attr", ImGuiTableColumnFlags_WidthFixed, categoryWidth * 0.55f);
        ImGui::TableSetupColumn("Val", ImGuiTableColumnFlags_WidthStretch);

        attrRow("Positioning", p->attr.get("Positioning"));
        attrRow("Off Ball", p->attr.get("OffTheBall"));
        attrRow("Marking", p->attr.get("Marking"));
        attrRow("Tackling", p->attr.get("Tackling"));

        ImGui::EndTable();
    }
    ImGui::EndChild();

    // Mental
    ImGui::SameLine();
    ImGui::BeginChild("mental", ImVec2(categoryWidth, categoryRowH), true, ImGuiWindowFlags_NoScrollbar);
    panelHeader("Mental", IM_COL32(70, 85, 110, 255));

    if (ImGui::BeginTable("ment_attrs", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Attr", ImGuiTableColumnFlags_WidthFixed, categoryWidth * 0.55f);
        ImGui::TableSetupColumn("Val", ImGuiTableColumnFlags_WidthStretch);

        attrRow("Creativity", p->attr.get("Creativity"));
        attrRow("Determination", p->attr.get("Determination"));
        attrRow("Influence", p->attr.get("Influence"));
        attrRow("Aggression", p->attr.get("Aggression"));
        attrRow("Flair", p->attr.get("Flair"));

        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::EndChild();  // player_attributes

    // Space below attributes available for future content
    // You can add more sections here using the remaining space

    // Right half: Position visualization and pitch
    ImGui::SameLine();
    ImGui::BeginChild("position_viz", ImVec2(halfW, bottomH), true, ImGuiWindowFlags_NoScrollbar);
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

    // Pitch visualization (horizontal: FC on right, GK on left)
    float availW = ImGui::GetContentRegionAvail().x;
    float availH = ImGui::GetContentRegionAvail().y;

    // Smaller pitch dimensions to avoid scrolling
    const float pitchW = availW * 0.75f;
    const float pitchH = pitchW * 0.58f;  // Reduced aspect ratio

    // Center the pitch
    float offsetX = (availW - pitchW) * 0.5f;
    float offsetY = (availH - pitchH) * 0.3f;  // Positioned higher

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);

    ImVec2 pitchPos = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Draw pitch background
    draw->AddRectFilled(pitchPos, ImVec2(pitchPos.x + pitchW, pitchPos.y + pitchH), 
                        IM_COL32(34, 139, 34, 255));
    draw->AddRect(pitchPos, ImVec2(pitchPos.x + pitchW, pitchPos.y + pitchH), 
                  IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);

    // Draw center line (vertical down the middle)
    draw->AddLine(ImVec2(pitchPos.x + pitchW * 0.5f, pitchPos.y),
                  ImVec2(pitchPos.x + pitchW * 0.5f, pitchPos.y + pitchH),
                  IM_COL32(255, 255, 255, 255), 2.0f);

    // Draw center circle
    float centerX = pitchPos.x + pitchW * 0.5f;
    float centerY = pitchPos.y + pitchH * 0.5f;
    draw->AddCircle(ImVec2(centerX, centerY), pitchH * 0.2f, 
                    IM_COL32(255, 255, 255, 255), 32, 2.0f);

    // Draw left penalty box (GK side)
    float penaltyW = pitchW * 0.18f;
    float penaltyH = pitchH * 0.6f;
    float penaltyOffsetY = (pitchH - penaltyH) * 0.5f;
    draw->AddRect(ImVec2(pitchPos.x, pitchPos.y + penaltyOffsetY),
                  ImVec2(pitchPos.x + penaltyW, pitchPos.y + penaltyOffsetY + penaltyH),
                  IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);

    // Draw right penalty box (FC side)
    draw->AddRect(ImVec2(pitchPos.x + pitchW - penaltyW, pitchPos.y + penaltyOffsetY),
                  ImVec2(pitchPos.x + pitchW, pitchPos.y + penaltyOffsetY + penaltyH),
                  IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);

    // Draw player position indicator based on role (horizontal: GK left, FC right)
    float playerX = centerX;
    float playerY = centerY;

    switch (p->role) {
        case Role::GK:
            playerX = pitchPos.x + pitchW * 0.08f;
            break;
        case Role::D:
            playerX = pitchPos.x + pitchW * 0.25f;
            break;
        case Role::DM:
            playerX = pitchPos.x + pitchW * 0.4f;
            break;
        case Role::M:
            playerX = pitchPos.x + pitchW * 0.5f;
            break;
        case Role::AM:
            playerX = pitchPos.x + pitchW * 0.65f;
            break;
        case Role::F:
            playerX = pitchPos.x + pitchW * 0.92f;
            break;
    }

    // Draw player circle
    draw->AddCircleFilled(ImVec2(playerX, playerY), 10.0f, IM_COL32(120, 160, 200, 255));
    draw->AddCircle(ImVec2(playerX, playerY), 10.0f, IM_COL32(255, 255, 255, 255), 0, 2.0f);

    ImGui::Dummy(ImVec2(pitchW, pitchH));

    ImGui::EndChild();

    ImGui::End();
}

}  // namespace nm
