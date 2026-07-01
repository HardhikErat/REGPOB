#include "pch.h"
#include "Validation.h"

namespace Validation
{
    std::string SanitizeRegistration(const std::string& input)
    {
        std::string result;
        result.reserve(input.size());

        for (unsigned char ch : input)
        {
            if (std::isalnum(ch))
            {
                result.push_back(static_cast<char>(std::toupper(ch)));
            }
        }

        return result;
    }

    bool IsValidRegistration(const std::string& input)
    {
        const std::string sanitized = SanitizeRegistration(input);
        if (sanitized.empty())
        {
            return false;
        }

        return std::all_of(sanitized.begin(), sanitized.end(), [](unsigned char ch)
        {
            return std::isalnum(ch) != 0;
        });
    }

    std::optional<int> ParsePob(const std::string& input)
    {
        if (input.empty())
        {
            return std::nullopt;
        }

        for (unsigned char ch : input)
        {
            if (!std::isdigit(ch))
            {
                return std::nullopt;
            }
        }

        try
        {
            const long value = std::stol(input);
            if (value <= 0)
            {
                return std::nullopt;
            }

            return static_cast<int>(value);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }
}
