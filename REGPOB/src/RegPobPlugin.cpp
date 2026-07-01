#include "pch.h"
#include "RegPobPlugin.h"

#include "RemarkParser.h"
#include "Validation.h"

#include <algorithm>
#include <cstring>

namespace
{
    constexpr char kEmptyDisplay[] = " ";

    std::string SafeString(const char* value)
    {
        return value != nullptr ? std::string(value) : std::string();
    }

    std::string TrimDisplayValue(const std::string& value)
    {
        std::string trimmed = value;
        trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), ' '), trimmed.end());
        return trimmed;
    }
}

RegPobPlugin::RegPobPlugin()
    : CPlugIn(
        EuroScopePlugIn::COMPATIBILITY_CODE,
        PLUGIN_NAME,
        PLUGIN_VERSION,
        PLUGIN_AUTHOR,
        PLUGIN_COPYRIGHT)
{
    RegisterTagItemType("REG", TAG_ITEM_REG);
    RegisterTagItemType("POB", TAG_ITEM_POB);

    // Multiple labels so the left-button dropdown is easy to find in column setup.
    RegisterTagItemFunction("REG", TAG_FUNC_EDIT_REG);
    RegisterTagItemFunction("POB", TAG_FUNC_EDIT_POB);
    RegisterTagItemFunction("Click to edit REG", TAG_FUNC_EDIT_REG);
    RegisterTagItemFunction("Click to edit POB", TAG_FUNC_EDIT_POB);

    regPobList_ = RegisterFpList("REGPOB Departures");
    ConfigureRegPobListColumns();
    regPobList_.ShowFpList(true);
}

void RegPobPlugin::ConfigureRegPobListColumns()
{
    if (!regPobList_.IsValid() || listColumnsConfigured_)
    {
        return;
    }

    if (regPobList_.GetColumnNumber() > 0)
    {
        listColumnsConfigured_ = true;
        return;
    }

    const char* itemProvider = GetPlugInName();
    // EuroScope SDK: function provider is not implemented; pass "" and use function ID only.
    const char* functionProvider = "";

    regPobList_.AddColumnDefinition(
        "CS",
        8,
        false,
        "",
        EuroScopePlugIn::TAG_ITEM_TYPE_CALLSIGN,
        functionProvider,
        EuroScopePlugIn::TAG_ITEM_FUNCTION_NO,
        functionProvider,
        EuroScopePlugIn::TAG_ITEM_FUNCTION_NO);

    regPobList_.AddColumnDefinition(
        "REG",
        7,
        true,
        itemProvider,
        TAG_ITEM_REG,
        functionProvider,
        TAG_FUNC_EDIT_REG,
        functionProvider,
        EuroScopePlugIn::TAG_ITEM_FUNCTION_NO);

    regPobList_.AddColumnDefinition(
        "POB",
        4,
        true,
        itemProvider,
        TAG_ITEM_POB,
        functionProvider,
        TAG_FUNC_EDIT_POB,
        functionProvider,
        EuroScopePlugIn::TAG_ITEM_FUNCTION_NO);

    listColumnsConfigured_ = true;
}

void RegPobPlugin::ShowStartupMessage()
{
    if (startupMessageShown_)
    {
        return;
    }

    startupMessageShown_ = true;
    DisplayUserMessage(
        "REGPOB",
        "REG/POB editing enabled",
        "REGPOB v" PLUGIN_VERSION " loaded.\n\n"
        "For the standard Departure List, each REG/POB row must have:\n"
        "  Tag item = REGPOB/REG or REGPOB/POB\n"
        "  Left button = REGPOB/REG or REGPOB/POB (same row)\n\n"
        "Without the left button, columns are display-only.\n"
        "The plugin also opens 'REGPOB Departures' with click-to-edit pre-wired.\n"
        "Type .regpob for setup help or .regpob resetlist to rebuild that list.",
        false,
        false,
        false,
        false,
        false);
}

void RegPobPlugin::EnsureCacheInitialized(const EuroScopePlugIn::CFlightPlan& flightPlan)
{
    if (!flightPlan.IsValid())
    {
        return;
    }

    const std::string callsign = flightPlan.GetCallsign();
    AircraftExtraData data = cache_.Get(callsign);
    if (data.initialized)
    {
        return;
    }

    SyncCacheFromAnnotations(flightPlan);

    data = cache_.Get(callsign);
    if (!data.initialized)
    {
        const std::string remarks = SafeString(flightPlan.GetFlightPlanData().GetRemarks());
        data.registration = RemarkParser::ExtractRegistration(remarks);
        const int pob = RemarkParser::ExtractPOB(remarks);
        data.pob = pob > 0 ? pob : 0;
        data.initialized = true;
        cache_.Set(callsign, data);
        SyncAnnotationsFromCache(flightPlan);
    }
}

AircraftExtraData RegPobPlugin::GetDisplayData(const EuroScopePlugIn::CFlightPlan& flightPlan)
{
    EnsureCacheInitialized(flightPlan);
    return cache_.Get(flightPlan.GetCallsign());
}

bool RegPobPlugin::LooksLikeCallsign(const std::string& value)
{
    const std::string trimmed = TrimDisplayValue(value);
    if (trimmed.size() < 3 || trimmed.size() > 7)
    {
        return false;
    }

    return std::all_of(trimmed.begin(), trimmed.end(), [](unsigned char ch) {
        return std::isalnum(ch) != 0;
    });
}

EuroScopePlugIn::CFlightPlan RegPobPlugin::FindFlightPlanByCallsign(const std::string& callsign) const
{
    if (callsign.empty())
    {
        return {};
    }

    EuroScopePlugIn::CFlightPlan flightPlan = FlightPlanSelect(callsign.c_str());
    if (flightPlan.IsValid())
    {
        return flightPlan;
    }

    return {};
}

EuroScopePlugIn::CFlightPlan RegPobPlugin::FindFlightPlanByRegistration(const std::string& registration) const
{
    const std::string normalized = Validation::SanitizeRegistration(registration);
    if (normalized.empty())
    {
        return {};
    }

    EuroScopePlugIn::CFlightPlan flightPlan = FlightPlanSelectFirst();
    while (flightPlan.IsValid())
    {
        const AircraftExtraData data = cache_.Get(flightPlan.GetCallsign());
        if (data.registration == normalized)
        {
            return flightPlan;
        }

        flightPlan = FlightPlanSelectNext(flightPlan);
    }

    return {};
}

EuroScopePlugIn::CFlightPlan RegPobPlugin::FindSingleDepartureCandidate() const
{
    EuroScopePlugIn::CFlightPlan match;
    int count = 0;

    EuroScopePlugIn::CFlightPlan flightPlan = FlightPlanSelectFirst();
    while (flightPlan.IsValid())
    {
        if (IsDepartureCandidate(flightPlan))
        {
            match = flightPlan;
            ++count;
            if (count > 1)
            {
                return {};
            }
        }

        flightPlan = FlightPlanSelectNext(flightPlan);
    }

    return count == 1 ? match : EuroScopePlugIn::CFlightPlan{};
}

EuroScopePlugIn::CFlightPlan RegPobPlugin::ResolveFlightPlanForEdit(
    const char* sItemString,
    int functionId) const
{
    EuroScopePlugIn::CFlightPlan flightPlan = FlightPlanSelectASEL();
    if (flightPlan.IsValid())
    {
        return flightPlan;
    }

    const std::string itemValue = TrimDisplayValue(SafeString(sItemString));
    if (LooksLikeCallsign(itemValue))
    {
        flightPlan = FindFlightPlanByCallsign(itemValue);
        if (flightPlan.IsValid())
        {
            return flightPlan;
        }
    }

    if (functionId == TAG_FUNC_EDIT_REG && !itemValue.empty() && itemValue != kEmptyDisplay)
    {
        flightPlan = FindFlightPlanByRegistration(itemValue);
        if (flightPlan.IsValid())
        {
            return flightPlan;
        }
    }

    return FindSingleDepartureCandidate();
}

EuroScopePlugIn::CFlightPlan RegPobPlugin::ResolveFlightPlanForCommit()
{
    std::string callsign;
    {
        std::lock_guard<std::mutex> lock(pendingEditMutex_);
        callsign = pendingEditCallsign_;
        pendingEditCallsign_.clear();
    }

    if (!callsign.empty())
    {
        EuroScopePlugIn::CFlightPlan flightPlan = FindFlightPlanByCallsign(callsign);
        if (flightPlan.IsValid())
        {
            SetASELAircraft(flightPlan);
            return flightPlan;
        }
    }

    EuroScopePlugIn::CFlightPlan flightPlan = FlightPlanSelectASEL();
    if (flightPlan.IsValid())
    {
        return flightPlan;
    }

    return FindSingleDepartureCandidate();
}

void RegPobPlugin::ApplyRemarksUpdate(EuroScopePlugIn::CFlightPlan flightPlan)
{
    if (!flightPlan.IsValid())
    {
        return;
    }

    const std::string callsign = flightPlan.GetCallsign();
    const AircraftExtraData data = cache_.Get(callsign);
    const std::string currentRemarks = SafeString(flightPlan.GetFlightPlanData().GetRemarks());
    const std::string updatedRemarks = RemarkParser::UpdateRemarks(
        currentRemarks,
        data.registration,
        data.pob);

    if (updatedRemarks == currentRemarks)
    {
        return;
    }

    suppressRemarkSync_ = true;
    flightPlan.GetFlightPlanData().SetRemarks(updatedRemarks.c_str());
    flightPlan.GetFlightPlanData().AmendFlightPlan();
    suppressRemarkSync_ = false;
}

void RegPobPlugin::SyncAnnotationsFromCache(EuroScopePlugIn::CFlightPlan flightPlan)
{
    if (!flightPlan.IsValid() || suppressAnnotationSync_)
    {
        return;
    }

    const AircraftExtraData data = cache_.Get(flightPlan.GetCallsign());
    suppressAnnotationSync_ = true;

    flightPlan.GetControllerAssignedData().SetFlightStripAnnotation(
        STRIP_ANNOTATION_REG,
        data.registration.c_str());

    const std::string pobText = data.pob > 0 ? std::to_string(data.pob) : std::string();
    flightPlan.GetControllerAssignedData().SetFlightStripAnnotation(
        STRIP_ANNOTATION_POB,
        pobText.c_str());

    suppressAnnotationSync_ = false;
}

void RegPobPlugin::SyncCacheFromAnnotations(const EuroScopePlugIn::CFlightPlan& flightPlan)
{
    if (!flightPlan.IsValid())
    {
        return;
    }

    const std::string callsign = flightPlan.GetCallsign();
    AircraftExtraData data = cache_.Get(callsign);

    const std::string regAnnotation = SafeString(
        flightPlan.GetControllerAssignedData().GetFlightStripAnnotation(STRIP_ANNOTATION_REG));
    const std::string pobAnnotation = SafeString(
        flightPlan.GetControllerAssignedData().GetFlightStripAnnotation(STRIP_ANNOTATION_POB));

    if (!regAnnotation.empty())
    {
        data.registration = Validation::SanitizeRegistration(regAnnotation);
        data.registrationEdited = true;
    }

    if (!pobAnnotation.empty())
    {
        const std::optional<int> pob = Validation::ParsePob(pobAnnotation);
        if (pob.has_value())
        {
            data.pob = pob.value();
            data.pobEdited = true;
        }
    }

    if (!regAnnotation.empty() || !pobAnnotation.empty())
    {
        data.initialized = true;
        cache_.Set(callsign, data);
    }
}

void RegPobPlugin::HandleRegistrationCommit(
    EuroScopePlugIn::CFlightPlan flightPlan,
    const char* rawInput)
{
    if (!flightPlan.IsValid())
    {
        return;
    }

    const std::string rawValue = SafeString(rawInput);
    const bool clearRequested = TrimDisplayValue(rawValue).empty();
    if (clearRequested)
    {
        cache_.SetRegistration(flightPlan.GetCallsign(), "", true);
        SyncAnnotationsFromCache(flightPlan);
        ApplyRemarksUpdate(flightPlan);
        return;
    }

    const std::string registration = Validation::SanitizeRegistration(rawValue);
    if (!Validation::IsValidRegistration(registration))
    {
        return;
    }

    cache_.SetRegistration(flightPlan.GetCallsign(), registration, true);
    SyncAnnotationsFromCache(flightPlan);
    ApplyRemarksUpdate(flightPlan);
}

void RegPobPlugin::HandlePobCommit(
    EuroScopePlugIn::CFlightPlan flightPlan,
    const char* rawInput)
{
    if (!flightPlan.IsValid())
    {
        return;
    }

    const std::string rawValue = SafeString(rawInput);
    const bool clearRequested = TrimDisplayValue(rawValue).empty();
    if (clearRequested)
    {
        cache_.SetPob(flightPlan.GetCallsign(), 0, true);
        SyncAnnotationsFromCache(flightPlan);
        ApplyRemarksUpdate(flightPlan);
        return;
    }

    const std::optional<int> pob = Validation::ParsePob(rawValue);
    if (!pob.has_value())
    {
        return;
    }

    cache_.SetPob(flightPlan.GetCallsign(), pob.value(), true);
    SyncAnnotationsFromCache(flightPlan);
    ApplyRemarksUpdate(flightPlan);
}

RECT RegPobPlugin::NormalizeEditArea(POINT Pt, RECT Area)
{
    if (Area.right > Area.left && Area.bottom > Area.top)
    {
        return Area;
    }

    RECT editArea = {};
    editArea.left = Pt.x - 48;
    editArea.right = Pt.x + 48;
    editArea.top = Pt.y - 12;
    editArea.bottom = Pt.y + 12;
    return editArea;
}

void RegPobPlugin::BeginRegistrationEdit(
    EuroScopePlugIn::CFlightPlan flightPlan,
    POINT Pt,
    RECT Area)
{
    if (!flightPlan.IsValid())
    {
        return;
    }

    const AircraftExtraData data = GetDisplayData(flightPlan);
    {
        std::lock_guard<std::mutex> lock(pendingEditMutex_);
        pendingEditCallsign_ = flightPlan.GetCallsign();
    }

    SetASELAircraft(flightPlan);
    const RECT editArea = NormalizeEditArea(Pt, Area);
    OpenPopupEdit(editArea, TAG_FUNC_COMMIT_REG, data.registration.c_str());
}

void RegPobPlugin::BeginPobEdit(
    EuroScopePlugIn::CFlightPlan flightPlan,
    POINT Pt,
    RECT Area)
{
    if (!flightPlan.IsValid())
    {
        return;
    }

    const AircraftExtraData data = GetDisplayData(flightPlan);
    {
        std::lock_guard<std::mutex> lock(pendingEditMutex_);
        pendingEditCallsign_ = flightPlan.GetCallsign();
    }

    SetASELAircraft(flightPlan);
    const std::string initialValue = data.pob > 0 ? std::to_string(data.pob) : std::string();
    const RECT editArea = NormalizeEditArea(Pt, Area);
    OpenPopupEdit(editArea, TAG_FUNC_COMMIT_POB, initialValue.c_str());
}

void RegPobPlugin::CopyToTagBuffer(char sItemString[16], const std::string& value)
{
    strncpy_s(sItemString, 16, value.c_str(), _TRUNCATE);
}

std::string RegPobPlugin::DisplayRegistration(const AircraftExtraData& data)
{
    return data.registration.empty() ? kEmptyDisplay : data.registration;
}

std::string RegPobPlugin::DisplayPob(const AircraftExtraData& data)
{
    return data.pob > 0 ? std::to_string(data.pob) : kEmptyDisplay;
}

void RegPobPlugin::OnGetTagItem(
    EuroScopePlugIn::CFlightPlan FlightPlan,
    EuroScopePlugIn::CRadarTarget RadarTarget,
    int ItemCode,
    int TagData,
    char sItemString[16],
    int* pColorCode,
    COLORREF* pRGB,
    double* pFontSize)
{
    (void)RadarTarget;
    (void)TagData;
    (void)pRGB;
    (void)pFontSize;

    *pColorCode = EuroScopePlugIn::TAG_COLOR_DEFAULT;
    CopyToTagBuffer(sItemString, kEmptyDisplay);

    if (!FlightPlan.IsValid())
    {
        return;
    }

    const AircraftExtraData data = GetDisplayData(FlightPlan);

    if (ItemCode == TAG_ITEM_REG)
    {
        CopyToTagBuffer(sItemString, DisplayRegistration(data));
    }
    else if (ItemCode == TAG_ITEM_POB)
    {
        CopyToTagBuffer(sItemString, DisplayPob(data));
    }
}

void RegPobPlugin::OnFunctionCall(
    int FunctionId,
    const char* sItemString,
    POINT Pt,
    RECT Area)
{
    if (FunctionId == TAG_FUNC_COMMIT_REG)
    {
        HandleRegistrationCommit(ResolveFlightPlanForCommit(), sItemString);
        return;
    }

    if (FunctionId == TAG_FUNC_COMMIT_POB)
    {
        HandlePobCommit(ResolveFlightPlanForCommit(), sItemString);
        return;
    }

    EuroScopePlugIn::CFlightPlan flightPlan = ResolveFlightPlanForEdit(sItemString, FunctionId);
    if (!flightPlan.IsValid())
    {
        return;
    }

    if (FunctionId == TAG_FUNC_EDIT_REG || FunctionId == TAG_ITEM_REG)
    {
        BeginRegistrationEdit(flightPlan, Pt, Area);
        return;
    }

    if (FunctionId == TAG_FUNC_EDIT_POB || FunctionId == TAG_ITEM_POB)
    {
        BeginPobEdit(flightPlan, Pt, Area);
    }
}

void RegPobPlugin::OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan)
{
    if (!FlightPlan.IsValid() || suppressRemarkSync_)
    {
        return;
    }

    EnsureCacheInitialized(FlightPlan);

    const std::string callsign = FlightPlan.GetCallsign();
    AircraftExtraData data = cache_.Get(callsign);
    const std::string remarks = SafeString(FlightPlan.GetFlightPlanData().GetRemarks());
    const bool hasRegToken = RemarkParser::ContainsRegistrationToken(remarks);
    const bool hasPobToken = RemarkParser::ContainsPobToken(remarks);
    const std::string extractedReg = RemarkParser::ExtractRegistration(remarks);
    const int extractedPob = RemarkParser::ExtractPOB(remarks);

    // Keep cache aligned with remarks so manual removals in remarks clear list cells too.
    data.registration = hasRegToken ? extractedReg : std::string();
    data.pob = (hasPobToken && extractedPob > 0) ? extractedPob : 0;
    data.initialized = true;
    data.registrationEdited = false;
    data.pobEdited = false;
    cache_.Set(callsign, data);
    SyncAnnotationsFromCache(FlightPlan);

    const std::string expectedRemarks = RemarkParser::UpdateRemarks(
        remarks,
        data.registration,
        data.pob);

    if (expectedRemarks != remarks)
    {
        ApplyRemarksUpdate(FlightPlan);
    }

}

void RegPobPlugin::OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan FlightPlan)
{
    if (!FlightPlan.IsValid())
    {
        return;
    }

    cache_.Remove(FlightPlan.GetCallsign());
    regPobListMembers_.erase(FlightPlan.GetCallsign());

    if (regPobList_.IsValid())
    {
        RefreshRegPobList(regPobList_);
    }
}

bool RegPobPlugin::IsDepartureCandidate(const EuroScopePlugIn::CFlightPlan& flightPlan) const
{
    if (!flightPlan.IsValid())
    {
        return false;
    }

    const std::string origin = SafeString(flightPlan.GetFlightPlanData().GetOrigin());
    if (origin.size() < 3)
    {
        return false;
    }

    const EuroScopePlugIn::CRadarTarget radarTarget = flightPlan.GetCorrelatedRadarTarget();
    if (radarTarget.IsValid() && radarTarget.GetGS() > 120)
    {
        return false;
    }

    return true;
}

void RegPobPlugin::RefreshRegPobList(EuroScopePlugIn::CFlightPlanList& list)
{
    if (!list.IsValid())
    {
        return;
    }

    for (const std::string& callsign : regPobListMembers_)
    {
        EuroScopePlugIn::CFlightPlan existing = FlightPlanSelect(callsign.c_str());
        if (existing.IsValid())
        {
            list.RemoveFpFromTheList(existing);
        }
    }

    regPobListMembers_.clear();

    EuroScopePlugIn::CFlightPlan flightPlan = FlightPlanSelectFirst();
    while (flightPlan.IsValid())
    {
        if (IsDepartureCandidate(flightPlan))
        {
            list.AddFpToTheList(flightPlan);
            regPobListMembers_.insert(flightPlan.GetCallsign());
        }

        flightPlan = FlightPlanSelectNext(flightPlan);
    }
}

void RegPobPlugin::OnRefreshFpListContent(EuroScopePlugIn::CFlightPlanList AcList)
{
    (void)AcList;

    if (regPobList_.IsValid())
    {
        RefreshRegPobList(regPobList_);
    }
}


void RegPobPlugin::OnTimer(int Counter)
{
    if (Counter == 3)
    {
        ShowStartupMessage();
    }

}

bool RegPobPlugin::OnCompileCommand(const char* sCommandLine)
{
    const std::string command = SafeString(sCommandLine);
    if (command.rfind(".regpob", 0) != 0)
    {
        return false;
    }

    if (command == ".regpob resetlist")
    {
        if (regPobList_.IsValid())
        {
            regPobList_.DeleteAllColumns();
            listColumnsConfigured_ = false;
            ConfigureRegPobListColumns();
            RefreshRegPobList(regPobList_);
        }

        DisplayUserMessage(
            "REGPOB",
            "List reset",
            "REGPOB Departures columns were rebuilt with click-to-edit enabled.",
            false,
            false,
            false,
            false,
            false);
        return true;
    }

    DisplayUserMessage(
        "REGPOB",
        "Departure List Setup",
        "Editable REG/POB (same white box as SPAD):\n\n"
        "Standard Departure List -> S (column setup):\n"
        "  REG row: Tag item = REGPOB/REG, Left button = REGPOB/REG\n"
        "  POB row: Tag item = REGPOB/POB, Left button = REGPOB/POB\n\n"
        "The left button MUST be on the same row as the tag item.\n"
        "Without it, the column is display-only.\n\n"
        "Or use the pre-wired 'REGPOB Departures' list.\n"
        "Type .regpob resetlist to rebuild that list if columns were saved wrong.",
        false,
        false,
        false,
        false,
        false);

    return true;
}

