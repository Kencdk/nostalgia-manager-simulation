#include "CareerFixtures.h"
#include "UIHelpers.h"
#include <algorithm>
#include <cstdio>

namespace nm {

static const char* kMonths[] = {
    "", "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

void CareerFixturesScreen::render(App* app) {
    app->drawStaticBackground(app->careerModeBaseBg_);
    app->beginScreen("Fixtures", false);

    int totalRounds = static_cast<int>(app->roundStart_.size()) - 1;

    // --- Back button ---
    if (ImGui::Button("< Back")) {
        app->screen_ = App::Screen::CareerModeBase;
        ImGui::End();
        return;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    float availWidth  = ImGui::GetContentRegionAvail().x;
    float availHeight = ImGui::GetContentRegionAvail().y;

    // Left panel: round list  |  Right panel: fixtures for selected round
    float leftW  = 160.0f;
    float rightW = availWidth - leftW - 12.0f;

    // ?? LEFT: round navigator ????????????????????????????????????????????
    ImGui::BeginChild("round_list", ImVec2(leftW, availHeight), true);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.85f, 0.6f, 1));
    ImGui::TextUnformatted("Rounds");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Use a static to remember the selected round (default: current round).
    static int selectedRound = 0;
    // Clamp in case season just started
    if (selectedRound >= totalRounds) selectedRound = std::max(0, totalRounds - 1);

    for (int r = 0; r < totalRounds; ++r) {
        char label[32];
        bool played = (r < app->careerRound_);
        bool current = (r == app->careerRound_);

        if (played)
            std::snprintf(label, sizeof(label), "Rd %d \xE2\x9C\x93", r + 1);  // UTF-8 check mark
        else if (current)
            std::snprintf(label, sizeof(label), "Rd %d >", r + 1);
        else
            std::snprintf(label, sizeof(label), "Rd %d", r + 1);

        bool sel = (r == selectedRound);
        if (played)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.9f, 0.55f, 1));
        else if (current)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.3f, 1));
        else
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.85f, 1));

        if (ImGui::Selectable(label, sel))
            selectedRound = r;

        ImGui::PopStyleColor();
    }

    ImGui::EndChild();

    // ?? RIGHT: fixture list for selectedRound ?????????????????????????????
    ImGui::SameLine();
    ImGui::BeginChild("fixture_detail", ImVec2(rightW, availHeight), true);

    // Header: date + round
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.85f, 0.6f, 1));
    ImGui::SetWindowFontScale(1.15f);

    char hdr[128];
    if (selectedRound < static_cast<int>(app->careerRoundDates_.size())) {
        const auto& rd = app->careerRoundDates_[selectedRound];
        const char* mon = (rd.month >= 1 && rd.month <= 12) ? kMonths[rd.month] : "?";
        std::snprintf(hdr, sizeof(hdr), "Round %d  —  %d %s %d",
                      selectedRound + 1, rd.day, mon, rd.year);
    } else {
        std::snprintf(hdr, sizeof(hdr), "Round %d", selectedRound + 1);
    }
    ImGui::TextUnformatted(hdr);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Table of fixtures for this round
    if (selectedRound < totalRounds) {
        size_t from = app->roundStart_[selectedRound];
        size_t to   = app->roundStart_[selectedRound + 1];

        ImGuiTableFlags tf = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                             ImGuiTableFlags_SizingFixedFit;

        if (ImGui::BeginTable("fix_tbl", 5, tf)) {
            ImGui::TableSetupColumn("Home",   ImGuiTableColumnFlags_WidthFixed, 220);
            ImGui::TableSetupColumn("",       ImGuiTableColumnFlags_WidthFixed,  30);  // HG
            ImGui::TableSetupColumn("-",      ImGuiTableColumnFlags_WidthFixed,  14);
            ImGui::TableSetupColumn("",       ImGuiTableColumnFlags_WidthFixed,  30);  // AG
            ImGui::TableSetupColumn("Away",   ImGuiTableColumnFlags_WidthFixed, 220);
            ImGui::TableHeadersRow();

            for (size_t i = from; i < to; ++i) {
                Team* home = app->teamById(app->fixtures_[i].first);
                Team* away = app->teamById(app->fixtures_[i].second);
                if (!home || !away) continue;

                bool playerInvolved = (home->id == app->careerTeam_ ||
                                       away->id == app->careerTeam_);
                bool played = (i < app->fixtureResults_.size() &&
                               app->fixtureResults_[i].played);

                ImGui::TableNextRow();
                if (playerInvolved)
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                                           IM_COL32(50, 80, 50, 220));

                // Home team name
                ImGui::TableSetColumnIndex(0);
                if (home->id == app->careerTeam_)
                    ImGui::TextColored(ImVec4(1, 1, 0.3f, 1), "%s", home->name.c_str());
                else
                    ImGui::TextUnformatted(home->name.c_str());

                // Score or "vs"
                if (played) {
                    int hg = app->fixtureResults_[i].hg;
                    int ag = app->fixtureResults_[i].ag;

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", hg);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted("-");

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%d", ag);
                } else {
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextDisabled(" ");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), "vs");
                    ImGui::TableSetColumnIndex(3);
                    ImGui::TextDisabled(" ");
                }

                // Away team name
                ImGui::TableSetColumnIndex(4);
                if (away->id == app->careerTeam_)
                    ImGui::TextColored(ImVec4(1, 1, 0.3f, 1), "%s", away->name.c_str());
                else
                    ImGui::TextUnformatted(away->name.c_str());
            }

            ImGui::EndTable();
        }
    }

    ImGui::EndChild();
    ImGui::End();
}

}  // namespace nm
