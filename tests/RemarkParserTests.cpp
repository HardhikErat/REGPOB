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
        "Case 1 registration only");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 DOF/260623 RMK/TCAS", "", 67),
        "PBN/A1B1C1D1L1O1S2 DOF/260623 P/67/S RMK/TCAS",
        "Case 2 pob only");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 DOF/260623 RMK/TCAS", "VTIBY", 67),
        "PBN/A1B1C1D1L1O1S2 DOF/260623 REG/VTIBY P/67/S RMK/TCAS",
        "Case 3 both fields");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 REG/OLDREG RMK/TCAS", "VTIBY", 0),
        "PBN/A1B1C1D1L1O1S2 REG/VTIBY RMK/TCAS",
        "Case 4 replace registration");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 P/12/S RMK/TCAS", "", 67),
        "PBN/A1B1C1D1L1O1S2 P/67/S RMK/TCAS",
        "Case 5 replace pob");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 REG/OLD P/10/S RMK/TCAS", "VTIBY", 67),
        "PBN/A1B1C1D1L1O1S2 REG/VTIBY P/67/S RMK/TCAS",
        "Case 6 replace both");

    ExpectEq(
        UpdateRemarks("PBN/A1B1C1D1L1O1S2 /V/", "VTIBY", 67),
        "PBN/A1B1C1D1L1O1S2 REG/VTIBY P/67/S /V/",
        "Case /V/ marker placement");

    const std::string sampleRemarks =
        "PBN/A1B1C1D1L1O1S2 DOF/260623 REG/VT-IBY EET/VABF0038 OPR/IGO PER/C RMK/TCAS P/67/S /V/";

    assert(ExtractRegistration(sampleRemarks) == "VTIBY");
    assert(ExtractPOB(sampleRemarks) == 67);
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
