#include "pch.h"
#include "RegPobPlugin.h"

static RegPobPlugin* g_plugin = nullptr;

void __declspec(dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn** ppPlugInInstance)
{
    *ppPlugInInstance = g_plugin = new RegPobPlugin();
}

void __declspec(dllexport) EuroScopePlugInExit(void)
{
    delete g_plugin;
    g_plugin = nullptr;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    (void)hModule;
    (void)reserved;
    return reason == DLL_PROCESS_ATTACH || reason == DLL_THREAD_ATTACH
        || reason == DLL_THREAD_DETACH || reason == DLL_PROCESS_DETACH;
}
