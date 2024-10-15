#pragma once
#include <iostream>
#include <fstream>
#include <taskschd.h>
#include <comdef.h>
#include <filesystem>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

#include "json.hpp"

using json = nlohmann::json;

namespace tools
{
    void createTaskSchedulerEntry() 
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr)) 
        {
            MessageBoxA(0, "������ ��� ������������� COM", 0, 0);
            return;
        }

        ITaskService* pService = NULL;
        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
        if (FAILED(hr)) 
        {
            MessageBoxA(0, "�� ������� ������� ��������� TaskScheduler", 0, 0);
            CoUninitialize();
            return;
        }

        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr)) 
        {
            MessageBoxA(0, "�� ������� ������������ � Task Scheduler", 0, 0);
            pService->Release();
            CoUninitialize();
            return;
        }

        ITaskFolder* pRootFolder = NULL;
        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        if (FAILED(hr)) 
        {
            MessageBoxA(0, "�� ������� �������� �������� ����� Task Scheduler", 0, 0);
            pService->Release();
            CoUninitialize();
            return;
        }

        pRootFolder->DeleteTask(_bstr_t(window::window_name), 0);

        ITaskDefinition* pTask = NULL;
        hr = pService->NewTask(0, &pTask);
        pService->Release();

        if (FAILED(hr)) 
        {
            MessageBoxA(0, "�� ������� ������� ����� ������", 0, 0);
            pRootFolder->Release();
            CoUninitialize();
            return;
        }

        IRegistrationInfo* pRegInfo = NULL;
        hr = pTask->get_RegistrationInfo(&pRegInfo);
        if (SUCCEEDED(hr)) 
        {
            pRegInfo->put_Author(_bstr_t("Elllkere"));
            pRegInfo->Release();
        }

        ITriggerCollection* pTriggerCollection = NULL;
        hr = pTask->get_Triggers(&pTriggerCollection);
        if (SUCCEEDED(hr)) 
        {
            ITrigger* pTrigger = NULL;
            hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
            pTrigger->Release();
            pTriggerCollection->Release();
        }

        IActionCollection* pActionCollection = NULL;
        hr = pTask->get_Actions(&pActionCollection);
        if (SUCCEEDED(hr)) 
        {
            IAction* pAction = NULL;
            hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);

            char exePath[MAX_PATH];
            GetModuleFileName(NULL, exePath, MAX_PATH);

            IExecAction* pExecAction = NULL;
            hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
            pExecAction->put_Path(_bstr_t(exePath));
            pExecAction->put_Arguments(_bstr_t("/autostart"));
            pExecAction->Release();
            pAction->Release();
            pActionCollection->Release();
        }

        IPrincipal* pPrincipal = NULL;
        hr = pTask->get_Principal(&pPrincipal);
        if (SUCCEEDED(hr)) 
        {
            pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
            pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
            pPrincipal->Release();
        }

        IRegisteredTask* pRegisteredTask = NULL;
        hr = pRootFolder->RegisterTaskDefinition(_bstr_t(window::window_name), pTask, TASK_CREATE_OR_UPDATE,
            _variant_t(), _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(L""), &pRegisteredTask);

        if (FAILED(hr))
            MessageBoxA(0, std::format("�� ������� ���������������� ������: {}", hr).c_str(), 0, 0);

        pRegisteredTask->Release();
        pTask->Release();
        pRootFolder->Release();

        CoUninitialize();
    }

    void deleteTaskSchedulerEntry()
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr))
        {
            MessageBoxA(0, "������ ��� ������������� COM", 0, 0);
            return;
        }

        ITaskService* pService = NULL;
        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
        if (FAILED(hr))
        {
            MessageBoxA(0, "�� ������� ������� ��������� TaskScheduler", 0, 0);
            CoUninitialize();
            return;
        }

        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr))
        {
            MessageBoxA(0, "�� ������� ������������ � Task Scheduler", 0, 0);
            pService->Release();
            CoUninitialize();
            return;
        }

        ITaskFolder* pRootFolder = NULL;
        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        if (FAILED(hr))
        {
            MessageBoxA(0, "�� ������� �������� �������� ����� Task Scheduler", 0, 0);
            pService->Release();
            CoUninitialize();
            return;
        }

        pRootFolder->DeleteTask(_bstr_t(window::window_name), 0);

        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
    }

    bool existTaskSchedulerEntry()
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr))
        {
            MessageBoxA(0, "������ ��� ������������� COM", 0, 0);
            return false;
        }

        ITaskService* pService = NULL;
        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
        if (FAILED(hr))
        {
            MessageBoxA(0, "�� ������� ������� ��������� TaskScheduler", 0, 0);
            CoUninitialize();
            return false;
        }

        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr))
        {
            MessageBoxA(0, "�� ������� ������������ � Task Scheduler", 0, 0);
            pService->Release();
            CoUninitialize();
            return false;
        }

        ITaskFolder* pRootFolder = NULL;
        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        if (FAILED(hr))
        {
            MessageBoxA(0, "�� ������� �������� �������� ����� Task Scheduler", 0, 0);
            pService->Release();
            CoUninitialize();
            return false;
        }

        IRegisteredTask* pTask = NULL;
        hr = pRootFolder->GetTask(_bstr_t(window::window_name), &pTask);

        bool result = SUCCEEDED(hr);

        if (pTask != nullptr)
            pTask->Release();
        pRootFolder->Release();
        pService->Release();

        CoUninitialize();

        return result;
    }

    void setWorkingDirectoryToExecutablePath() 
    {
        char exePath[MAX_PATH];
        GetModuleFileName(NULL, exePath, MAX_PATH);

        std::filesystem::path path = exePath;
        std::filesystem::current_path(path.parent_path());
    }

    std::vector<const char*> convertMapToCharArray(const std::map<int, std::string>& map)
    {
        std::vector<const char*> items;
        for (const auto& [key, value] : map)
        {
            items.push_back(value.c_str());
        }
        return items;
    }

    void addToStartup() 
    {
        HKEY hKey;

#ifndef _WIN64
        SYSTEM_INFO si;
        GetNativeSystemInfo(&si);

        bool b64 = false;
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
            b64 = true;

        LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, (b64 ? KEY_SET_VALUE | KEY_WOW64_64KEY : KEY_SET_VALUE), &hKey);
#else
        LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
#endif

        if (lRes == ERROR_SUCCESS)
        {
            char path[MAX_PATH];
            GetModuleFileName(NULL, path, MAX_PATH);
            const char* appPath = std::format("\"{}\" /autostart", path).c_str();

            RegSetValueEx(hKey, window::window_name, 0, REG_SZ, (BYTE*)appPath, strlen(appPath) + 1);
            RegCloseKey(hKey);
        }
    }

    void removeFromStartup() 
    {
        HKEY hKey;

#ifndef _WIN64
        SYSTEM_INFO si;
        GetNativeSystemInfo(&si);

        bool b64 = false;
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
            b64 = true;

        LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, (b64 ? KEY_SET_VALUE | KEY_WOW64_64KEY : KEY_SET_VALUE), &hKey);
#else
        LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
#endif

        if (lRes == ERROR_SUCCESS)
        {
            RegDeleteValue(hKey, window::window_name);
            RegCloseKey(hKey);
        }
    }

    bool isInStartup() 
    {
        HKEY hKey;

#ifndef _WIN64
        SYSTEM_INFO si;
        GetNativeSystemInfo(&si);

        bool b64 = false;
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
            b64 = true;

        LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, (b64 ? KEY_QUERY_VALUE | KEY_WOW64_64KEY : KEY_QUERY_VALUE), &hKey);
#else
        LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &hKey);
#endif

        if (lRes == ERROR_SUCCESS)
        {
            DWORD dwType = REG_SZ;
            TCHAR szValue[MAX_PATH];
            DWORD dwSize = sizeof(szValue);

            if (RegQueryValueEx(hKey, window::window_name, NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return true;
            }

            RegCloseKey(hKey);
        }

        return false;
    }

    bool updateSettings(const json& settings)
    {
        std::ofstream ofs(std::filesystem::current_path().string() + "\\settings.json");
        if (ofs.is_open())
        {
            ofs << std::setw(4) << settings << std::endl;
            ofs.close();
        }
        else
            return false;

        return true;
    }

    json loadSettings(const json& n)
    {
        json settings;

        std::ifstream ifs(std::filesystem::current_path().string() + "\\settings.json");
        if (!ifs.is_open())
        {
            updateSettings(n);
            settings = n;
        }
        else
        {
            ifs >> settings;
            ifs.close();
        }

        bool flag_update = false;

        for (auto& [key, value] : n.items())
        {
            if (settings.find(key) == settings.end())
            {
                settings[key] = value;
                flag_update = true;
            }
        }

        if (flag_update)
            updateSettings(settings);

        return settings;
    }
}