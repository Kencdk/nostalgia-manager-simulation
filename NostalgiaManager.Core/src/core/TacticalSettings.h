#pragma once
#include <string>
#include <vector>

namespace nm {

// Forward run instructions (how far players advance)
enum class ForwardRun {
    None,   // Maintain position
    Short,  // Limited forward movement
    Long    // Frequently advances into attacking positions
};

inline std::string ForwardRunName(ForwardRun fr) {
    switch (fr) {
        case ForwardRun::None:  return "None";
        case ForwardRun::Short: return "Short";
        case ForwardRun::Long:  return "Long";
    }
    return "None";
}

// Team passing style
enum class PassingStyle {
    Short,   // Build-up play
    Mixed,   // Balanced
    Direct   // Long balls
};

inline std::string PassingStyleName(PassingStyle ps) {
    switch (ps) {
        case PassingStyle::Short:  return "Short";
        case PassingStyle::Mixed:  return "Mixed";
        case PassingStyle::Direct: return "Direct";
    }
    return "Mixed";
}

// Team tackling style
enum class TacklingStyle {
    Easy,    // Reduced aggression
    Normal,  // Balanced
    Hard     // Increased aggression
};

inline std::string TacklingStyleName(TacklingStyle ts) {
    switch (ts) {
        case TacklingStyle::Easy:   return "Easy";
        case TacklingStyle::Normal: return "Normal";
        case TacklingStyle::Hard:   return "Hard";
    }
    return "Normal";
}

// Team pressing intensity
enum class PressingLevel {
    Low,     // Positional discipline
    Medium,  // Balanced
    High     // Aggressive engagement
};

inline std::string PressingLevelName(PressingLevel pl) {
    switch (pl) {
        case PressingLevel::Low:    return "Low";
        case PressingLevel::Medium: return "Medium";
        case PressingLevel::High:   return "High";
    }
    return "Medium";
}

// Individual player tactical settings
struct PlayerTactics {
    int playerId = 0;
    ForwardRun forwardRun = ForwardRun::None;
};

// Team tactical settings
struct TeamTactics {
    std::string formation = "4-4-2";
    PassingStyle passingStyle = PassingStyle::Mixed;
    TacklingStyle tacklingStyle = TacklingStyle::Normal;
    PressingLevel pressing = PressingLevel::Medium;
    bool counterAttack = false;
    bool offsideTrap = false;

    std::vector<PlayerTactics> playerSettings;

    // Find player tactics
    PlayerTactics* findPlayerTactics(int playerId) {
        for (auto& pt : playerSettings) {
            if (pt.playerId == playerId) return &pt;
        }
        return nullptr;
    }

    // Get or create player tactics
    PlayerTactics& getPlayerTactics(int playerId) {
        PlayerTactics* existing = findPlayerTactics(playerId);
        if (existing) return *existing;

        PlayerTactics pt;
        pt.playerId = playerId;
        playerSettings.push_back(pt);
        return playerSettings.back();
    }
};

}  // namespace nm
