#pragma once

#include <string>

namespace Validation
{
    // Strip non-alphanumeric characters and convert to uppercase.
    // Returns empty string if nothing valid remains.
    std::string SanitizeRegistration(const std::string& input);

    // Returns true when the sanitized registration is non-empty and alphanumeric only.
    bool IsValidRegistration(const std::string& input);

    // Parse a positive integer POB value. Returns std::nullopt on invalid input.
    std::optional<int> ParsePob(const std::string& input);
}
