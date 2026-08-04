#include "WagesScreen.h"
#include "../core/Team.h"
#include <algorithm>
#include <cstdio>

namespace nm {

void WagesScreen::render(App* app) {
    app->drawStaticBackground(app->careerModeBaseBg_);
    app->beginScreen("Wages", false);

    Team* myTeam = app->teamById(app->careerTeam_);

    if (ImGui::Button("< Back")) {
        app->screen_ = App::Screen::CareerModeBase;
        ImGui::End();
        return;
    }

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.6f, 1),
                       "Weekly wage demands for %s",
                       myTeam ? myTeam->name.c_str() : "your squad");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (!myTeam || myTeam->squad.empty()) {
        ImGui::TextDisabled("No squad data available.");
        ImGui::End();
        return;
    }

    // Sort players by wage demand (highest first)
    std::vector<const Player*> players;
    for (const auto& p : myTeam->squad)
        players.push_back(&p);
    std::sort(players.begin(), players.end(),
              [](const Player* a, const Player* b) {
                  return a->wageDemand > b->wageDemand;
              });

    // Table
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
    float tableHeight = ImGui::GetContentRegionAvail().y - 40.0f;

    int totalWage = 0;
    if (ImGui::BeginTable("wages_table", 5, flags, ImVec2(0, tableHeight))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Name",         ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Pos",          ImGuiTableColumnFlags_WidthFixed, 50);
        ImGui::TableSetupColumn("Age",          ImGuiTableColumnFlags_WidthFixed, 45);
        ImGui::TableSetupColumn("Ability",      ImGuiTableColumnFlags_WidthFixed, 65);
        ImGui::TableSetupColumn("Wage Demand",  ImGuiTableColumnFlags_WidthFixed, 110);
        ImGui::TableHeadersRow();

        for (const Player* p : players) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(p->name.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(PosName(p->primaryPos).c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%d", p->age);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.0f", PlayerAbility(*p) * 10.0);

            ImGui::TableSetColumnIndex(4);
            char wageBuf[32];
            std::snprintf(wageBuf, sizeof(wageBuf), "\xC2\xA3%d", p->wageDemand);
            ImGui::TextUnformatted(wageBuf);

            totalWage += p->wageDemand;
        }

        ImGui::EndTable();
    }

    // Total weekly wage bill
    ImGui::Spacing();
    char totalBuf[64];
    std::snprintf(totalBuf, sizeof(totalBuf), "Total weekly wage bill: \xC2\xA3%d", totalWage);
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1), "%s", totalBuf);

    ImGui::End();
}

}  // namespace nm
