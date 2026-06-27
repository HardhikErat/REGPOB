#include "pch.h"
#include "RemarkParser.h"
#include "Validation.h"

#include <regex>
#include <functional>

namespace RemarkParser
{
    namespace
    {
        const std::regex kPobTokenRegex(R"(^P/(\d+)/S$)", std::regex::ECMAScript);
        const std::regex kRegistrationValueRegex(R"(^REG/(.+)$)", std::regex::ECMAScript);

        std::string JoinTokens(const std::vector<std::string>& tokens)
        {
            std::ostringstream stream;
            for (std::size_t index = 0; index < tokens.size(); ++index)
            {
                if (index > 0)
                {
                    stream << ' ';
                }

                stream << tokens[index];
            }

            return stream.str();
        }

        std::size_t FindTokenIndex(
            const std::vector<std::string>& tokens,
            const std::function<bool(const std::string&)>& predicate)
        {
            for (std::size_t index = 0; index < tokens.size(); ++index)
            {
                if (predicate(tokens[index]))
                {
                    return index;
                }
            }

            return tokens.size();
        }
    }

    std::vector<std::string> TokenizeRemarks(const std::string& remarks)
    {
        std::vector<std::string> tokens;
        std::istringstream stream(remarks);
        std::string token;

        while (stream >> token)
        {
            tokens.push_back(token);
        }

        return tokens;
    }

    bool IsRegistrationToken(const std::string& token)
    {
        return token.rfind("REG/", 0) == 0;
    }

    bool IsPobToken(const std::string& token)
    {
        return std::regex_match(token, kPobTokenRegex);
    }

    bool IsEndOfFplMarker(const std::string& token)
    {
        return token == "/V/";
    }

    std::string ExtractRegistration(const std::string& remarks)
    {
        std::smatch match;
        for (const std::string& token : TokenizeRemarks(remarks))
        {
            if (std::regex_match(token, match, kRegistrationValueRegex))
            {
                return Validation::SanitizeRegistration(match[1].str());
            }
        }

        return {};
    }

    int ExtractPOB(const std::string& remarks)
    {
        std::smatch match;
        for (const std::string& token : TokenizeRemarks(remarks))
        {
            if (std::regex_match(token, match, kPobTokenRegex))
            {
                try
                {
                    return std::stoi(match[1].str());
                }
                catch (...)
                {
                    return -1;
                }
            }
        }

        return -1;
    }

    std::string UpdateRemarks(
        const std::string& remarks,
        const std::string& registration,
        int pob)
    {
        std::vector<std::string> tokens = TokenizeRemarks(remarks);

        tokens.erase(
            std::remove_if(tokens.begin(), tokens.end(), [](const std::string& token)
            {
                return IsRegistrationToken(token) || IsPobToken(token);
            }),
            tokens.end());

        std::vector<std::string> insertTokens;
        if (!registration.empty())
        {
            insertTokens.push_back("REG/" + registration);
        }

        if (pob > 0)
        {
            insertTokens.push_back("P/" + std::to_string(pob) + "/S");
        }

        if (insertTokens.empty())
        {
            return JoinTokens(tokens);
        }

        const std::size_t endMarkerIndex = FindTokenIndex(tokens, IsEndOfFplMarker);
        if (endMarkerIndex < tokens.size())
        {
            tokens.insert(tokens.begin() + static_cast<std::ptrdiff_t>(endMarkerIndex),
                insertTokens.begin(), insertTokens.end());
            return JoinTokens(tokens);
        }

        const std::size_t remarksIndex = FindTokenIndex(tokens, [](const std::string& token)
        {
            return token.rfind("RMK/", 0) == 0;
        });

        if (remarksIndex < tokens.size())
        {
            tokens.insert(tokens.begin() + static_cast<std::ptrdiff_t>(remarksIndex),
                insertTokens.begin(), insertTokens.end());
        }
        else
        {
            tokens.insert(tokens.end(), insertTokens.begin(), insertTokens.end());
        }

        return JoinTokens(tokens);
    }
}
