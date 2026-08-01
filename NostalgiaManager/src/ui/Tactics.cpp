#include "Tactics.h"
#include "UIHelpers.h"
#include <algorithm>

namespace nm {

void TacticsScreen::openTactics(App* app, Team* team, App::Screen returnTo) {
    app->tacticsTeam_ = team;
    app->tacticsReturn_ = returnTo;
    app->tacticsXiSel_ = -1;
    app->tacticsSubSel_ = -1;
    app->tacticsPlayerSel_ = -1;
    if (team) {
        if (team->formation.empty()) {
            team->formation = team->preferredFormation;
        }
        if (team->startingXI.empty()) {
            team->autoSelectXI();
        }
    }
    app->screen_ = App::Screen::Tactics;
}

void TacticsScreen::render(App* app) {
    app->drawCyclingBackground();
    app->beginScreen("Tactics");

    Team* t = app->tacticsTeam_;
    if (ImGui::Button("< Back")) { 
        app->screen_ = app->tacticsReturn_; 
        ImGui::End(); 
        return; 
    }
    if (!t) {
        ImGui::TextDisabled("No team selected.");
        ImGui::End();
        return;
    }

    ImGui::SameLine();
    ImGui::SetWindowFontScale(1.25f);
    ImGui::TextColored(ImVec4(0.95f, 0.97f, 1.0f, 1), "%s", t->name.c_str());
    ImGui::SetWindowFontScale(1.0f);

    // Check if this is a human-controlled team in career mode
    const bool inCareer = (app->tacticsReturn_ == App::Screen::CareerModeBase);
    const bool isHumanTeam = !inCareer || (t->id == app->careerTeam_);
    const bool canEdit = isHumanTeam;

    // Show a warning if viewing AI team in career mode
    if (inCareer && !isHumanTeam) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1), "(View Only - AI Controlled)");
    }

    ImGui::Spacing();

    const ImVec4 gold(0.60f, 0.75f, 0.95f, 1);
    const bool inMatch = (app->tacticsReturn_ == App::Screen::Match);
    const bool subsLeft = !inMatch || app->matchSubsUsed_ < kMaxMatchSubs;
    float avail = ImGui::GetContentRegionAvail().y - 56;
    if (avail < 200) avail = 200;
    float leftW = 320.0f, rightW = 340.0f;
    float midW = ImGui::GetContentRegionAvail().x - leftW - rightW - 24;
    if (midW < 260) midW = 260;

    // ---- Left Panel: Squad List ----
    int dragSourceIdx = -1, dragTargetIdx = -1;
    int subPlayerDragId = -1;
    ImGui::BeginChild("tac_squad", ImVec2(leftW, avail), true);
    panelHeader("Squad");
    ImGui::TextColored(gold, "Starting XI");

    for (size_t i = 0; i < t->startingXI.size(); ++i) {
        Player* p = t->findPlayer(t->startingXI[i]);
        if (!p) continue;
        Position assignedPos = i < t->assignedPositions.size() ? t->assignedPositions[i] : p->primaryPos;
        char lbl[160];
        std::string posStr = cmPositionFormat(*p);
        std::snprintf(lbl, sizeof(lbl), "%2d  %-8s %s", p->shirtNumber,
                      posStr.c_str(), shortName(p->name).c_str());

        ImGui::PushID(static_cast<int>(i));
        if (ImGui::Selectable(lbl, app->tacticsPlayerSel_ == p->id)) {
            app->tacticsPlayerSel_ = (app->tacticsPlayerSel_ == p->id) ? -1 : p->id;
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            app->openPlayerDetail(p, App::Screen::Tactics);
        }

        // Only allow drag-drop if can edit
        if (canEdit && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            int idx = static_cast<int>(i);
            ImGui::SetDragDropPayload("PLAYER_SLOT", &idx, sizeof(int));
            ImGui::Text("%s", p->name.c_str());
            ImGui::EndDragDropSource();
        }

        if (canEdit && ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PLAYER_SLOT")) {
                dragSourceIdx = *static_cast<const int*>(payload->Data);
                dragTargetIdx = static_cast<int>(i);
            }
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SUB_PLAYER")) {
                subPlayerDragId = *static_cast<const int*>(payload->Data);
                dragTargetIdx = static_cast<int>(i);
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();
        positionTooltip(*p);
    }

    ImGui::Spacing();
    ImGui::TextColored(gold, "Substitutes");
    int swapStarter = -1, swapSub = -1;
    int shownSubs = 0;
    int subIdx = 0;
    int swapSubSource = -1, swapSubTarget = -1;

    for (const Player& pl : t->squad) {
        bool starting = std::find(t->startingXI.begin(), t->startingXI.end(), pl.id) !=
                        t->startingXI.end();
        if (starting) continue;
        if (shownSubs >= 5) break;
        shownSubs++;
        char lbl[160];
        std::string posStr = cmPositionFormat(pl);
        std::snprintf(lbl, sizeof(lbl), "%2d  %-8s %s", pl.shirtNumber,
                      posStr.c_str(), shortName(pl.name).c_str());

        ImGui::PushID(100 + subIdx);
        if (ImGui::Selectable(lbl, app->tacticsXiSel_ == pl.id)) {
            if (canEdit && app->tacticsXiSel_ != -1) { swapStarter = app->tacticsXiSel_; swapSub = pl.id; }
            else if (canEdit) app->tacticsXiSel_ = (app->tacticsXiSel_ == pl.id) ? -1 : pl.id;
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            app->openPlayerDetail(&pl, App::Screen::Tactics);
        }

        if (canEdit && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            int subId = pl.id;
            ImGui::SetDragDropPayload("SUB_PLAYER", &subId, sizeof(int));
            ImGui::Text("%s", pl.name.c_str());
            ImGui::EndDragDropSource();
        }

        if (canEdit && ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PLAYER_SLOT")) {
                int starterIdx = *static_cast<const int*>(payload->Data);
                if (starterIdx >= 0 && starterIdx < static_cast<int>(t->startingXI.size())) {
                    swapStarter = t->startingXI[starterIdx];
                    swapSub = pl.id;
                }
            }
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SUB_PLAYER")) {
                int sourceSubId = *static_cast<const int*>(payload->Data);
                if (sourceSubId != pl.id) {
                    swapSubSource = sourceSubId;
                    swapSubTarget = pl.id;
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();
        positionTooltip(pl);
        subIdx++;
    }

    // Rest of Squad (only when NOT in match)
    if (!inMatch) {
        ImGui::Spacing();
        ImGui::TextColored(gold, "Rest of Squad");
        int squadIdx = 0;
        int restIdx = 0;
        for (const Player& pl : t->squad) {
            bool starting = std::find(t->startingXI.begin(), t->startingXI.end(), pl.id) !=
                            t->startingXI.end();
            if (starting) continue;

            squadIdx++;
            if (squadIdx <= 5) continue;

            char lbl[160];
            std::string posStr = cmPositionFormat(pl);
            std::snprintf(lbl, sizeof(lbl), "%2d  %-8s %s", pl.shirtNumber,
                          posStr.c_str(), shortName(pl.name).c_str());

            ImGui::PushID(200 + restIdx);
            if (ImGui::Selectable(lbl, app->tacticsXiSel_ == pl.id)) {
                if (canEdit && app->tacticsXiSel_ != -1) { swapStarter = app->tacticsXiSel_; swapSub = pl.id; }
                else if (canEdit) app->tacticsXiSel_ = (app->tacticsXiSel_ == pl.id) ? -1 : pl.id;
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                app->openPlayerDetail(&pl, App::Screen::Tactics);
            }

            if (canEdit && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                int subId = pl.id;
                ImGui::SetDragDropPayload("SUB_PLAYER", &subId, sizeof(int));
                ImGui::Text("%s", pl.name.c_str());
                ImGui::EndDragDropSource();
            }

            ImGui::PopID();
            positionTooltip(pl);
            restIdx++;
        }
    }

    if (swapStarter != -1 && swapSub != -1 && subsLeft) {
        auto it = std::find(t->startingXI.begin(), t->startingXI.end(), swapStarter);
        if (it != t->startingXI.end()) {
            *it = swapSub;
            if (inMatch) ++app->matchSubsUsed_;
        }
        app->tacticsXiSel_ = app->tacticsSubSel_ = -1;
    }

    if (swapSubSource != -1 && swapSubTarget != -1) {
        auto itSource = std::find_if(t->squad.begin(), t->squad.end(),
                                      [swapSubSource](const Player& p) { return p.id == swapSubSource; });
        auto itTarget = std::find_if(t->squad.begin(), t->squad.end(),
                                      [swapSubTarget](const Player& p) { return p.id == swapSubTarget; });
        if (itSource != t->squad.end() && itTarget != t->squad.end()) {
            std::iter_swap(itSource, itTarget);
        }
    }

    if (inMatch) {
        ImGui::Spacing();
        ImVec4 c = subsLeft ? gold : ImVec4(0.9f, 0.4f, 0.2f, 1);
        ImGui::TextColored(c, "Subs: %d / %d", app->matchSubsUsed_, kMaxMatchSubs);
    }
    ImGui::EndChild();

    // ---- Centre Panel: Tactical Pitch (abbreviated for space) ----
    ImGui::SameLine();
    ImGui::BeginChild("tac_pitch", ImVec2(midW, avail), true);
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 o = ImGui::GetCursorScreenPos();
        ImVec2 sz = ImGui::GetContentRegionAvail();

        // Draw pitch background and stripes
        dl->AddRectFilled(o, ImVec2(o.x + sz.x, o.y + sz.y), IM_COL32(74, 132, 62, 255), 4);
        int stripes = 10;
        for (int s = 0; s < stripes; ++s) {
            if (s % 2) continue;
            float y0 = o.y + sz.y * s / stripes;
            float y1 = o.y + sz.y * (s + 1) / stripes;
            dl->AddRectFilled(ImVec2(o.x, y0), ImVec2(o.x + sz.x, y1),
                              IM_COL32(82, 142, 68, 255));
        }
        ImU32 line = IM_COL32(225, 235, 220, 150);
        dl->AddRect(ImVec2(o.x + 8, o.y + 8), ImVec2(o.x + sz.x - 8, o.y + sz.y - 8),
                    line, 0, 0, 2.0f);
        dl->AddLine(ImVec2(o.x + 8, o.y + sz.y * 0.5f),
                    ImVec2(o.x + sz.x - 8, o.y + sz.y * 0.5f), line, 2.0f);
        dl->AddCircle(ImVec2(o.x + sz.x * 0.5f, o.y + sz.y * 0.5f),
                      sz.x * 0.12f, line, 32, 2.0f);

        float padX = 40.0f, padY = 34.0f;
        float drawWidth = sz.x - 2 * padX;
        float drawHeight = sz.y - 2 * padY;

        // Position mapping helper
        auto getPositionCoords = [](Position pos) -> std::pair<float, float> {
            float yPos = 0.5f, xPos = 0.5f;
            switch (pos) {
                case Position::FL:  yPos = 0.08f; xPos = 0.25f; break;
                case Position::FC:  yPos = 0.08f; xPos = 0.50f; break;
                case Position::FR:  yPos = 0.08f; xPos = 0.75f; break;
                case Position::AML: yPos = 0.28f; xPos = 0.20f; break;
                case Position::AMC: yPos = 0.28f; xPos = 0.50f; break;
                case Position::AMR: yPos = 0.28f; xPos = 0.80f; break;
                case Position::ML:  yPos = 0.48f; xPos = 0.15f; break;
                case Position::MC:  yPos = 0.48f; xPos = 0.50f; break;
                case Position::MR:  yPos = 0.48f; xPos = 0.85f; break;
                case Position::DM:  yPos = 0.65f; xPos = 0.50f; break;
                case Position::WBL: yPos = 0.74f; xPos = 0.12f; break;
                case Position::DL:  yPos = 0.80f; xPos = 0.22f; break;
                case Position::DC:  yPos = 0.83f; xPos = 0.50f; break;
                case Position::DR:  yPos = 0.80f; xPos = 0.78f; break;
                case Position::WBR: yPos = 0.74f; xPos = 0.88f; break;
                case Position::GK:  yPos = 0.95f; xPos = 0.50f; break;
            }
            return {xPos, yPos};
        };

        // Draw formation slots (simplified - showing positioned players)
        std::map<Position, int> positionCount;
        for (size_t i = 0; i < t->assignedPositions.size(); ++i) {
            positionCount[t->assignedPositions[i]]++;
        }

        std::map<Position, int> positionIndex;
        const ImGuiPayload* dragPayload = ImGui::GetDragDropPayload();
        bool isDragging = dragPayload && (dragPayload->IsDataType("PLAYER_SLOT") || dragPayload->IsDataType("SUB_PLAYER"));

        for (size_t slotIdx = 0; slotIdx < t->startingXI.size() && slotIdx < t->assignedPositions.size(); ++slotIdx) {
            Player* p = t->findPlayer(t->startingXI[slotIdx]);
            Position assignedPos = t->assignedPositions[slotIdx];

            auto [baseX, baseY] = getPositionCoords(assignedPos);

            int totalAtPos = positionCount[assignedPos];
            int thisIndex = positionIndex[assignedPos]++;
            float xOffset = 0.0f;

            if (totalAtPos > 1) {
                bool isCentral = (baseX >= 0.35f && baseX <= 0.65f);
                if (isCentral) {
                    if (totalAtPos == 2) {
                        xOffset = (thisIndex == 0) ? -0.15f : 0.15f;
                    } else if (totalAtPos == 3) {
                        float offsets[] = {-0.15f, 0.0f, 0.15f};
                        xOffset = offsets[thisIndex];
                    } else if (totalAtPos == 4) {
                        float offsets[] = {-0.22f, -0.08f, 0.08f, 0.22f};
                        xOffset = offsets[thisIndex];
                    } else if (totalAtPos == 5) {
                        float offsets[] = {-0.22f, -0.11f, 0.0f, 0.11f, 0.22f};
                        xOffset = offsets[thisIndex];
                    } else {
                        float spacing = 0.44f / (totalAtPos - 1);
                        xOffset = (thisIndex * spacing) - 0.22f;
                    }
                } else {
                    if (totalAtPos == 2) {
                        xOffset = (thisIndex == 0) ? -0.08f : 0.08f;
                    } else if (totalAtPos == 3) {
                        float offsets[] = {-0.10f, 0.0f, 0.10f};
                        xOffset = offsets[thisIndex];
                    } else {
                        float spacing = 0.20f / (totalAtPos - 1);
                        xOffset = (thisIndex * spacing) - 0.10f;
                    }
                }
            }

            float xPos = std::max(0.05f, std::min(0.95f, baseX + xOffset));
            float x = o.x + padX + drawWidth * xPos;
            float y = o.y + padY + drawHeight * baseY;

            bool gk = RoleOf(assignedPos) == Role::GK;
            ImU32 jersey = gk ? IM_COL32(60, 150, 70, 255) : IM_COL32(46, 96, 176, 255);
            if (p && app->tacticsPlayerSel_ == p->id) jersey = IM_COL32(200, 140, 60, 255);
            float r = 21.0f;

            ImGui::SetCursorScreenPos(ImVec2(x - r, y - r));
            ImGui::PushID(static_cast<int>(slotIdx) + 1000);
            ImGui::InvisibleButton("pos", ImVec2(r * 2, r * 2));
            bool hovered = ImGui::IsItemHovered();
            bool clicked = ImGui::IsItemClicked();

            if (p && clicked) {
                app->tacticsPlayerSel_ = (app->tacticsPlayerSel_ == p->id) ? -1 : p->id;
            }

            if (p && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                int srcIdx = static_cast<int>(slotIdx);
                ImGui::SetDragDropPayload("PLAYER_SLOT", &srcIdx, sizeof(int));
                ImGui::Text("%s", p->name.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PLAYER_SLOT")) {
                    int srcIdx = *static_cast<const int*>(payload->Data);
                    int tgtIdx = static_cast<int>(slotIdx);
                    if (srcIdx != tgtIdx && dragSourceIdx == -1) {
                        dragSourceIdx = srcIdx;
                        dragTargetIdx = tgtIdx;
                    }
                }
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SUB_PLAYER")) {
                    subPlayerDragId = *static_cast<const int*>(payload->Data);
                    dragTargetIdx = static_cast<int>(slotIdx);
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopID();

            if (isDragging && hovered) {
                dl->AddCircle(ImVec2(x, y), r + 4, IM_COL32(255, 220, 100, 255), 24, 3.0f);
            }

            if (p) {
                dl->AddCircleFilled(ImVec2(x, y), r, jersey, 24);
                dl->AddCircle(ImVec2(x, y), r, IM_COL32(225, 200, 120, 255), 24, 2.0f);

                PlayerTactics& pt = t->tactics.getPlayerTactics(p->id);
                if (pt.forwardRun != ForwardRun::None) {
                    float arrowLen = (pt.forwardRun == ForwardRun::Short) ? 15.0f : 25.0f;
                    ImVec2 arrowStart = ImVec2(x, y - r - 2);
                    ImVec2 arrowEnd = ImVec2(x, y - r - 2 - arrowLen);
                    dl->AddLine(arrowStart, arrowEnd, IM_COL32(255, 200, 80, 255), 2.5f);
                    dl->AddTriangleFilled(
                        ImVec2(arrowEnd.x, arrowEnd.y),
                        ImVec2(arrowEnd.x - 5, arrowEnd.y + 7),
                        ImVec2(arrowEnd.x + 5, arrowEnd.y + 7),
                        IM_COL32(255, 200, 80, 255));
                }

                char num[8];
                std::snprintf(num, sizeof(num), "%d", p->shirtNumber);
                ImVec2 ns = ImGui::CalcTextSize(num);
                dl->AddText(ImVec2(x - ns.x * 0.5f, y - ns.y * 0.5f),
                            IM_COL32(255, 255, 255, 255), num);

                std::string posLabel = PosName(assignedPos);
                ImVec2 ps = ImGui::CalcTextSize(posLabel.c_str());
                dl->AddText(ImVec2(x - ps.x * 0.5f, y - r - ps.y - 1),
                            IM_COL32(248, 214, 130, 255), posLabel.c_str());

                char nm[64];
                std::snprintf(nm, sizeof(nm), "%s", shortName(p->name).c_str());
                ImVec2 ms = ImGui::CalcTextSize(nm);
                float lx = x - ms.x * 0.5f - 4, ly = y + r + 3;
                dl->AddRectFilled(ImVec2(lx, ly), ImVec2(lx + ms.x + 8, ly + ms.y + 4),
                                  IM_COL32(20, 24, 18, 210), 3);
                dl->AddText(ImVec2(lx + 4, ly + 2), IM_COL32(238, 232, 214, 255), nm);
            } else {
                dl->AddCircle(ImVec2(x, y), r, IM_COL32(180, 180, 180, 150), 24, 2.0f);
                std::string posLabel = PosName(assignedPos);
                ImVec2 ps = ImGui::CalcTextSize(posLabel.c_str());
                dl->AddText(ImVec2(x - ps.x * 0.5f, y - ps.y * 0.5f),
                            IM_COL32(180, 180, 180, 200), posLabel.c_str());
            }
        }

        if (!inMatch) {
            ImVec2 tp = ImVec2(o.x + 10, o.y + sz.y - 25);
            dl->AddText(tp, IM_COL32(230, 230, 230, 200), "Drag players to swap or create custom positions");
        }

        // Pitch drop target for custom positions (simplified)
        ImGui::SetCursorScreenPos(o);
        ImGui::InvisibleButton("pitch_drop", sz);
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PLAYER_SLOT")) {
                ImVec2 mousePos = ImGui::GetMousePos();
                float relX = (mousePos.x - o.x - padX) / drawWidth;
                float relY = (mousePos.y - o.y - padY) / drawHeight;
                relX = std::max(0.05f, std::min(0.95f, relX));
                relY = std::max(0.05f, std::min(0.95f, relY));

                int srcIdx = *static_cast<const int*>(payload->Data);
                Position newPos = Position::MC;

                // Determine closest position based on Y coordinate
                if (relY < 0.15f) {
                    if (relX < 0.35f) newPos = Position::FL;
                    else if (relX > 0.65f) newPos = Position::FR;
                    else newPos = Position::FC;
                } else if (relY < 0.35f) {
                    if (relX < 0.3f) newPos = Position::AML;
                    else if (relX > 0.7f) newPos = Position::AMR;
                    else newPos = Position::AMC;
                } else if (relY < 0.58f) {
                    if (relX < 0.25f) newPos = Position::ML;
                    else if (relX > 0.75f) newPos = Position::MR;
                    else newPos = Position::MC;
                } else if (relY < 0.72f) {
                    newPos = Position::DM;
                } else if (relY < 0.88f) {
                    if (relX < 0.2f) newPos = Position::WBL;
                    else if (relX > 0.8f) newPos = Position::WBR;
                    else if (relX < 0.35f) newPos = Position::DL;
                    else if (relX > 0.65f) newPos = Position::DR;
                    else newPos = Position::DC;
                } else {
                    newPos = Position::GK;
                }

                bool positionExists = false;
                for (size_t i = 0; i < t->assignedPositions.size(); ++i) {
                    if (i != srcIdx && t->assignedPositions[i] == newPos) {
                        positionExists = true;
                        break;
                    }
                }

                if (!positionExists || t->assignedPositions[srcIdx] != newPos) {
                    t->assignedPositions[srcIdx] = newPos;
                    t->updatePlayerRoles();
                    if (t->tactics.formation.find(" Custom") == std::string::npos) {
                        t->tactics.formation = t->tactics.formation + " Custom";
                        t->formation = t->tactics.formation;
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }
    ImGui::EndChild();

    // Apply swaps
    if (dragSourceIdx != -1 && dragTargetIdx != -1 && dragSourceIdx != dragTargetIdx) {
        std::swap(t->startingXI[dragSourceIdx], t->startingXI[dragTargetIdx]);
        t->updatePlayerRoles();
    }

    if (subPlayerDragId != -1 && dragTargetIdx != -1) {
        t->startingXI[dragTargetIdx] = subPlayerDragId;
        if (inMatch) ++app->matchSubsUsed_;
        t->updatePlayerRoles();
    }

    // ---- Right Panel: Formation & Team Tactics ----
    ImGui::SameLine();
    ImGui::BeginChild("tac_right", ImVec2(rightW, avail), false);

    ImGui::BeginChild("tac_formation", ImVec2(0, avail * 0.35f), true);
    panelHeader("Formation");
    tacticRow("Current:", t->tactics.formation);
    ImGui::Spacing();
    if (!canEdit) ImGui::BeginDisabled();
    if (tintButton("Change Formation", IM_COL32(60, 130, 60, 255), ImVec2(-1, 34))) {
        std::string baseFormation = t->tactics.formation;
        const std::string customSuffix = " Custom";
        if (baseFormation.size() >= customSuffix.size() && 
            baseFormation.substr(baseFormation.size() - customSuffix.size()) == customSuffix) {
            baseFormation = baseFormation.substr(0, baseFormation.size() - customSuffix.size());
        }

        int n = kNumFormations;
        int cur = 0;
        for (int i = 0; i < n; ++i)
            if (baseFormation == kFormations[i]) { cur = i; break; }
        t->tactics.formation = kFormations[(cur + 1) % n];
        t->formation = t->tactics.formation;
        if (!inMatch) {
            t->autoSelectXI();
            app->tacticsXiSel_ = app->tacticsSubSel_ = app->tacticsPlayerSel_ = -1;
        } else {
            t->updateFormationPositions();
        }
    }
    if (!canEdit) ImGui::EndDisabled();
    ImGui::EndChild();

    ImGui::BeginChild("tac_team_instructions", ImVec2(0, avail * 0.35f), true);
    panelHeader("Team Instructions");

    if (!canEdit) ImGui::BeginDisabled();
    if (ImGui::Button(PassingStyleName(t->tactics.passingStyle).c_str(), ImVec2(-1, 28))) {
        int val = static_cast<int>(t->tactics.passingStyle);
        t->tactics.passingStyle = static_cast<PassingStyle>((val + 1) % 3);
    }
    ImGui::TextDisabled("Passing Style");
    ImGui::Spacing();

    if (ImGui::Button(TacklingStyleName(t->tactics.tacklingStyle).c_str(), ImVec2(-1, 28))) {
        int val = static_cast<int>(t->tactics.tacklingStyle);
        t->tactics.tacklingStyle = static_cast<TacklingStyle>((val + 1) % 3);
    }
    ImGui::TextDisabled("Tackling");
    ImGui::Spacing();

    if (ImGui::Button(PressingLevelName(t->tactics.pressing).c_str(), ImVec2(-1, 28))) {
        int val = static_cast<int>(t->tactics.pressing);
        t->tactics.pressing = static_cast<PressingLevel>((val + 1) % 3);
    }
    ImGui::TextDisabled("Pressing");
    ImGui::Spacing();

    if (ImGui::Checkbox("Counter Attack", &t->tactics.counterAttack)) {}
    ImGui::Spacing();

    if (ImGui::Checkbox("Offside Trap", &t->tactics.offsideTrap)) {}

    if (!canEdit) ImGui::EndDisabled();

    ImGui::EndChild();

    ImGui::BeginChild("tac_player_instr", ImVec2(0, 0), true);
    panelHeader("Player Instructions");
    if (app->tacticsPlayerSel_ != -1) {
        Player* selPlayer = t->findPlayer(app->tacticsPlayerSel_);
        if (selPlayer) {
            ImGui::TextColored(gold, "%s", selPlayer->name.c_str());
            ImGui::Spacing();

            PlayerTactics& pt = t->tactics.getPlayerTactics(selPlayer->id);

            ImGui::Text("Forward Runs:");
            if (!canEdit) ImGui::BeginDisabled();
            if (ImGui::Button(ForwardRunName(pt.forwardRun).c_str(), ImVec2(-1, 28))) {
                int val = static_cast<int>(pt.forwardRun);
                pt.forwardRun = static_cast<ForwardRun>((val + 1) % 3);
            }
            if (!canEdit) ImGui::EndDisabled();
        }
    } else {
        ImGui::TextDisabled("Select a player");
    }
    ImGui::EndChild();
    ImGui::EndChild();

    // ---- Action bar ----
    ImGui::Spacing();
    if (inMatch || !canEdit) ImGui::BeginDisabled();
    if (ImGui::Button("Auto-pick XI", ImVec2(160, 40))) {
        t->autoSelectXI();
        app->tacticsXiSel_ = app->tacticsSubSel_ = app->tacticsPlayerSel_ = -1;
    }
    if (inMatch || !canEdit) ImGui::EndDisabled();
    ImGui::SameLine();
    if (inMatch || !canEdit) ImGui::BeginDisabled();
    if (ImGui::Button("Reset Instructions", ImVec2(160, 40))) {
        for (auto& pt : t->tactics.playerSettings) {
            pt.forwardRun = ForwardRun::None;
        }
    }
    if (inMatch || !canEdit) ImGui::EndDisabled();
    ImGui::SameLine();

    // Handle different screen return contexts
    if (app->tacticsReturn_ == App::Screen::CareerModeBase) {
        // Career mode - find opponent and start match
        Team* home = nullptr;
        Team* away = nullptr;

        // Get the fixture details
        if (app->careerPlayerMatchIdx_ < app->fixtures_.size()) {
            int homeId = app->fixtures_[app->careerPlayerMatchIdx_].first;
            int awayId = app->fixtures_[app->careerPlayerMatchIdx_].second;
            home = app->teamById(homeId);
            away = app->teamById(awayId);
        }

        bool ok = home && away;
        if (!ok) {
            ImGui::BeginDisabled();
            // Debug: show why button is disabled
            if (!app->careerMatchPending_) {
                ImGui::TextDisabled("No match pending");
            } else if (app->careerPlayerMatchIdx_ >= app->fixtures_.size()) {
                ImGui::TextDisabled("Invalid fixture index");
            } else if (!home || !away) {
                ImGui::TextDisabled("Cannot find teams");
            }
        }
        if (tintButton("Play Match", IM_COL32(86, 150, 38, 255), ImVec2(220, 40))) {
            if (home && away) {
                app->startMatch(home, away);
            }
        }
        if (!ok) ImGui::EndDisabled();
    } else if (app->tacticsReturn_ == App::Screen::Friendly) {
        Team* away = app->teamById(app->awayTeam_);
        bool ok = away && t->id != away->id;
        if (!ok) ImGui::BeginDisabled();
        if (tintButton("Kick Off", IM_COL32(86, 150, 38, 255), ImVec2(220, 40)))
            app->startMatch(t, away);
        if (!ok) ImGui::EndDisabled();
    } else if (app->tacticsReturn_ == App::Screen::Match) {
        if (tintButton("Resume Match", IM_COL32(70, 120, 150, 255), ImVec2(220, 40)))
            app->screen_ = App::Screen::Match;
    } else {
        // Debug: show which return screen this is
        char debugText[64];
        std::snprintf(debugText, sizeof(debugText), "Return screen: %d", static_cast<int>(app->tacticsReturn_));
        ImGui::TextDisabled("%s", debugText);
        ImGui::SameLine();
        if (tintButton("Back to Career", IM_COL32(196, 150, 40, 255), ImVec2(220, 40)))
            app->screen_ = App::Screen::Career;
    }

    ImGui::End();
}

}  // namespace nm
