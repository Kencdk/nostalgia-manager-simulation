#pragma once
#include "App.h"

namespace nm {

class TacticsScreen {
public:
    static void render(App* app);
    static void openTactics(App* app, Team* team, App::Screen returnTo);
};

}  // namespace nm
