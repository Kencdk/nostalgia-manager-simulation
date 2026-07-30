#include <iostream>
#include <vector>
#include <string>
#include "NostalgiaManager/src/core/Player.h"
#include "NostalgiaManager/src/ui/UIHelpers.h"

// Test cases for CM position format
void testCMPositionFormat() {
    using namespace nm;

    std::cout << "Testing Championship Manager Position Format\n";
    std::cout << "============================================\n\n";

    // Test 1: David Beckham - MC, MR
    {
        Player beckham;
        beckham.name = "David Beckham";
        beckham.primaryPos = Position::MC;
        beckham.playablePositions = {Position::MC, Position::MR};

        std::string result = cmPositionFormat(beckham);
        std::cout << "David Beckham (MC, MR): " << result << std::endl;
        std::cout << "Expected: M C/R" << std::endl;
        std::cout << (result == "M C/R" ? "? PASS" : "? FAIL") << "\n\n";
    }

    // Test 2: Brian Laudrup - MR, AMR, ML, AML, FC, FR, FL
    {
        Player laudrup;
        laudrup.name = "Brian Laudrup";
        laudrup.primaryPos = Position::MR;
        laudrup.playablePositions = {
            Position::MR, Position::AMR, Position::ML, Position::AML,
            Position::FC, Position::FR, Position::FL
        };

        std::string result = cmPositionFormat(laudrup);
        std::cout << "Brian Laudrup (MR,AMR,ML,AML,FC,FR,FL): " << result << std::endl;
        std::cout << "Expected: M/AM R/L, F C/R/L" << std::endl;
        std::cout << (result == "M/AM R/L, F C/R/L" ? "? PASS" : "? FAIL") << "\n\n";
    }

    // Test 3: Simple Goalkeeper - GK only
    {
        Player gk;
        gk.name = "Peter Schmeichel";
        gk.primaryPos = Position::GK;
        gk.playablePositions = {Position::GK};

        std::string result = cmPositionFormat(gk);
        std::cout << "Peter Schmeichel (GK): " << result << std::endl;
        std::cout << "Expected: GK" << std::endl;
        std::cout << (result == "GK" ? "? PASS" : "? FAIL") << "\n\n";
    }

    // Test 4: Centre-back - DC only
    {
        Player cb;
        cb.name = "Rio Ferdinand";
        cb.primaryPos = Position::DC;
        cb.playablePositions = {Position::DC};

        std::string result = cmPositionFormat(cb);
        std::cout << "Rio Ferdinand (DC): " << result << std::endl;
        std::cout << "Expected: D C" << std::endl;
        std::cout << (result == "D C" ? "? PASS" : "? FAIL") << "\n\n";
    }

    // Test 5: Versatile defender - DC, DR, DL
    {
        Player def;
        def.name = "Versatile Defender";
        def.primaryPos = Position::DC;
        def.playablePositions = {Position::DC, Position::DR, Position::DL};

        std::string result = cmPositionFormat(def);
        std::cout << "Versatile Defender (DC,DR,DL): " << result << std::endl;
        std::cout << "Expected: D C/R/L" << std::endl;
        std::cout << (result == "D C/R/L" ? "? PASS" : "? FAIL") << "\n\n";
    }

    // Test 6: Box-to-box midfielder - MC, AMC
    {
        Player b2b;
        b2b.name = "Box-to-Box Mid";
        b2b.primaryPos = Position::MC;
        b2b.playablePositions = {Position::MC, Position::AMC};

        std::string result = cmPositionFormat(b2b);
        std::cout << "Box-to-Box Mid (MC,AMC): " << result << std::endl;
        std::cout << "Expected: M/AM C" << std::endl;
        std::cout << (result == "M/AM C" ? "? PASS" : "? FAIL") << "\n\n";
    }

    // Test 7: Wide winger - MR, AMR
    {
        Player winger;
        winger.name = "Winger";
        winger.primaryPos = Position::MR;
        winger.playablePositions = {Position::MR, Position::AMR};

        std::string result = cmPositionFormat(winger);
        std::cout << "Winger (MR,AMR): " << result << std::endl;
        std::cout << "Expected: M/AM R" << std::endl;
        std::cout << (result == "M/AM R" ? "? PASS" : "? FAIL") << "\n\n";
    }

    // Test 8: Both flanks midfielder - MR, ML
    {
        Player wide;
        wide.name = "Both Flanks";
        wide.primaryPos = Position::MR;
        wide.playablePositions = {Position::MR, Position::ML};

        std::string result = cmPositionFormat(wide);
        std::cout << "Both Flanks (MR,ML): " << result << std::endl;
        std::cout << "Expected: M R/L" << std::endl;
        std::cout << (result == "M R/L" ? "? PASS" : "? FAIL") << "\n\n";
    }
}

int main() {
    testCMPositionFormat();
    return 0;
}
