#include "CareerModeBase.h"
#include "UIHelpers.h"
#include <algorithm>

namespace nm {

void CareerModeBaseScreen::render(App* app) {
    // Draw static background
    app->drawStaticBackground(app->careerModeBaseBg_);

    app->beginScreen("Career Mode", false);

    Team* myteam = app->teamById(app->careerTeam_);
    int totalRounds = static_cast<int>(app->roundStart_.size()) - 1;

    // Get available space for layout
    float availWidth = ImGui::GetContentRegionAvail().x;
    float availHeight = ImGui::GetContentRegionAvail().y;

    // === TOP BAR: Manager info on left, Calendar on right ===
    ImGui::BeginGroup();

    // Left side: Manager and team info
    ImGui::SetWindowFontScale(1.2f);
    ImGui::TextColored(ImVec4(0.95f, 0.97f, 1.0f, 1), "Manager: %s", app->managerName_);
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Text("Managing: %s (%s)", myteam ? myteam->name.c_str() : "?", app->careerLeagueName_.c_str());
    ImGui::Text("Round %d / %d", app->careerRound_, totalRounds);

    ImGui::EndGroup();

    // Right side: Calendar
    ImGui::SameLine(availWidth - 250);
    ImGui::BeginChild("calendar_widget", ImVec2(250, 240), true);

    // Build events list for current month using pre-computed round dates.
    std::vector<std::pair<int, std::string>> monthEvents;

    for (int r = 0; r < static_cast<int>(app->careerRoundDates_.size()); ++r) {
        const auto& rd = app->careerRoundDates_[r];
        if (rd.year == app->calendarViewYear_ && rd.month == app->calendarViewMonth_) {
            if (r == app->careerRound_) {
                monthEvents.push_back({rd.day, "Next Match"});
            } else if (r < app->careerRound_) {
                monthEvents.push_back({rd.day, "Match (completed)"});
            } else {
                monthEvents.push_back({rd.day, "Upcoming Match"});
            }
        }
    }

    // Boxing Day indicator
    if (app->calendarViewMonth_ == 12) {
        bool hasBoxingDay = false;
        for (const auto& comp : app->careerCompetitions_) { (void)comp; }
        // Check if any round is already on 26 Dec (Boxing Day); if so it's labelled above.
        // Add a standing "Boxing Day" marker if not already a match event.
        bool bdCovered = false;
        for (auto& ev : monthEvents)
            if (ev.first == 26) { bdCovered = true; break; }
        if (!bdCovered) {
            // Only add if country has Boxing Day enabled (England etc.)
            for (const auto& sf : app->careerCompetitions_) { (void)sf; }
        }
    }

    drawCalendar(app->calendarViewYear_, app->calendarViewMonth_, 
                 app->currentYear_, app->currentMonth_, app->currentDay_, monthEvents);

    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Use child windows for side-by-side layout instead of nested columns
    float sidebarWidth = 200;
    float contentWidth = availWidth - sidebarWidth - 20;
    float contentHeight = availHeight - 260; // Account for top bar

    // === LEFT SIDEBAR: Navigation Buttons ===
    ImGui::BeginChild("left_sidebar", ImVec2(sidebarWidth, contentHeight), true);

    const ImVec2 buttonSize(180, 50);

    if (ImGui::Button("Team", buttonSize) && myteam) {
        app->teamOverviewTeam_ = myteam;
        app->teamOverviewReturn_ = App::Screen::CareerModeBase;
        app->screen_ = App::Screen::TeamOverview;
        ImGui::EndChild();
        ImGui::End();
        return;
    }

    if (ImGui::Button("Reserve Team", buttonSize)) {
        // TODO: Implement Reserve Team screen
    }

    if (ImGui::Button("Leagues", buttonSize)) {
        // TODO: Implement Leagues screen (showing all competitions for the country)
    }
    // Show all competitions for the country as a tooltip
    if (ImGui::IsItemHovered() && !app->careerCompetitions_.empty()) {
        ImGui::BeginTooltip();
        for (const auto& comp : app->careerCompetitions_) {
            if (comp.type != "Knockout")
                ImGui::Text("[L%d] %s", comp.tier, comp.name.c_str());
            else
                ImGui::Text("[Cup] %s", comp.name.c_str());
        }
        ImGui::EndTooltip();
    }

    if (ImGui::Button("Fixtures", buttonSize)) {
        app->screen_ = App::Screen::CareerFixtures;
        ImGui::EndChild();
        ImGui::End();
        return;
    }

    if (ImGui::Button("Transfers", buttonSize)) {
        // TODO: Implement Transfers screen
    }

    if (ImGui::Button("Wages", buttonSize)) {
        app->screen_ = App::Screen::Wages;
        ImGui::EndChild();
        ImGui::End();
        return;
    }

    if (ImGui::Button("Transfer Prices", buttonSize)) {
        app->screen_ = App::Screen::TransferPrices;
        ImGui::EndChild();
        ImGui::End();
        return;
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
        app->screen_ = App::Screen::MatchDay;
        ImGui::EndChild();
        ImGui::End();
        return;
    }
    if (done) ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Exit button at the bottom
    if (ImGui::Button("Exit to Main Menu", buttonSize)) {
        app->screen_ = App::Screen::Main;
        ImGui::EndChild();
        ImGui::End();
        return;
    }

    if (done) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1, 0.9f, 0.3f, 1), "Season\ncomplete!");
    }

    ImGui::EndChild();

    // === RIGHT CONTENT AREA: News and League Table side by side ===
    ImGui::SameLine();

    float newsWidth = contentWidth * 0.50f;
    float tableWidth = contentWidth * 0.50f;

    // === NEWS SECTION (LEFT) ===
    ImGui::BeginChild("news_section", ImVec2(newsWidth, contentHeight), true);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.85f, 0.6f, 1));
    ImGui::SetWindowFontScale(1.1f);
    ImGui::Text("News");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Display recent results as news
    if (app->careerLog_.empty()) {
        ImGui::TextWrapped("No news yet. Start the season to see match results and updates!");
    } else {
        // Show last 10 results
        int count = 0;
        for (auto it = app->careerLog_.rbegin(); it != app->careerLog_.rend() && count < 10; ++it, ++count) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1));
            ImGui::BulletText("%s", it->c_str());
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }
    }

    ImGui::Spacing();

    // Additional news items (placeholder for future features)
    if (app->careerRound_ == 0) {
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1), "Season Start!");
        ImGui::TextWrapped("Welcome to the new season! Good luck managing %s.", 
                          myteam ? myteam->name.c_str() : "your team");
    }

    ImGui::EndChild();

    // === LEAGUE TABLE SECTION (RIGHT) ===
    ImGui::SameLine();

    ImGui::BeginChild("league_table_section", ImVec2(tableWidth, contentHeight), true);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.85f, 0.6f, 1));
    ImGui::SetWindowFontScale(1.1f);
    ImGui::Text("League Table");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::Separator();
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

    ImGuiTableFlags tf = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("standings", 9, tf, ImVec2(0, contentHeight - 70))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30);
        ImGui::TableSetupColumn("Club", ImGuiTableColumnFlags_WidthFixed, 180);
        const char* nums[] = {"P", "W", "D", "L", "GF", "GA", "Pts"};
        for (const char* c : nums)
            ImGui::TableSetupColumn(c, ImGuiTableColumnFlags_WidthFixed, 40);
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

    ImGui::EndChild();
    ImGui::End();
}

}  // namespace nm
