#include "CareerModeBase.h"
#include "UIHelpers.h"
#include <algorithm>

namespace nm {

void CareerModeBaseScreen::render(App* app) {
    // Draw cycling background
    app->drawCyclingBackground();

    app->beginScreen("Career Mode");

    Team* myteam = app->teamById(app->careerTeam_);
    int totalRounds = static_cast<int>(app->roundStart_.size()) - 1;

    // Header with manager and team info
    ImGui::SetWindowFontScale(1.2f);
    ImGui::TextColored(ImVec4(0.95f, 0.97f, 1.0f, 1), "Manager: %s", app->managerName_);
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Text("Managing: %s (%s)", myteam ? myteam->name.c_str() : "?", app->careerLeagueName_.c_str());
    ImGui::Text("Round %d / %d", app->careerRound_, totalRounds);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Get available space for layout
    float availWidth = ImGui::GetContentRegionAvail().x;
    float availHeight = ImGui::GetContentRegionAvail().y;

    // Create columns: Left sidebar for buttons, right for content
    ImGui::Columns(2, "career_layout", false);
    ImGui::SetColumnWidth(0, 200);  // Fixed width for button column

    // === LEFT COLUMN: Navigation Buttons ===
    const ImVec2 buttonSize(180, 50);

    if (ImGui::Button("Team", buttonSize) && myteam) {
        app->teamOverviewTeam_ = myteam;
        app->teamOverviewReturn_ = App::Screen::CareerModeBase;
        app->screen_ = App::Screen::TeamOverview;
    }

    if (ImGui::Button("Reserve Team", buttonSize)) {
        // TODO: Implement Reserve Team screen
    }

    if (ImGui::Button("League", buttonSize)) {
        // TODO: Implement League screen (showing current view)
    }

    if (ImGui::Button("Fixtures", buttonSize)) {
        // TODO: Implement Fixtures screen
    }

    if (ImGui::Button("Transfers", buttonSize)) {
        // TODO: Implement Transfers screen
    }

    if (ImGui::Button("Search", buttonSize)) {
        // TODO: Implement Search screen
    }

    if (ImGui::Button("Save Game", buttonSize)) {
        app->careerSave();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Advance Round button (with conditional enable/disable)
    bool done = app->careerRound_ >= totalRounds;
    if (done) ImGui::BeginDisabled();
    if (ImGui::Button("Advance Round", buttonSize)) {
        app->careerAdvance();
    }
    if (done) ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Exit button at the bottom
    if (ImGui::Button("Exit to Main Menu", buttonSize)) {
        app->screen_ = App::Screen::Main;
        ImGui::Columns(1);
        ImGui::End();
        return;
    }

    if (done) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1, 0.9f, 0.3f, 1), "Season\ncomplete!");
    }

    // === RIGHT COLUMN: Content Area ===
    ImGui::NextColumn();

    // League Standings
    ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.6f, 1), "League Standings");
    ImGui::Spacing();

    // Standings sorted by points then goal difference
    std::vector<App::Standing> rows;
    rows.reserve(app->table_.size());
    for (auto& kv : app->table_) rows.push_back(kv.second);
    std::sort(rows.begin(), rows.end(), [](const App::Standing& a, const App::Standing& b) {
        if (a.pts != b.pts) return a.pts > b.pts;
        int ga = a.gf - a.ga, gb = b.gf - b.ga;
        if (ga != gb) return ga > gb;
        return a.gf > b.gf;
    });

    float contentWidth = ImGui::GetContentRegionAvail().x;
    ImGuiTableFlags tf = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("standings", 9, tf, ImVec2(contentWidth * 0.6f, 350))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30);
        ImGui::TableSetupColumn("Club", ImGuiTableColumnFlags_WidthFixed, 180);
        const char* nums[] = {"P", "W", "D", "L", "GF", "GA", "Pts"};
        for (const char* c : nums)
            ImGui::TableSetupColumn(c, ImGuiTableColumnFlags_WidthFixed, 34);
        ImGui::TableHeadersRow();

        int pos = 1;
        for (const App::Standing& s : rows) {
            ImGui::TableNextRow();
            bool mine = s.teamId == app->careerTeam_;
            if (mine)
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(40, 80, 40, 255));
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", pos++);
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(s.name.c_str());
            int vals[] = {s.p, s.w, s.d, s.l, s.gf, s.ga, s.pts};
            for (int i = 0; i < 7; ++i) {
                ImGui::TableSetColumnIndex(2 + i);
                ImGui::Text("%d", vals[i]);
            }
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();

    // Recent Results
    ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.6f, 1), "Recent Results");
    ImGui::Spacing();
    ImGui::BeginChild("results_log", ImVec2(contentWidth * 0.6f, 200), true);
    for (auto it = app->careerLog_.rbegin(); it != app->careerLog_.rend(); ++it)
        ImGui::TextWrapped("%s", it->c_str());
    ImGui::EndChild();

    ImGui::Columns(1);
    ImGui::End();
}

}  // namespace nm
