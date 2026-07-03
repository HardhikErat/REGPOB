#include "pch.h"
#include "RemarkParser.h"
#include "Validation.h"

#include <cctype>
#include <functional>
#include <regex>

namespace RemarkParser
{
    namespace
    {
        const std::regex kPobTokenRegex(R"(^P/(\d*)/S$)", std::regex::ECMAScript);
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

        bool IsDofToken(const std::string& token)
        {
            return token.rfind("DOF/", 0) == 0;
        }

        bool IsOprToken(const std::string& token)
        {
            return token.rfind("OPR/", 0) == 0;
        }

        std::size_t FindRegistrationInsertIndex(const std::vector<std::string>& tokens)
        {
            const std::size_t dofIndex = FindTokenIndex(tokens, IsDofToken);
            const std::size_t oprIndex = FindTokenIndex(tokens, IsOprToken);

            if (dofIndex < tokens.size())
            {
                return dofIndex + 1;
            }

            if (oprIndex < tokens.size())
            {
                return oprIndex;
            }

            const std::size_t endMarkerIndex = FindTokenIndex(tokens, IsEndOfFplMarker);
            if (endMarkerIndex < tokens.size())
            {
                return endMarkerIndex;
            }

            return tokens.size();
        }

        std::size_t FindPobInsertIndex(const std::vector<std::string>& tokens)
        {
            const std::size_t endMarkerIndex = FindTokenIndex(tokens, IsEndOfFplMarker);
            if (endMarkerIndex < tokens.size())
            {
                return endMarkerIndex;
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
        if (token.size() != 3 || token.front() != '/' || token.back() != '/')
        {
            return false;
        }

        const char marker = static_cast<char>(std::toupper(static_cast<unsigned char>(token[1])));
        return marker == 'V' || marker == 'T' || marker == 'R';
    }

    bool ContainsRegistrationToken(const std::string& remarks)
    {
        const std::vector<std::string> tokens = TokenizeRemarks(remarks);
        return std::any_of(tokens.begin(), tokens.end(), IsRegistrationToken);
    }

    bool ContainsPobToken(const std::string& remarks)
    {
        const std::vector<std::string> tokens = TokenizeRemarks(remarks);
        return std::any_of(tokens.begin(), tokens.end(), IsPobToken);
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
                if (match[1].str().empty())
                {
                    return -1;
                }

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

        const bool insertReg = !registration.empty();
        const bool insertPob = pob > 0;

        if (!insertReg && !insertPob)
        {
            return JoinTokens(tokens);
        }

        if (insertReg && insertPob)
        {
            std::size_t regIndex = FindRegistrationInsertIndex(tokens);
            std::size_t pobIndex = FindPobInsertIndex(tokens);

            if (regIndex <= pobIndex)
            {
                tokens.insert(tokens.begin() + static_cast<std::ptrdiff_t>(regIndex), "REG/" + registration);
                ++pobIndex;
                tokens.insert(
                    tokens.begin() + static_cast<std::ptrdiff_t>(pobIndex),
                    "P/" + std::to_string(pob) + "/S");
            }
            else
            {
                tokens.insert(
                    tokens.begin() + static_cast<std::ptrdiff_t>(pobIndex),
                    "P/" + std::to_string(pob) + "/S");
                tokens.insert(tokens.begin() + static_cast<std::ptrdiff_t>(regIndex), "REG/" + registration);
            }
        }
        else if (insertReg)
        {
            const std::size_t regIndex = FindRegistrationInsertIndex(tokens);
            tokens.insert(tokens.begin() + static_cast<std::ptrdiff_t>(regIndex), "REG/" + registration);
        }
        else
        {
            const std::size_t pobIndex = FindPobInsertIndex(tokens);
            tokens.insert(
                tokens.begin() + static_cast<std::ptrdiff_t>(pobIndex),
                "P/" + std::to_string(pob) + "/S");
        }

        return JoinTokens(tokens);
    }
}
