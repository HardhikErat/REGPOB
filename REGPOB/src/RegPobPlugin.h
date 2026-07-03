#pragma once

#include "AircraftCache.h"
#include "PluginConstants.h"

#include <mutex>
#include <set>
#include <string>

class RegPobPlugin : public EuroScopePlugIn::CPlugIn
{
public:
    RegPobPlugin();
    ~RegPobPlugin() override = default;

    void OnGetTagItem(
        EuroScopePlugIn::CFlightPlan FlightPlan,
        EuroScopePlugIn::CRadarTarget RadarTarget,
        int ItemCode,
        int TagData,
        char sItemString[16],
        int* pColorCode,
        COLORREF* pRGB,
        double* pFontSize) override;

    void OnFunctionCall(
        int FunctionId,
        const char* sItemString,
        POINT Pt,
        RECT Area) override;

    void OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan) override;
    void OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan FlightPlan) override;
    void OnRefreshFpListContent(EuroScopePlugIn::CFlightPlanList AcList) override;
    void OnTimer(int Counter) override;
    bool OnCompileCommand(const char* sCommandLine) override;

private:
    AircraftCache cache_;
    EuroScopePlugIn::CFlightPlanList regPobList_;
    bool suppressRemarkSync_ = false;
    bool suppressAnnotationSync_ = false;
    bool listColumnsConfigured_ = false;
    bool startupMessageShown_ = false;
    std::set<std::string> regPobListMembers_;

    mutable std::mutex pendingEditMutex_;
    std::string pendingEditCallsign_;

    void ConfigureRegPobListColumns();
    void EnsureCacheInitialized(const EuroScopePlugIn::CFlightPlan& flightPlan);
    AircraftExtraData GetDisplayData(const EuroScopePlugIn::CFlightPlan& flightPlan);
    EuroScopePlugIn::CFlightPlan ResolveFlightPlanForEdit(
        const char* sItemString,
        int functionId) const;
    EuroScopePlugIn::CFlightPlan ResolveFlightPlanForCommit();
    EuroScopePlugIn::CFlightPlan FindFlightPlanByCallsign(const std::string& callsign) const;
    EuroScopePlugIn::CFlightPlan FindFlightPlanByRegistration(const std::string& registration) const;
    EuroScopePlugIn::CFlightPlan FindSingleDepartureCandidate() const;
    void ApplyRemarksUpdate(EuroScopePlugIn::CFlightPlan flightPlan);
    void SyncAnnotationsFromCache(EuroScopePlugIn::CFlightPlan flightPlan);
    void SyncCacheFromAnnotations(const EuroScopePlugIn::CFlightPlan& flightPlan);
    void SyncCacheFromRemarks(const EuroScopePlugIn::CFlightPlan& flightPlan);
    void HandleRegistrationCommit(EuroScopePlugIn::CFlightPlan flightPlan, const char* rawInput);
    void HandlePobCommit(EuroScopePlugIn::CFlightPlan flightPlan, const char* rawInput);
    void BeginRegistrationEdit(EuroScopePlugIn::CFlightPlan flightPlan, POINT Pt, RECT Area);
    void BeginPobEdit(EuroScopePlugIn::CFlightPlan flightPlan, POINT Pt, RECT Area);
    bool IsDepartureCandidate(const EuroScopePlugIn::CFlightPlan& flightPlan) const;
    bool IsInManagedDepartureList(const std::string& callsign) const;
    bool ShouldManageRemarks(const EuroScopePlugIn::CFlightPlan& flightPlan) const;
    void RefreshRegPobList(EuroScopePlugIn::CFlightPlanList& list);
    void ShowStartupMessage();
    static RECT NormalizeEditArea(POINT Pt, RECT Area);
    static void CopyToTagBuffer(char sItemString[16], const std::string& value);
    static std::string DisplayRegistration(const AircraftExtraData& data);
    static std::string DisplayPob(const AircraftExtraData& data);
    static bool LooksLikeCallsign(const std::string& value);
};
