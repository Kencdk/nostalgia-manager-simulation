#pragma once
#include "App.h"

namespace nm {

class PlayerDetailScreen {
public:
    static void render(App* app);
    static void openPlayerDetail(App* app, const Player* player, App::Screen returnTo);
};

}  // namespace nm
