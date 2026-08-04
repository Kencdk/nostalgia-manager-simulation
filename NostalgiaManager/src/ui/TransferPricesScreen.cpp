#include "TransferPricesScreen.h"
#include "../core/Team.h"
#include <algorithm>
#include <cstdio>

namespace nm {

void TransferPricesScreen::render(App* app) {
    app->drawStaticBackground(app->careerModeBaseBg_);
    app->beginScreen("Transfer Prices", false);

    Team* myTeam = app->teamById(app->careerTeam_);

    if (ImGui::Button("< Back")) {
        app->screen_ = App::Screen::CareerModeBase;
        ImGui::End();
        return;
    }

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.6f, 1),
                       "Estimated transfer values for %s",
                       myTeam ? myTeam->name.c_str() : "your squad");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (!myTeam || myTeam->squad.empty()) {
        ImGui::TextDisabled("No squad data available.");
        ImGui::End();
        return;
    }

    // Sort players by transfer value (highest first)
    std::vector<const Player*> players;
    for (const auto& p : myTeam->squad)
        players.push_back(&p);
    std::sort(players.begin(), players.end(),
              [](const Player* a, const Player* b) {
                  return a->transferValue > b->transferValue;
              });

    // Table
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
    float tableHeight = ImGui::GetContentRegionAvail().y - 20.0f;

    if (ImGui::BeginTable("transfer_table", 6, flags, ImVec2(0, tableHeight))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Name",           ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Pos",            ImGuiTableColumnFlags_WidthFixed, 50);
        ImGui::TableSetupColumn("Age",            ImGuiTableColumnFlags_WidthFixed, 45);
        ImGui::TableSetupColumn("Ability",        ImGuiTableColumnFlags_WidthFixed, 65);
        ImGui::TableSetupColumn("Wage Demand",    ImGuiTableColumnFlags_WidthFixed, 110);
        ImGui::TableSetupColumn("Transfer Value", ImGuiTableColumnFlags_WidthFixed, 130);
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

            ImGui::TableSetColumnIndex(5);
            // Format large numbers with M / K suffixes for readability
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
            ImGui::TextUnformatted(tvBuf);
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

}  // namespace nm
