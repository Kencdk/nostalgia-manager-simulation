#include "XmlRules.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace nm {

// ---------------------------------------------------------------------------
// String helpers
// ---------------------------------------------------------------------------
std::string XmlRulesLoader::lower(const std::string& s) {
    std::string o = s;
    std::transform(o.begin(), o.end(), o.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return o;
}

std::string XmlRulesLoader::trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Return the text content of the FIRST <tag>…</tag> in xml (non-recursive).
std::string XmlRulesLoader::tagText(const std::string& xml, const std::string& tag) {
    std::string open  = "<" + tag;
    std::string close = "</" + tag + ">";
    size_t start = xml.find(open);
    if (start == std::string::npos) return "";
    // Skip to end of opening tag (may have attributes)
    size_t gt = xml.find('>', start);
    if (gt == std::string::npos) return "";
    // Self-closing?
    if (gt > 0 && xml[gt - 1] == '/') return "";
    size_t end = xml.find(close, gt + 1);
    if (end == std::string::npos) return "";
    return trim(xml.substr(gt + 1, end - gt - 1));
}

// Return the value of attr="…" or attr='…' inside `element`.
std::string XmlRulesLoader::attrValue(const std::string& element, const std::string& attr) {
    std::string key = attr + "=";
    size_t pos = element.find(key);
    if (pos == std::string::npos) return "";
    pos += key.size();
    if (pos >= element.size()) return "";
    char quote = element[pos];
    if (quote != '"' && quote != '\'') return "";
    size_t end = element.find(quote, pos + 1);
    if (end == std::string::npos) return "";
    return element.substr(pos + 1, end - pos - 1);
}

// Return all <tag …>…</tag> blocks (including self-closing <tag … />).
std::vector<std::string> XmlRulesLoader::findBlocks(const std::string& xml,
                                                     const std::string& tag) {
    std::vector<std::string> out;
    std::string open  = "<" + tag;
    std::string close = "</" + tag + ">";
    size_t pos = 0;
    while (true) {
        size_t start = xml.find(open, pos);
        if (start == std::string::npos) break;
        // Self-closing?
        size_t gt = xml.find('>', start);
        if (gt == std::string::npos) break;
        if (gt > 0 && xml[gt - 1] == '/') {
            // Self-closing tag: just include up to '>'
            out.push_back(xml.substr(start, gt - start + 1));
            pos = gt + 1;
        } else {
            size_t end = xml.find(close, gt + 1);
            if (end == std::string::npos) break;
            out.push_back(xml.substr(start, end + close.size() - start));
            pos = end + close.size();
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Parse a single XML file into a CountryRules struct.
// New unified structure:
//   <FootballCountryDatabase>
//     <Metadata><Country>…</Country></Metadata>
//     <HistoricalRuleTimeline>
//       <Rule id="SUBSTITUTIONS"><Period from="…" to="…"><MaximumSubs>…</MaximumSubs><NamedSubstitutes>…</NamedSubstitutes></Period></Rule>
//     </HistoricalRuleTimeline>
//     <LeagueStructure>
//       <League id="L1"><Name>…</Name><Tier>1</Tier><Teams>…</Teams></League>
//     </LeagueStructure>
//     <CupCompetitions>
//       <Cup id="…"><Name>…</Name>…</Cup>
//     </CupCompetitions>
//   </FootballCountryDatabase>
// ---------------------------------------------------------------------------
bool XmlRulesLoader::parseFile(const std::string& path, CountryRules& out) const {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string xml = ss.str();

    out.filename = path;

    // --- Country name from <Metadata><Country> ---
    std::string meta = tagText(xml, "Metadata");
    if (!meta.empty()) {
        out.country = trim(tagText(meta, "Country"));
    }
    // Fallback: derive from root element name (e.g. "EnglandFootballDatabase" -> "England")
    if (out.country.empty()) {
        size_t lt = xml.find('<');
        if (lt != std::string::npos) {
            size_t sp = xml.find_first_of(" \t\r\n>", lt + 1);
            std::string root = xml.substr(lt + 1, sp - lt - 1);
            for (const char* suffix : {"FootballDatabase", "Football", "Database"}) {
                size_t fb = root.find(suffix);
                if (fb != std::string::npos) { root = root.substr(0, fb); break; }
            }
            out.country = trim(root);
        }
    }

    // --- Rules from <HistoricalRuleTimeline> for year 1997 ---
    constexpr int kYear = 1997;
    auto rulesBlocks = findBlocks(xml, "Rule");
    for (const auto& rule : rulesBlocks) {
        std::string rid = attrValue(rule, "id");

        if (rid == "SUBSTITUTIONS") {
            auto periods = findBlocks(rule, "Period");
            for (const auto& p : periods) {
                int from = 0, to = 9999;
                try { from = std::stoi(attrValue(p, "from")); } catch (...) {}
                try { to   = std::stoi(attrValue(p, "to"));   } catch (...) {}
                if (from <= kYear && to >= kYear) {
                    std::string ms = tagText(p, "MaximumSubs");
                    if (!ms.empty()) {
                        try { out.maxSubs = std::stoi(ms); } catch (...) {}
                    }
                    break;
                }
            }
        } else if (rid == "BENCH_SIZE") {
            auto periods = findBlocks(rule, "Period");
            for (const auto& p : periods) {
                int from = 0, to = 9999;
                try { from = std::stoi(attrValue(p, "from")); } catch (...) {}
                try { to   = std::stoi(attrValue(p, "to"));   } catch (...) {}
                if (from <= kYear && to >= kYear) {
                    std::string ns = tagText(p, "NamedSubstitutes");
                    if (!ns.empty()) {
                        try { out.benchPlayers = std::stoi(ns); } catch (...) {}
                    }
                    break;
                }
            }
        }
    }

    // --- Leagues from <LeagueStructure> ---
    // <Rounds> and <MatchesPerOpponent> appear as siblings of <League> inside
    // <LeagueStructure>, immediately after the closing </League> tag.
    // We parse them positionally: after each </League> we look for those tags
    // before the next <League or end of the section.
    std::string leagueStructure = tagText(xml, "LeagueStructure");
    if (!leagueStructure.empty()) {
        std::string openTag = "<League";
        std::string closeTag = "</League>";
        size_t pos = 0;
        while (true) {
            size_t lStart = leagueStructure.find(openTag, pos);
            if (lStart == std::string::npos) break;
            size_t lEnd = leagueStructure.find(closeTag, lStart);
            if (lEnd == std::string::npos) break;
            lEnd += closeTag.size();

            std::string lb = leagueStructure.substr(lStart, lEnd - lStart);

            XmlCompetition c;
            c.id   = attrValue(lb, "id");
            c.name = tagText(lb, "Name");
            if (c.name.empty()) c.name = c.id;
            c.type = "League";
            std::string tierStr = tagText(lb, "Tier");
            if (!tierStr.empty()) { try { c.tier = std::stoi(tierStr); } catch (...) {} }
            std::string teamsStr = tagText(lb, "Teams");
            if (!teamsStr.empty()) { try { c.teams = std::stoi(teamsStr); } catch (...) {} }

            // Look for <Rounds> and <MatchesPerOpponent> between this </League>
            // and the next <League (or end of section).
            size_t nextLeague = leagueStructure.find(openTag, lEnd);
            size_t sibEnd = (nextLeague != std::string::npos) ? nextLeague : leagueStructure.size();
            std::string between = leagueStructure.substr(lEnd, sibEnd - lEnd);

            std::string roundsStr = tagText(between, "Rounds");
            if (!roundsStr.empty()) { try { c.rounds = std::stoi(roundsStr); } catch (...) {} }
            std::string mpoStr = tagText(between, "MatchesPerOpponent");
            if (!mpoStr.empty()) { try { c.matchesPerOpponent = std::stoi(mpoStr); } catch (...) {} }
            // Derive matchesPerOpponent from rounds if not explicit
            if (c.matchesPerOpponent == 1 && c.rounds > 0 && c.teams > 0) {
                int singleRR = c.teams - 1;
                if (c.rounds >= singleRR * 2) c.matchesPerOpponent = 2;
            }

            if (!c.name.empty()) out.competitions.push_back(std::move(c));
            pos = lEnd;
        }
    }

    // --- Cup competitions from <CupCompetitions><Cup> ---
    std::string cupSection = tagText(xml, "CupCompetitions");
    if (!cupSection.empty()) {
        auto cupBlocks = findBlocks(cupSection, "Cup");
        for (const auto& cb : cupBlocks) {
            XmlCompetition c;
            c.id   = attrValue(cb, "id");
            c.name = tagText(cb, "Name");
            if (c.name.empty()) c.name = c.id;  // fallback: use id when no <Name> tag
            c.type = "Knockout";
            c.tier = 0;  // cups are not a tier
            if (!c.id.empty()) out.competitions.push_back(std::move(c));
        }
    }

    // --- CalendarRules: WinterBreak + SpecialFixtureRules ---
    std::string calSection = tagText(xml, "CalendarRules");
    if (!calSection.empty()) {
        // Winter break flag
        std::string wbSection = tagText(calSection, "WinterBreak");
        if (!wbSection.empty()) {
            std::string enabled = tagText(wbSection, "Enabled");
            out.winterBreak = (enabled == "true");
        }

        // Parse each <Rule> inside <SpecialFixtureRules>
        std::string sfSection = tagText(calSection, "SpecialFixtureRules");
        if (!sfSection.empty()) {
            auto sfBlocks = findBlocks(sfSection, "Rule");
            for (const auto& rb : sfBlocks) {
                SpecialFixtureRule sr;
                sr.id = attrValue(rb, "id");
                sr.enabled = (tagText(rb, "Enabled") == "true");
                if (!sr.enabled) { out.specialFixtures.push_back(std::move(sr)); continue; }

                // Single date: <Date>DD-MM</Date>
                std::string dateStr = tagText(rb, "Date");
                if (!dateStr.empty()) {
                    size_t dash = dateStr.find('-');
                    if (dash != std::string::npos) {
                        try { sr.day   = std::stoi(dateStr.substr(0, dash)); } catch (...) {}
                        try { sr.month = std::stoi(dateStr.substr(dash + 1)); } catch (...) {}
                    }
                }
                // Range: <StartDate>DD-MM</StartDate> <EndDate>DD-MM</EndDate>
                auto parseDate = [&](const std::string& tag, int& day, int& month) {
                    std::string s = tagText(rb, tag);
                    if (s.empty()) return;
                    size_t dash = s.find('-');
                    if (dash == std::string::npos) return;
                    try { day   = std::stoi(s.substr(0, dash)); } catch (...) {}
                    try { month = std::stoi(s.substr(dash + 1)); } catch (...) {}
                };
                parseDate("StartDate", sr.startDay, sr.startMonth);
                parseDate("EndDate",   sr.endDay,   sr.endMonth);

                out.specialFixtures.push_back(std::move(sr));
            }
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// Scan a directory for *.xml files.
// ---------------------------------------------------------------------------
void XmlRulesLoader::load(const std::string& dir) {
    rules_.clear();
    defaultRules_ = nullptr;

    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return;

    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file()) continue;
        auto ext = entry.path().extension().string();
        // Case-insensitive .xml check
        std::string extLow = ext;
        std::transform(extLow.begin(), extLow.end(), extLow.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (extLow != ".xml") continue;

        CountryRules cr;
        if (!parseFile(entry.path().string(), cr)) continue;
        if (cr.country.empty()) continue;

        std::string key = lower(cr.country);
        rules_[key] = std::move(cr);

        // Is this the default file?
        std::string stem = entry.path().stem().string();
        std::string stemLow = lower(stem);
        if (stemLow == "default" || stemLow == "deafult" /* typo in repo */) {
            defaultRules_ = &rules_[key];
        }
    }

    // If no explicit default file, use nullptr (caller falls back to hardcoded defaults).
}

const CountryRules* XmlRulesLoader::findCountry(const std::string& country) const {
    auto it = rules_.find(lower(country));
    if (it != rules_.end()) return &it->second;
    return defaultRules_;  // may be nullptr
}

}  // namespace nm
