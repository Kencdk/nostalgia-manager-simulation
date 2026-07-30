#pragma once
#include "App.h"

namespace nm {

class TeamOverviewScreen {
public:
    static void render(App* app);
    static void openTeamOverview(App* app, Team* team, App::Screen returnTo);
};

}  // namespace nm
