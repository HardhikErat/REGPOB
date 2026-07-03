#pragma once

#include <optional>
#include <string>
#include <vector>

namespace RemarkParser
{
    // Split remarks on whitespace while preserving token order.
    std::vector<std::string> TokenizeRemarks(const std::string& remarks);

    // Extract registration from remarks (REG/ token), normalized to uppercase alphanumeric.
    std::string ExtractRegistration(const std::string& remarks);

    // Extract POB from remarks (P/<n>/S token). Returns -1 when absent or invalid.
    int ExtractPOB(const std::string& remarks);

    // Update or insert REG and POB tokens when values are present.
    // Empty registration removes REG; pob <= 0 removes P. No empty placeholders are added.
    std::string UpdateRemarks(
        const std::string& remarks,
        const std::string& registration,
        int pob
    );

    bool IsRegistrationToken(const std::string& token);
    bool IsPobToken(const std::string& token);
    // True for /V/, /T/, /R/ and lowercase variants.
    bool IsEndOfFplMarker(const std::string& token);
    bool ContainsRegistrationToken(const std::string& remarks);
    bool ContainsPobToken(const std::string& remarks);
}
