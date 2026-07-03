#include <cassert>
#include <iostream>
#include <string>

#include "../src/RemarkParser.h"
#include "../src/Validation.h"

namespace
{
    int g_failures = 0;

    void ExpectEq(const std::string& actual, const std::string& expected, const char* label)
    {
        if (actual != expected)
        {
            std::cerr << "FAIL: " << label << '\n'
                << "  expected: [" << expected << "]\n"
                << "  actual:   [" << actual << "]\n";
            ++g_failures;
        }
    }

}

int main()
{
    using namespace RemarkParser;

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 DOF/260623 RMK/TCAS", "VTIBY", 0),
        "PBN/A1B1C1D1L1O1S2 DOF/260623 REG/VTIBY RMK/TCAS",
        "Case 1 registration only after DOF");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 DOF/260623 RMK/TCAS", "", 67),
        "PBN/A1B1C1D1L1O1S2 DOF/260623 RMK/TCAS P/67/S",
        "Case 2 pob only without empty REG");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 DOF/260623 RMK/TCAS", "VTIBY", 67),
        "PBN/A1B1C1D1L1O1S2 DOF/260623 REG/VTIBY RMK/TCAS P/67/S",
        "Case 3 both fields with DOF placement");

    ExpectEq(
        UpdateRemarks(
            "PBN/A1B1C1D1L1O1S2 DOF/260503 EET/VABF0038 OPR/AIC PER/C RMK/TCAS SIMBRIEF /V/",
            "N359SB",
            0),
        "PBN/A1B1C1D1L1O1S2 DOF/260503 REG/N359SB EET/VABF0038 OPR/AIC PER/C RMK/TCAS SIMBRIEF /V/",
        "Case REG after DOF before OPR without empty POB");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 OPR/AIC PER/C RMK/TCAS", "VTIBY", 67),
        "PBN/A1B1C1D1L1O1S2 REG/VTIBY OPR/AIC PER/C RMK/TCAS P/67/S",
        "Case REG before OPR when DOF absent");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 REG/OLDREG RMK/TCAS", "VTIBY", 0),
        "PBN/A1B1C1D1L1O1S2 RMK/TCAS REG/VTIBY",
        "Case 4 replace registration without empty POB");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 P/12/S RMK/TCAS", "", 67),
        "PBN/A1B1C1D1L1O1S2 RMK/TCAS P/67/S",
        "Case 5 replace pob without empty REG");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 REG/OLD P/10/S RMK/TCAS", "VTIBY", 67),
        "PBN/A1B1C1D1L1O1S2 RMK/TCAS REG/VTIBY P/67/S",
        "Case 6 replace both");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 /V/", "VTIBY", 67),
        "PBN/A1B1C1D1L1O1S2 REG/VTIBY P/67/S /V/",
        "Case /V/ marker placement");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 DOF/260623 RMK/TCAS /T/", "VTIBY", 67),
        "PBN/A1B1C1D1L1O1S2 DOF/260623 REG/VTIBY RMK/TCAS P/67/S /T/",
        "Case /T/ marker with DOF placement");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 DOF/260623 RMK/TCAS /R/", "VTIBY", 67),
        "PBN/A1B1C1D1L1O1S2 DOF/260623 REG/VTIBY RMK/TCAS P/67/S /R/",
        "Case /R/ marker with DOF placement");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 /v/", "VTIBY", 67),
        "PBN/A1B1C1D1L1O1S2 REG/VTIBY P/67/S /v/",
        "Case lowercase /v/ marker placement");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 /T/ /V/", "VTIBY", 67),
        "PBN/A1B1C1D1L1O1S2 REG/VTIBY P/67/S /T/ /V/",
        "Case inserts before first end marker");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 REG/VTIBY P/67/S RMK/TCAS", "", 0),
        "PBN/A1B1C1D1L1O1S2 RMK/TCAS",
        "Case clear both removes REG and POB tokens");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 DOF/260623 RMK/TCAS", "", 0),
        "PBN/A1B1C1D1L1O1S2 DOF/260623 RMK/TCAS",
        "Case empty values leave remarks unchanged");

    assert(ContainsRegistrationToken("RMK/X REG/ P//S /V/"));
    assert(ContainsPobToken("RMK/X REG/ P//S /V/"));
    assert(IsEndOfFplMarker("/V/"));
    assert(IsEndOfFplMarker("/T/"));
    assert(IsEndOfFplMarker("/R/"));
    assert(IsEndOfFplMarker("/v/"));
    assert(IsEndOfFplMarker("/t/"));
    assert(IsEndOfFplMarker("/r/"));
    assert(!IsEndOfFplMarker("/RMK/"));

    const std::string sampleRemarks =
        "PBN/A1B1C1D1L1O1S2 DOF/260623 REG/VT-IBY EET/VABF0038 OPR/IGO PER/C RMK/TCAS P/67/S /V/";

    assert(ExtractRegistration(sampleRemarks) == "VTIBY");
    assert(ExtractPOB(sampleRemarks) == 67);
    assert(ExtractRegistration("PBN/A1 DOF/260503 REG/N359SB OPR/AIC /V/") == "N359SB");
    assert(ExtractPOB("PBN/A1 DOF/260503 REG/N359SB OPR/AIC /V/") == -1);
    assert(Validation::SanitizeRegistration("VT-IBY") == "VTIBY");
    assert(Validation::IsValidRegistration("VT-IBY"));
    assert(!Validation::IsValidRegistration("VT/IBY"));
    assert(Validation::ParsePob("67").value() == 67);
    assert(!Validation::ParsePob("2A").has_value());

    if (g_failures == 0)
    {
        std::cout << "All RemarkParser tests passed.\n";
        return 0;
    }

    std::cout << g_failures << " test(s) failed.\n";
    return 1;
}
