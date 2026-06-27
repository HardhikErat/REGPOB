#include "pch.h"
#include "AircraftCache.h"

bool AircraftCache::Contains(const std::string& callsign) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.find(callsign) != entries_.end();
}

AircraftExtraData AircraftCache::Get(const std::string& callsign) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = entries_.find(callsign);
    if (iterator == entries_.end())
    {
        return {};
    }

    return iterator->second;
}

void AircraftCache::Set(const std::string& callsign, const AircraftExtraData& data)
{
    std::lock_guard<std::mutex> lock(mutex_);
    entries_[callsign] = data;
}

void AircraftCache::SetRegistration(
    const std::string& callsign,
    const std::string& registration,
    bool userEdited)
{
    std::lock_guard<std::mutex> lock(mutex_);
    entries_[callsign].registration = registration;
    entries_[callsign].initialized = true;
    if (userEdited)
    {
        entries_[callsign].registrationEdited = true;
    }
}

void AircraftCache::SetPob(const std::string& callsign, int pob, bool userEdited)
{
    std::lock_guard<std::mutex> lock(mutex_);
    entries_[callsign].pob = pob;
    entries_[callsign].initialized = true;
    if (userEdited)
    {
        entries_[callsign].pobEdited = true;
    }
}

void AircraftCache::Remove(const std::string& callsign)
{
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.erase(callsign);
}

void AircraftCache::Clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}
