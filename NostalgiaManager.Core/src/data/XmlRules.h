#pragma once
#include <map>
#include <string>
#include <vector>

namespace nm {

// Describes one competition defined inside a country XML file.
struct XmlCompetition {
    std::string id;    // e.g. "PL", "SUPERLIGA"
    std::string name;  // display name, e.g. "Premier League"
    int tier = 1;      // 1 = top flight
    int teams = 0;     // 0 = not specified in XML
    int matchesPerTeam = 0;
    int rounds = 0;              // total rounds (e.g. 38); 0 = derive from teams
    int matchesPerOpponent = 1;  // 1 = single, 2 = home+away
    std::string type;  // "League" (default) or "Knockout"
};

// A special fixture date rule parsed from <SpecialFixtureRules>.
struct SpecialFixtureRule {
    std::string id;       // e.g. "BOXING_DAY", "NEW_YEAR"
    bool enabled = false;
    int day = 0;          // for single-date rules (e.g. 26 for Boxing Day)
    int month = 0;        // e.g. 12 for Boxing Day
    int startDay = 0;     // for range rules (NEW_YEAR start day)
    int startMonth = 0;
    int endDay = 0;       // for range rules (NEW_YEAR end day)
    int endMonth = 0;
};

// All rules parsed from a single country XML file.
struct CountryRules {
    std::string country;   // from <Metadata><Country>
    std::string filename;  // full path of the source XML

    int maxSubs = 3;
    int benchPlayers = 5;
    bool winterBreak = false;

    std::vector<XmlCompetition> competitions;
    std::vector<SpecialFixtureRule> specialFixtures;

    // Returns the tier-1 league, or nullptr if none.
    const XmlCompetition* topFlight() const {
        for (const auto& c : competitions)
            if (c.tier == 1 && c.type != "Knockout") return &c;
        return nullptr;
    }

    // Find a special fixture rule by id; returns nullptr if not present/enabled.
    const SpecialFixtureRule* findSpecial(const std::string& ruleId) const {
        for (const auto& r : specialFixtures)
            if (r.id == ruleId && r.enabled) return &r;
        return nullptr;
    }
};

// Scans a directory for *.xml files, parses each as a country rules file,
// and stores results keyed by lower-cased country name.
// The special file "Default.xml" (any case) is loaded as the fallback.
class XmlRulesLoader {
public:
    // Scan `dir` for XML files and populate `rules_` and `defaultRules_`.
    void load(const std::string& dir);

    // Returns the rules for `country` (case-insensitive), or the default rules
    // if no matching country file was found.  Returns nullptr only if no XML
    // files at all were loaded.
    const CountryRules* findCountry(const std::string& country) const;

    const CountryRules* defaultRules() const { return defaultRules_; }

    const std::map<std::string, CountryRules>& allRules() const { return rules_; }

private:
    std::map<std::string, CountryRules> rules_;   // lower(country) -> rules
    const CountryRules* defaultRules_ = nullptr;  // points into rules_

    // Parse a single XML file; returns false on hard read error.
    bool parseFile(const std::string& path, CountryRules& out) const;

    // Tiny helpers.
    static std::string lower(const std::string& s);
    static std::string trim(const std::string& s);
    // Extract the text content of the first occurrence of <tag>…</tag>.
    static std::string tagText(const std::string& xml, const std::string& tag);
    // Extract the value of an attribute  attr="value"  in `element`.
    static std::string attrValue(const std::string& element, const std::string& attr);
    // Find all occurrences of <Competition …> … </Competition> blocks.
    static std::vector<std::string> findBlocks(const std::string& xml,
                                               const std::string& tag);
};

}  // namespace nm
