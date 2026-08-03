// Unit tests for cmPositionFormat (Championship Manager style position string).
#include <iostream>
#include <string>
#include "core/Player.h"
#include "core/PositionFormat.h"

using namespace nm;

namespace {

int g_failures = 0;

void check(const std::string& label, const std::string& actual, const std::string& expected) {
    bool pass = actual == expected;
    std::cout << label << ": " << actual << "\n";
    std::cout << "Expected: " << expected << "\n";
    std::cout << (pass ? "PASS" : "FAIL") << "\n\n";
    if (!pass) ++g_failures;
}

}  // namespace

int RunCmFormatTests() {
    g_failures = 0;
    std::cout << "Testing Championship Manager Position Format\n";
    std::cout << "============================================\n\n";

    // David Beckham - MC, MR
    {
        Player beckham;
        beckham.name = "David Beckham";
        beckham.primaryPos = Position::MC;
        beckham.playablePositions = {Position::MC, Position::MR};
        check("David Beckham (MC, MR)", cmPositionFormat(beckham), "M C/R");
    }

    // Brian Laudrup - MR, AMR, ML, AML, FC, FR, FL
    {
        Player laudrup;
        laudrup.name = "Brian Laudrup";
        laudrup.primaryPos = Position::MR;
        laudrup.playablePositions = {
            Position::MR, Position::AMR, Position::ML, Position::AML,
            Position::FC, Position::FR, Position::FL
        };
        check("Brian Laudrup (MR,AMR,ML,AML,FC,FR,FL)", cmPositionFormat(laudrup),
              "M/AM R/L, F C/R/L");
    }

    // Simple Goalkeeper - GK only
    {
        Player gk;
        gk.name = "Peter Schmeichel";
        gk.primaryPos = Position::GK;
        gk.playablePositions = {Position::GK};
        check("Peter Schmeichel (GK)", cmPositionFormat(gk), "GK");
    }

    // Centre-back - DC only
    {
        Player cb;
        cb.name = "Rio Ferdinand";
        cb.primaryPos = Position::DC;
        cb.playablePositions = {Position::DC};
        check("Rio Ferdinand (DC)", cmPositionFormat(cb), "D C");
    }

    // Versatile defender - DC, DR, DL
    {
        Player def;
        def.name = "Versatile Defender";
        def.primaryPos = Position::DC;
        def.playablePositions = {Position::DC, Position::DR, Position::DL};
        check("Versatile Defender (DC,DR,DL)", cmPositionFormat(def), "D C/R/L");
    }

    // Box-to-box midfielder - MC, AMC
    {
        Player b2b;
        b2b.name = "Box-to-Box Mid";
        b2b.primaryPos = Position::MC;
        b2b.playablePositions = {Position::MC, Position::AMC};
        check("Box-to-Box Mid (MC,AMC)", cmPositionFormat(b2b), "M/AM C");
    }

    // Wide winger - MR, AMR
    {
        Player winger;
        winger.name = "Winger";
        winger.primaryPos = Position::MR;
        winger.playablePositions = {Position::MR, Position::AMR};
        check("Winger (MR,AMR)", cmPositionFormat(winger), "M/AM R");
    }

    // Both flanks midfielder - MR, ML
    {
        Player wide;
        wide.name = "Both Flanks";
        wide.primaryPos = Position::MR;
        wide.playablePositions = {Position::MR, Position::ML};
        check("Both Flanks (MR,ML)", cmPositionFormat(wide), "M R/L");
    }

    std::cout << (g_failures == 0 ? "All CM format tests passed."
                                  : std::to_string(g_failures) + " CM format check(s) failed.")
              << "\n";
    return g_failures;
}
