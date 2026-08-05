#include "PlayerDetail.h"
#include "PlayerDetail.h"
#include "UIHelpers.h"
#include <algorithm>
#include <cstdio>

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
    ImGui::BeginChild("player_bio", ImVec2(fullW, fullH * 0.22f), true);
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

    // Financial column: Wage Demand and Transfer Value
    ImGui::SameLine(fullW * 0.82f);
    ImGui::BeginGroup();
    {
        char wageBuf[48];
        if (p->wageDemand >= 1000)
            std::snprintf(wageBuf, sizeof(wageBuf), "\xC2\xA3%dK p/w",
                          p->wageDemand / 1000);
        else
            std::snprintf(wageBuf, sizeof(wageBuf), "\xC2\xA3%d p/w", p->wageDemand);
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1), "Wage:  %s", wageBuf);
    }
    {
        char tvBuf[48];
        int tv = p->transferValue;
        if (tv >= 1000000)
            std::snprintf(tvBuf, sizeof(tvBuf), "\xC2\xA3%.2fM",
                          static_cast<double>(tv) / 1000000.0);
        else if (tv >= 1000)
            std::snprintf(tvBuf, sizeof(tvBuf), "\xC2\xA3%.0fK",
                          static_cast<double>(tv) / 1000.0);
        else
            std::snprintf(tvBuf, sizeof(tvBuf), "\xC2\xA3%d", tv);
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1), "Value: %s", tvBuf);
    }
    ImGui::EndGroup();

    ImGui::EndChild();

    ImGui::Spacing();

    // === BOTTOM SECTION: Left column (Attributes + Season Stats) and Right column (Pitch) ===
    float halfW = fullW * 0.49f;
    float bottomH = fullH * 0.78f;

    // Helper lambda for attribute rows
    auto attrRow = [](const char* name, int value) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s", name);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", value);
    };

    // Left column: Attributes + Season Stats stacked
    ImGui::BeginChild("left_column", ImVec2(halfW, bottomH), false, ImGuiWindowFlags_NoScrollbar);

    // --- Attributes panel ---
    float categoryWidth = halfW * 0.48f;

    // Technical attributes (GK adds one extra row)
    int techRows = (p->role == Role::GK) ? 6 : 5;
    float rowH = ImGui::GetTextLineHeightWithSpacing();
    float headerH = ImGui::GetTextLineHeightWithSpacing() + 6.0f;

    // Compute natural height for each category box: header + table rows + padding
    auto categoryH = [&](int rows) {
        return headerH + rows * rowH + 10.0f;
    };

    float techH  = categoryH(techRows);
    float physH  = categoryH(4) + rowH + 4.0f;  // +1 row for Injury Proneness
    float tactH  = categoryH(4);
    int mentRows = 5 + (p->personality > 0 ? 1 : 0);
    float mentH  = categoryH(mentRows) + (p->personality > 0 ? rowH + 4.0f : 0.0f);
    float row1H  = std::max(techH,  physH);
    float row2H  = std::max(tactH,  mentH);
    float attrPanelH = row1H + row2H + 12.0f;  // 12px gap between rows

    ImGui::BeginChild("player_attributes", ImVec2(halfW, attrPanelH), true, ImGuiWindowFlags_NoScrollbar);
    panelHeader("Attributes");

    // === Top row: Technical and Physical ===
    ImGui::BeginChild("technical", ImVec2(categoryWidth, row1H), true, ImGuiWindowFlags_NoScrollbar);
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
    ImGui::BeginChild("physical", ImVec2(categoryWidth, row1H), true, ImGuiWindowFlags_NoScrollbar);
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

    {
        ImGui::Spacing();
        const char* injuryLabel = "";
        if      (p->injuryProneness <= 0)  injuryLabel = "-";
        else if (p->injuryProneness <= 4)  injuryLabel = "Very Healthy";
        else if (p->injuryProneness <= 8)  injuryLabel = "Healthy";
        else if (p->injuryProneness <= 12) injuryLabel = "Average Risk";
        else if (p->injuryProneness <= 16) injuryLabel = "Above Average Risk";
        else                               injuryLabel = "Injury Prone";
        ImGui::TextDisabled("Injury Pron:");
        ImGui::SameLine();
        if (p->injuryProneness > 0)
            ImGui::Text("%d - %s", p->injuryProneness, injuryLabel);
        else
            ImGui::TextDisabled("%s", injuryLabel);
    }

    ImGui::EndChild();

    // === Bottom row: Tactical and Mental ===
    ImGui::BeginChild("tactical", ImVec2(categoryWidth, row2H), true, ImGuiWindowFlags_NoScrollbar);
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
    ImGui::BeginChild("mental", ImVec2(categoryWidth, row2H), true, ImGuiWindowFlags_NoScrollbar);
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

    if (p->personality > 0) {
        ImGui::Spacing();
        const char* personalityLabel = "";
        if      (p->personality <= 5)  personalityLabel = "Unprofessional";
        else if (p->personality <= 10) personalityLabel = "Below Average";
        else if (p->personality <= 15) personalityLabel = "Normal";
        else                           personalityLabel = "Excellent Pro";
        ImGui::TextDisabled("Character:");
        ImGui::SameLine();
        ImGui::Text("%d - %s", p->personality, personalityLabel);
        if (ImGui::IsItemHovered()) {
            const char* fullLabel = p->personality <= 5  ? "Unprofessional, difficult to develop"
                                  : p->personality <= 10 ? "Below average"
                                  : p->personality <= 15 ? "Normal"
                                                         : "Excellent professional";
            ImGui::SetTooltip("%s", fullLabel);
        }
    }

    ImGui::EndChild();  // mental

    ImGui::EndChild();  // player_attributes

    // --- Season Stats panel ---
    ImGui::Spacing();
    float statsH = bottomH - attrPanelH - ImGui::GetStyle().ItemSpacing.y * 2.0f;
    ImGui::BeginChild("season_stats", ImVec2(halfW, statsH), true, ImGuiWindowFlags_NoScrollbar);
    panelHeader("Season Stats");
    ImGui::Spacing();

    const auto& ss = p->seasonStats;
    if (ImGui::BeginTable("season_stats_tbl", 6,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                          ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("Avg Rat");
        ImGui::TableSetupColumn("Games");
        ImGui::TableSetupColumn("Goals");
        ImGui::TableSetupColumn("Assists");
        ImGui::TableSetupColumn("Yellow");
        ImGui::TableSetupColumn("Red");
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        if (ss.ratingGames > 0)
            ImGui::Text("%.2f", ss.avgRating());
        else
            ImGui::TextDisabled("-");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", ss.games);
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%d", ss.goals);
        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%d", ss.assists);
        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%d", ss.yellowCards);
        ImGui::TableSetColumnIndex(5);
        ImGui::Text("%d", ss.redCards);

        ImGui::EndTable();
    }

    ImGui::EndChild();  // season_stats

    ImGui::EndChild();  // left_column

    // Right half: Position visualization and pitch
    ImGui::SameLine();
    ImGui::BeginChild("position_viz", ImVec2(halfW, bottomH), true, ImGuiWindowFlags_NoScrollbar);
    panelHeader("Position & Role");
    ImGui::Spacing();

    // Pitch visualization: all positions with rating > 0, colour-graded
    float availW = ImGui::GetContentRegionAvail().x;
    float availH = ImGui::GetContentRegionAvail().y;

    const float pitchW = availW * 0.92f;
    const float pitchH = pitchW * 0.62f;

    float offsetX = (availW - pitchW) * 0.5f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

    ImVec2 pitchPos = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    // ---- Pitch background and markings ----
    draw->AddRectFilled(pitchPos, ImVec2(pitchPos.x + pitchW, pitchPos.y + pitchH),
                        IM_COL32(34, 139, 34, 255));
    // Stripes
    const int stripes = 6;
    for (int i = 0; i < stripes; ++i) {
        float x0 = pitchPos.x + pitchW * i / stripes;
        float x1 = pitchPos.x + pitchW * (i + 1) / stripes;
        if (i % 2 == 1)
            draw->AddRectFilled(ImVec2(x0, pitchPos.y), ImVec2(x1, pitchPos.y + pitchH),
                                IM_COL32(30, 128, 30, 255));
    }
    draw->AddRect(pitchPos, ImVec2(pitchPos.x + pitchW, pitchPos.y + pitchH),
                  IM_COL32(255, 255, 255, 200), 0.0f, 0, 1.5f);
    // Halfway line
    draw->AddLine(ImVec2(pitchPos.x + pitchW * 0.5f, pitchPos.y),
                  ImVec2(pitchPos.x + pitchW * 0.5f, pitchPos.y + pitchH),
                  IM_COL32(255, 255, 255, 120), 1.0f);
    // Penalty boxes
    float pbW = pitchW * 0.14f, pbH = pitchH * 0.55f, pbOY = (pitchH - pbH) * 0.5f;
    draw->AddRect(ImVec2(pitchPos.x, pitchPos.y + pbOY),
                  ImVec2(pitchPos.x + pbW, pitchPos.y + pbOY + pbH),
                  IM_COL32(255, 255, 255, 120), 0.0f, 0, 1.0f);
    draw->AddRect(ImVec2(pitchPos.x + pitchW - pbW, pitchPos.y + pbOY),
                  ImVec2(pitchPos.x + pitchW, pitchPos.y + pbOY + pbH),
                  IM_COL32(255, 255, 255, 120), 0.0f, 0, 1.0f);

    // ---- Position layout: X = role column (0-5), Y = side row (R/C/L) ----
    // X fractions across the pitch (GK left, FC right)
    struct PosLayout { Position pos; float xf; float yf; };
    const PosLayout layout[] = {
        { Position::GK,  0.07f, 0.50f },
        { Position::DR,  0.22f, 0.80f }, { Position::DC,  0.22f, 0.50f }, { Position::DL,  0.22f, 0.20f },
        { Position::WBR, 0.30f, 0.88f }, { Position::WBL, 0.30f, 0.12f },
        { Position::DM,  0.38f, 0.50f },
        { Position::MR,  0.50f, 0.80f }, { Position::MC,  0.50f, 0.50f }, { Position::ML,  0.50f, 0.20f },
        { Position::AMR, 0.62f, 0.80f }, { Position::AMC, 0.62f, 0.50f }, { Position::AML, 0.62f, 0.20f },
        { Position::FR,  0.78f, 0.80f }, { Position::FC,  0.78f, 0.50f }, { Position::FL,  0.78f, 0.20f },
    };

    // Find max rating for graduation thresholds
    int maxRating = 1;
    for (const auto& kv : p->positionRatings)
        if (kv.second > maxRating) maxRating = kv.second;

    const float dotR = 9.0f;
    const float fontSize = ImGui::GetFontSize() * 0.75f;

    for (const auto& pl : layout) {
        auto it = p->positionRatings.find(pl.pos);
        if (it == p->positionRatings.end() || it->second <= 0) continue;

        int rating = it->second;
        float frac = static_cast<float>(rating) / static_cast<float>(maxRating);

        // Grade thresholds (relative to player's own best)
        ImU32 dotCol, borderCol;
        const char* grade;
        if (frac >= 0.90f) {
            // Preferred – gold
            dotCol    = IM_COL32(255, 210, 30,  255);
            borderCol = IM_COL32(255, 255, 255, 255);
            grade     = "P";
        } else if (frac >= 0.70f) {
            // Natural – bright green
            dotCol    = IM_COL32(60,  220, 90,  255);
            borderCol = IM_COL32(200, 255, 200, 255);
            grade     = "N";
        } else if (frac >= 0.45f) {
            // Competent – blue
            dotCol    = IM_COL32(80,  150, 230, 255);
            borderCol = IM_COL32(180, 200, 255, 255);
            grade     = "C";
        } else {
            // Unsuitable – dark grey
            dotCol    = IM_COL32(100, 100, 100, 200);
            borderCol = IM_COL32(160, 160, 160, 200);
            grade     = "U";
        }

        float cx = pitchPos.x + pl.xf * pitchW;
        float cy = pitchPos.y + pl.yf * pitchH;

        draw->AddCircleFilled(ImVec2(cx, cy), dotR, dotCol);
        draw->AddCircle(ImVec2(cx, cy), dotR, borderCol, 0, 1.5f);

        // Position label above dot
        std::string posLabel = PosName(pl.pos);
        ImVec2 labelSz = ImGui::CalcTextSize(posLabel.c_str());
        draw->AddText(ImGui::GetFont(), fontSize,
                      ImVec2(cx - labelSz.x * 0.5f * (fontSize / ImGui::GetFontSize()),
                             cy - dotR - fontSize - 1),
                      IM_COL32(255, 255, 255, 230), posLabel.c_str());
        // Rating below dot
        char ratingStr[8];
        std::snprintf(ratingStr, sizeof(ratingStr), "%d", rating);
        ImVec2 rSz = ImGui::CalcTextSize(ratingStr);
        draw->AddText(ImGui::GetFont(), fontSize,
                      ImVec2(cx - rSz.x * 0.5f * (fontSize / ImGui::GetFontSize()),
                             cy + dotR + 1),
                      dotCol, ratingStr);
    }

    ImGui::Dummy(ImVec2(pitchW, pitchH));

    ImGui::Spacing();

    // ---- Legend ----
    const struct { ImU32 col; const char* label; } legend[] = {
        { IM_COL32(255, 210, 30,  255), "Preferred" },
        { IM_COL32(60,  220, 90,  255), "Natural"   },
        { IM_COL32(80,  150, 230, 255), "Competent" },
        { IM_COL32(100, 100, 100, 200), "Unsuitable"},
    };
    for (const auto& lg : legend) {
        ImVec2 cp = ImGui::GetCursorScreenPos();
        draw->AddCircleFilled(ImVec2(cp.x + 7, cp.y + 7), 6.0f, lg.col);
        ImGui::Dummy(ImVec2(16, 14));
        ImGui::SameLine(0, 4);
        ImGui::TextUnformatted(lg.label);
        ImGui::SameLine(0, 16);
    }
    ImGui::NewLine();

    ImGui::EndChild();

    ImGui::End();
}

}  // namespace nm
