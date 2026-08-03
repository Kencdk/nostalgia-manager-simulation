#pragma once
#include <string>

#include "Player.h"

namespace nm {

// Championship Manager style position format (e.g. "M C/R", "M/AM R/L, F C/R/L").
// Pure Player/Position logic - no UI framework dependency, so it lives in
// core/ alongside Player.h rather than in ui/UIHelpers.cpp.
std::string cmPositionFormat(const Player& p);

}  // namespace nm
