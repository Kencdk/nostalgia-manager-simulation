#pragma once
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui.h"

#include "../data/Database.h"
#include "../engine/Config.h"
#include "../engine/MatchEngine.h"

namespace nm {

// A GPU texture handle. Created by the active rendering backend (OpenGL3 or
// Direct3D 11) via AppLoadTexture, which each platform entry point defines.
struct AppTexture {
    ImTextureID id = (ImTextureID)0;
    int w = 0;
    int h = 0;
    bool ok = false;
};

// Loads a PNG/JPG file into a GPU texture. Implemented per backend in the
// platform entry point (main_glfw.cpp / main_win32.cpp).
bool AppLoadTexture(const std::string& path, AppTexture* out);

// Loads a texture from memory buffer. Implemented per backend in the
// platform entry point (main_glfw.cpp / main_win32.cpp).
bool AppLoadTextureFromMemory(const unsigned char* image_data, size_t image_size, AppTexture* out);

// Applies the shared "nostalgia" visual style (colors, rounding, spacing).
void ApplyNostalgiaTheme();

// Forward declarations for component classes
class TacticsScreen;
class MatchScreen;
class PlayerDetailScreen;
class TeamOverviewScreen;
class MatchDayScreen;

// Dear ImGui application: the windowed front-end for Nostalgia Manager
// Simulation. It drives the same Config / Database / MatchEngine core used by
// the original console build, but presents everything as clickable screens.
class App {
public:
    enum class Screen { Main, Friendly, Tactics, Match, Database, Career, CareerSetup, CareerModeBase, MatchDay, Data, About, PlayerDetail, TeamOverview };

    // Per-player match statistics (for tracking performance)
    struct PlayerMatchStats {
        int goals = 0;
        int assists = 0;
        int shots = 0;
        int shotsOnTarget = 0;
        int passes = 0;
        int passesCompleted = 0;
        int tackles = 0;
        int interceptions = 0;
        int fouls = 0;
        int minutesPlayed = 0;
        bool isSubstitute = false;
    };

    bool init(const std::string& dataDir);
    void render();  // call once per frame
    bool wantQuit() const { return quit_; }

    // Allow component classes to access App internals
    friend class TacticsScreen;
    friend class MatchScreen;
    friend class PlayerDetailScreen;
    friend class TeamOverviewScreen;
    friend class CareerModeBaseScreen;
    friend class MatchDayScreen;

private:

    // One snapshot of the match at the moment an event was narrated.
    struct Frame {
        std::string text;
        bool key = false;
        int minute = 0;
        std::string pitch;
        int hg = 0, ag = 0;
        float ballX = 52.5f, ballY = 34.0f;  // Continuous ball position (0-105m, 0-68m)
        int ballCol = 7, ballRow = 5;  // Grid position for compatibility (1..13, 0..8)
        int carrier = 0;               // 0 home, 1 away
        MatchStats stats;              // cumulative stats at this frame

        // Player positions for graphical rendering
        struct PlayerPos {
            int shirtNumber = 0;
            float x = 52.5f;  // Continuous X position (0-105m)
            float y = 34.0f;  // Continuous Y position (0-68m)
            int col = 7;      // Grid column for compatibility (1-13)
            int row = 5;      // Grid row for compatibility (0-8)
            int side = 0;
            bool isCarrier = false;
        };
        std::vector<PlayerPos> players;
    };

    // Career standings row.
    struct Standing {
        int teamId = 0;
        std::string name;
        int p = 0, w = 0, d = 0, l = 0, gf = 0, ga = 0, pts = 0;
    };

    void beginScreen(const char* title, bool withBackground = true);
    void beginFullscreen(const char* id, bool withBackground);
    void drawCyclingBackground();  // Helper for cycling screenshot backgrounds
    void drawStaticBackground(const AppTexture& texture);  // Helper for static background
    void renderMain();
    void renderFriendly();
    void renderTactics();
    void renderMatch();
    void drawPitch(ImVec2 pos, ImVec2 size, const Frame* f, const Frame* nextF, float t);
    void renderDatabase();
    void renderCareer();
    void renderCareerSetup();
    void renderCareerModeBase();
    void renderMatchDay();
    void renderData();
    void renderAbout();
    void renderPlayerDetail();
    void renderTeamOverview();

    void teamPicker(const char* id, int& leagueIdx, int& teamId, char* filter,
                    size_t filterSz);
    void startMatch(Team* home, Team* away);
    void openTactics(Team* team, Screen returnTo);
    void openPlayerDetail(const Player* player, Screen returnTo);
    void openTeamOverview(Team* team, Screen returnTo);
    Team* teamById(int id);

    // Career helpers
    void careerStart(int teamId);
    void careerAdvance();
    void careerAdvanceToPlayerMatch();  // Start player's match in career mode
    void careerFinishRound();           // Simulate remaining matches after player's match
    void careerSave();
    void careerLoad();

    Config cfg_;
    Database db_;
    std::string dataDir_;
    std::string status_;
    Screen screen_ = Screen::Main;
    bool quit_ = false;

    AppTexture menuBg_;
    AppTexture playerDetailBg_;  // Background for player detail screen
    AppTexture teamOverviewBg_;  // Background for team overview screen
    AppTexture careerModeBaseBg_;  // Background for career mode base screen
    std::vector<std::string> leagues_;
    std::vector<AppTexture> friendlyScreenshots_;  // Decorative images for Friendly screen

    // Tactics screen
    Team* tacticsTeam_ = nullptr;
    Screen tacticsReturn_ = Screen::Friendly;
    int tacticsXiSel_ = -1;   // selected starter (player id) for a swap
    int tacticsSubSel_ = -1;  // selected substitute (player id) for a swap
    int matchSubsUsed_ = 0;   // substitutions made in the current match (max 3)
    int tacticsDragPlayer_ = -1;  // player currently being dragged on pitch
    int tacticsPlayerSel_ = -1;   // selected player for individual instructions
    static constexpr int kMaxMatchSubs = 3;

    // Friendly selection
    int homeLeague_ = 0, awayLeague_ = 0;
    int homeTeam_ = -1, awayTeam_ = -1;
    char homeFilter_[64] = "";
    char awayFilter_[64] = "";

    // Match playback
    std::vector<Frame> frames_;
    std::string matchHome_, matchAway_;
    int finalHG_ = 0, finalAG_ = 0, finalHS_ = 0, finalAS_ = 0;
    std::vector<std::pair<int, std::string>> homeScorers_, awayScorers_;

    // Per-player match statistics (shirt number -> stats)
    std::map<int, PlayerMatchStats> homePlayerStats_;
    std::map<int, PlayerMatchStats> awayPlayerStats_;

    Team* matchHomeTeam_ = nullptr;
    Team* matchAwayTeam_ = nullptr;
    size_t playIdx_ = 0;
    double playAccum_ = 0.0;
    float speed_ = 8.0f;  // events revealed per second
    bool playing_ = true;
    bool matchOver_ = false;
    bool halftimePause_ = false;  // true when paused at halftime
    size_t halftimeIdx_ = 0;       // frame index where halftime occurs

    // Database browse
    char dbSearch_[128] = "";

    // Custom data sources
    char playersPath_[512] = "";
    char clubsPath_[512] = "";

    // Career
    bool careerActive_ = false;
    int careerTeam_ = -1;
    int careerLeague_ = 0;
    char careerFilter_[64] = "";
    char managerName_[128] = "";  // Manager's name for career mode
    std::string careerLeagueName_;
    std::vector<std::pair<int, int>> fixtures_;  // flattened home,away pairs
    std::vector<size_t> roundStart_;             // index into fixtures_ per round
    int careerRound_ = 0;
    std::unordered_map<int, Standing> table_;
    std::vector<std::string> careerLog_;

    // Career match flow
    bool careerMatchPending_ = false;  // True when player match needs to be played
    size_t careerPlayerMatchIdx_ = 0;  // Index of player's match in current round

    // Calendar and date tracking
    int currentYear_ = 1997;      // Starting year
    int currentMonth_ = 8;        // Starting month (August - start of season)
    int currentDay_ = 1;          // Current day
    int calendarViewYear_ = 1997; // Year being viewed in calendar
    int calendarViewMonth_ = 8;   // Month being viewed in calendar

    // Edit Tactics screen
    int editTacticsLeague_ = 0;
    int editTacticsTeamId_ = -1;
    char editTacticsFilter_[64] = "";

    // Player Detail screen
    const Player* detailPlayer_ = nullptr;
    int detailTeamId_ = -1;
    Screen detailReturn_ = Screen::Main;

    // Team Overview screen
    Team* teamOverviewTeam_ = nullptr;
    Screen teamOverviewReturn_ = Screen::Main;
};

}  // namespace nm
