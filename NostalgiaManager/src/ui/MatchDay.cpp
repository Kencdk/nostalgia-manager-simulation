#include "MatchDay.h"
#include "UIHelpers.h"
#include <algorithm>

namespace nm {

void MatchDayScreen::render(App* app) {
    // Draw static background (use career mode background)
    app->drawStaticBackground(app->careerModeBaseBg_);

    app->beginScreen("Match Day", false);

    Team* myteam = app->teamById(app->careerTeam_);
    int totalRounds = static_cast<int>(app->roundStart_.size()) - 1;

    // Get viewport dimensions for positioning
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float availWidth = ImGui::GetContentRegionAvail().x;
    float availHeight = ImGui::GetContentRegionAvail().y;
    float centerX = availWidth * 0.5f;

    // === TOP HEADER: Date and Round (Centered) ===
    ImGui::SetWindowFontScale(1.4f);
    const char* roundText = "Round %d / %d";
    char roundBuffer[64];
    std::snprintf(roundBuffer, sizeof(roundBuffer), roundText, app->careerRound_ + 1, totalRounds);
    float roundTextWidth = ImGui::CalcTextSize(roundBuffer).x;
    ImGui::SetCursorPosX(centerX - roundTextWidth * 0.5f);
    ImGui::TextColored(ImVec4(0.95f, 0.97f, 1.0f, 1), "%s", roundBuffer);
    ImGui::SetWindowFontScale(1.0f);

    // Display the date (simplified format, centered)
    const char* months[] = {"", "January", "February", "March", "April", "May", "June",
                            "July", "August", "September", "October", "November", "December"};
    if (app->currentMonth_ >= 1 && app->currentMonth_ <= 12) {
        char dateBuffer[128];
        std::snprintf(dateBuffer, sizeof(dateBuffer), "Date: %d %s %d", 
                     app->currentDay_, months[app->currentMonth_], app->currentYear_);
        float dateTextWidth = ImGui::CalcTextSize(dateBuffer).x;
        ImGui::SetCursorPosX(centerX - dateTextWidth * 0.5f);
        ImGui::Text("%s", dateBuffer);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    // === FIXTURES LIST (Centered) ===
    ImGui::SetWindowFontScale(1.2f);
    const char* fixturesTitle = "Fixtures";
    float fixturesTitleWidth = ImGui::CalcTextSize(fixturesTitle).x;
    ImGui::SetCursorPosX(centerX - fixturesTitleWidth * 0.5f);
    ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.6f, 1), "%s", fixturesTitle);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing();

    // Get fixtures for current round
    if (app->careerRound_ < totalRounds) {
        size_t from = app->roundStart_[app->careerRound_];
        size_t to = app->roundStart_[app->careerRound_ + 1];

        // Calculate table dimensions and center it
        float tableWidth = 640.0f;  // 300 + 40 + 300
        float tableStartX = centerX - tableWidth * 0.5f;

        ImGui::SetCursorPosX(tableStartX);

        // Create a table for fixtures
        ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

        if (ImGui::BeginTable("fixtures_table", 3, tableFlags, ImVec2(tableWidth, 0))) {
            ImGui::TableSetupColumn("Home", ImGuiTableColumnFlags_WidthFixed, 300);
            ImGui::TableSetupColumn("vs", ImGuiTableColumnFlags_WidthFixed, 40);
            ImGui::TableSetupColumn("Away", ImGuiTableColumnFlags_WidthFixed, 300);
            ImGui::TableHeadersRow();

            for (size_t i = from; i < to; ++i) {
                Team* home = app->teamById(app->fixtures_[i].first);
                Team* away = app->teamById(app->fixtures_[i].second);

                if (!home || !away) continue;

                ImGui::TableNextRow();

                // Check if this is the player's team match
                bool isPlayerMatch = (home->id == app->careerTeam_ || away->id == app->careerTeam_);

                if (isPlayerMatch) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(120, 100, 20, 200));
                }

                // Home team
                ImGui::TableSetColumnIndex(0);
                if (home->id == app->careerTeam_) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.3f, 1));
                    ImGui::TextUnformatted(home->name.c_str());
                    ImGui::PopStyleColor();
                } else {
                    ImGui::TextUnformatted(home->name.c_str());
                }

                // vs
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted("vs");

                // Away team
                ImGui::TableSetColumnIndex(2);
                if (away->id == app->careerTeam_) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.3f, 1));
                    ImGui::TextUnformatted(away->name.c_str());
                    ImGui::PopStyleColor();
                } else {
                    ImGui::TextUnformatted(away->name.c_str());
                }
            }

            ImGui::EndTable();
        }
    } else {
        const char* noFixturesText = "No more fixtures this season.";
        float noFixturesWidth = ImGui::CalcTextSize(noFixturesText).x;
        ImGui::SetCursorPosX(centerX - noFixturesWidth * 0.5f);
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1), "%s", noFixturesText);
    }

    // === BUTTONS AT 3/4 DOWN THE SCREEN (Centered) ===
    float buttonsY = availHeight * 0.75f;
    ImGui::SetCursorPosY(buttonsY);

    // Calculate button positioning for centering
    const ImVec2 continueSize(200, 50);
    const ImVec2 backSize(150, 50);
    float spacing = 20.0f;
    float totalButtonWidth = continueSize.x + spacing + backSize.x;
    float buttonStartX = centerX - totalButtonWidth * 0.5f;

    ImGui::SetCursorPosX(buttonStartX);

    ImGui::SetWindowFontScale(1.2f);
    if (ImGui::Button("Continue", continueSize)) {
        // Start the career match flow (will go to tactics, then player match, then simulate rest)
        app->careerAdvanceToPlayerMatch();
    }
    ImGui::SetWindowFontScale(1.0f);

    ImGui::SameLine(0, spacing);

    if (ImGui::Button("< Back", backSize)) {
        app->screen_ = App::Screen::CareerModeBase;
    }

    ImGui::End();
}

}  // namespace nm
