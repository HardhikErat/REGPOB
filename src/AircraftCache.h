#pragma once

#include <map>
#include <mutex>
#include <string>

struct AircraftExtraData
{
    std::string registration;
    int pob = 0;
    bool initialized = false;
    bool registrationEdited = false;
    bool pobEdited = false;
};

class AircraftCache
{
public:
    bool Contains(const std::string& callsign) const;
    AircraftExtraData Get(const std::string& callsign) const;
    void Set(const std::string& callsign, const AircraftExtraData& data);
    void SetRegistration(const std::string& callsign, const std::string& registration, bool userEdited = true);
    void SetPob(const std::string& callsign, int pob, bool userEdited = true);
    void Remove(const std::string& callsign);
    void Clear();

private:
    mutable std::mutex mutex_;
    std::map<std::string, AircraftExtraData> entries_;
};
