#pragma once
#include <iostream>
#include <fstream>
#include <taskschd.h>
#include <comdef.h>
#include <filesystem>
#include <TlHelp32.h>
#include <ShlObj.h>
#include <set>
#include <map>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

#include "json.hpp"

#define CURL_STATICLIB
#include <curl/curl.h>

using json = nlohmann::json;

namespace vars
{
    extern std::string version;
}

namespace tools
{
    class WinToastHandlerExample : public WinToastLib::IWinToastHandler
    {
    public:
        WinToastHandlerExample() {}
        // Public interfaces
        void toastActivated() const override {}
        void toastActivated(int actionIndex) const override {}
        void toastActivated(const char* response) const override {}
        void toastDismissed(WinToastDismissalReason state) const override {}
        void toastFailed() const override {}
    };

    struct MemoryStruct {
        char* memory;
        size_t size;
    };

    static size_t _write_mem_callback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        size_t realsize = size * nmemb;
        struct MemoryStruct* mem = (struct MemoryStruct*)userp;

        char* ptr = (char*)realloc(mem->memory, mem->size + realsize + 1);
        if (!ptr) 
        {
            /* out of memory! */
            printf("not enough memory (realloc returned NULL)\n");
            return 0;
        }

        mem->memory = ptr;
        memcpy(&(mem->memory[mem->size]), contents, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;

        return realsize;
    }

    std::wstring to_wstring(const std::string& str) 
    {
        if (str.empty()) return L"";

        int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
        return wstr;
    }
    
    void sendNotif(const std::string& head, const std::string& body = "", bool messagebox = false, bool messagebox_on_fail = true)
    {
        if (messagebox)
        {
            std::string name = window::window_name;
            MessageBoxW(0, to_wstring(head).c_str(), std::wstring(name.begin(), name.end()).c_str(), MB_OK);
            return;
        }

        using namespace WinToastLib;

        if (!WinToast::isCompatible()) 
        {
            if (messagebox_on_fail)
                sendNotif(head, body, true, false);

            return;
        }

        if (!window::toast_init)
        {
            std::string app = window::window_name;
            WinToast::instance()->setAppName(std::wstring(app.begin(), app.end()));
            const auto aumi = WinToast::configureAUMI(L"elllkere", L"misterfishdpi", std::wstring(), std::wstring(vars::version.begin(), vars::version.end()));
            WinToast::instance()->setAppUserModelId(aumi);

            if (!WinToast::instance()->initialize())
            {
                if (messagebox_on_fail)
                    sendNotif(head, body, true, false);

                return;
            }

            window::toast_init = true;
        }

        WinToastTemplate templ = WinToastTemplate(WinToastTemplate::Text02);
        templ.setTextField(to_wstring(head), WinToastTemplate::FirstLine);
        if (!body.empty())
            templ.setTextField(to_wstring(body), WinToastTemplate::SecondLine);

        WinToast::WinToastError error;
        const auto toast_id = WinToast::instance()->showToast(templ, new WinToastHandlerExample(), &error);
        if (toast_id < 0) 
        {
            if (messagebox_on_fail)
                sendNotif(head, body, true, false);

            MessageBoxA(0, std::format("Error: Could not launch toast notification - {}", (int)error).c_str(), window::window_name, MB_OK);
        }
    }

    std::vector<std::string> split(const std::string& input, const std::string& delimiter)
    {
        std::vector<std::string> tokens;
        size_t startPos = 0;
        size_t endPos;

        while ((endPos = input.find(delimiter, startPos)) != std::string::npos) {
            tokens.push_back(input.substr(startPos, endPos - startPos));
            startPos = endPos + delimiter.length();
        }
        tokens.push_back(input.substr(startPos));

        return tokens;
    }

    bool request(const std::string& url, const std::string& post_data, std::string* answer, bool timeout_check = false)
    {
        bool result = false; 

        CURL* curl;
        CURLcode res;
        struct MemoryStruct chunk;

        chunk.memory = (char*)malloc(1);  /* grown as needed by realloc above */
        chunk.size = 0;    /* no data at this point */

        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        if (curl) 
        {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

            /* send all data to this function  */
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _write_mem_callback);

            /* we pass our 'chunk' struct to the callback function */
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);

            /* some servers do not like requests that are made without a user-agent
               field, so we provide one */
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

            if (post_data.size() > 0)
            {
                const char* postthis = post_data.c_str();

                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postthis);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(postthis));
            }

            /* Set timeout for the entire request */
            if (timeout_check)
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

            /* Perform the request, res gets the return code */
            res = curl_easy_perform(curl);
            /* Check for errors */
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
            }
            else 
            {
                *answer = chunk.size > 0 ? std::string(chunk.memory) : "";
                result = true;
            }

            /* always cleanup */
            curl_easy_cleanup(curl);
        }

        free(chunk.memory);
        curl_global_cleanup();

        return result;
    }

    bool request(const std::string& url, std::string* answer, bool timeout_check = false)
    {
        return request(url, "", answer, timeout_check);
    }

    void getDomains(const std::string& path, std::set<std::string>& domains)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::ofstream createFile(path);
            if (!createFile)
            {
                MessageBoxA(0, std::format("Ошибка запуска процесса: {}", GetLastError()).c_str(), 0, 0);
                return;
            }

            createFile.close();
            file.open(path);

            if (!file.is_open())
            {
                MessageBoxA(0, std::format("Ошибка запуска процесса 2: {}", GetLastError()).c_str(), 0, 0);
                return;
            }
        }

        std::string line;
        while (std::getline(file, line))
            domains.insert(line);

        file.close();
    }

    void killProcess(const std::string& name)
    {
        PROCESSENTRY32   pe32;
        HANDLE         hSnapshot = NULL;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        if (Process32First(hSnapshot, &pe32))
        {
            do
            {
                std::string str = pe32.szExeFile;

                std::transform(str.begin(), str.end(), str.begin(),
                    [](unsigned char c) { return std::tolower(c); });


                if (strcmp(str.c_str(), name.c_str()) == 0)
                {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                    if (hProcess == NULL)
                        continue;

                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }

            } while (Process32Next(hSnapshot, &pe32));
        }

        if (hSnapshot != INVALID_HANDLE_VALUE)
            CloseHandle(hSnapshot);
    }

    int setupConsole()
    {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);

        setlocale(LC_ALL, "Rus");
        SetConsoleOutputCP(1251);
        SetConsoleCP(1251);

        if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE))
        {
            std::cerr << "Ошибка установки обработчика консоли!" << std::endl;
            ::Sleep(3000);
            return 1;
        }

        return 0;
    }

    BOOL WINAPI _tempConsoleHndl(DWORD signal)
    {
        return false;
    }

    void sendStop(const std::string& name)
    {
        PROCESSENTRY32   pe32;
        HANDLE         hSnapshot = NULL;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        DWORD pid = 0;

        if (Process32First(hSnapshot, &pe32))
        {
            do
            {
                std::string str = pe32.szExeFile;

                std::transform(str.begin(), str.end(), str.begin(),
                    [](unsigned char c) { return std::tolower(c); });


                if (strcmp(str.c_str(), name.c_str()) == 0)
                {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                    if (hProcess == NULL)
                        continue;

                    FreeConsole();

                    if (!AttachConsole(pe32.th32ProcessID))
                    {
                        tools::sendNotif(std::format("Неудалось остановить {} 2 {}", name, GetLastError()), "", true);
                        if (vars::console_mode)
                            setupConsole();

                        return;
                    }

                    SetConsoleCtrlHandler(NULL, TRUE);

                    if (!GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0))
                    {
                        tools::sendNotif(std::format("Неудалось остановить {} 3 {}", name, GetLastError()), "", true);
                        FreeConsole();
                        return;
                    }

                    FreeConsole();

                    SetConsoleCtrlHandler(_tempConsoleHndl, TRUE);

                    Sleep(50);

                    if (vars::console_mode)
                        setupConsole();
                    else
                        SetConsoleCtrlHandler(NULL, FALSE);

                    CloseHandle(hProcess);

                    break;
                }

            } while (Process32Next(hSnapshot, &pe32));
        }

        if (hSnapshot != INVALID_HANDLE_VALUE)
            CloseHandle(hSnapshot);
    }

    void killAll()
    {
        PROCESSENTRY32   pe32;
        HANDLE         hSnapshot = NULL;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        if (Process32First(hSnapshot, &pe32))
        {
            do
            {
                std::string str = pe32.szExeFile;

                std::transform(str.begin(), str.end(), str.begin(),
                    [](unsigned char c) { return std::tolower(c); });


                if (strcmp(str.c_str(), "winws.exe") == 0)
                {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                    if (hProcess == NULL)
                        continue;

                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }

            } while (Process32Next(hSnapshot, &pe32));
        }

        if (hSnapshot != INVALID_HANDLE_VALUE)
            CloseHandle(hSnapshot);
    }

    bool createUserStartup()
    {
        bool result = true;

        CoInitialize(NULL);
        IShellLink* pShellLink = NULL;
        HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&pShellLink);
        if (SUCCEEDED(hres))
        {
            char startupPath[MAX_PATH];
            ZeroMemory(startupPath, MAX_PATH);

            if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_STARTUP, NULL, 0, startupPath)))
            {
                std::string targetPath = std::filesystem::current_path().string() + "/MisterFish.exe";
                std::string shortcutPath = startupPath + std::string("\\MisterFishDPI.lnk");
                pShellLink->SetPath(targetPath.c_str());

                IPersistFile* pPersistFile;
                hres = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
                if (SUCCEEDED(hres))
                {
                    hres = pPersistFile->Save(_bstr_t(shortcutPath.c_str()), TRUE);
                    pPersistFile->Release();
                }
                else result = false;
            }
            else result = false;

            pShellLink->Release();
        }
        else result = false;

        CoUninitialize();

        return result;
    }

    bool existUserStartup()
    {
        char startupPath[MAX_PATH];
        ZeroMemory(startupPath, MAX_PATH);

        if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_STARTUP, NULL, 0, startupPath)))
            return std::filesystem::exists(startupPath + std::string("\\MisterFishDPI.lnk"));
        else return false;
    }

    void deleteUserStartup()
    {
        char startupPath[MAX_PATH];
        ZeroMemory(startupPath, MAX_PATH);

        if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_STARTUP, NULL, 0, startupPath)))
        {
            std::string shortcutPath = startupPath + std::string("\\MisterFishDPI.lnk");

            remove(shortcutPath.c_str());
        }
    }

    void createTaskSchedulerEntry() 
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr)) 
        {
            MessageBoxA(0, "Ошибка при инициализации COM", 0, 0);
            return;
        }

        ITaskService* pService = NULL;
        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
        if (FAILED(hr)) 
        {
            MessageBoxA(0, "Не удалось создать экземпляр TaskScheduler", 0, 0);
            CoUninitialize();
            return;
        }

        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr)) 
        {
            MessageBoxA(0, "Не удалось подключиться к Task Scheduler", 0, 0);
            pService->Release();
            CoUninitialize();
            return;
        }

        ITaskFolder* pRootFolder = NULL;
        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        if (FAILED(hr)) 
        {
            MessageBoxA(0, "Не удалось получить корневую папку Task Scheduler", 0, 0);
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
            MessageBoxA(0, "Не удалось создать новую задачу", 0, 0);
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
            pExecAction->put_Arguments(_bstr_t("-autostart"));
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

        ITaskSettings* pSettings = NULL;
        hr = pTask->get_Settings(&pSettings);
        if (SUCCEEDED(hr))
        {
            pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
            pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
            pSettings->Release();
        }

        IRegisteredTask* pRegisteredTask = NULL;
        hr = pRootFolder->RegisterTaskDefinition(_bstr_t(window::window_name), pTask, TASK_CREATE_OR_UPDATE,
            _variant_t(), _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(L""), &pRegisteredTask);

        if (FAILED(hr))
            MessageBoxA(0, std::format("Не удалось зарегистрировать задачу: {}", hr).c_str(), 0, 0);

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
            MessageBoxA(0, "Ошибка при инициализации COM", 0, 0);
            return;
        }

        ITaskService* pService = NULL;
        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
        if (FAILED(hr))
        {
            MessageBoxA(0, "Не удалось создать экземпляр TaskScheduler", 0, 0);
            CoUninitialize();
            return;
        }

        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr))
        {
            MessageBoxA(0, "Не удалось подключиться к Task Scheduler", 0, 0);
            pService->Release();
            CoUninitialize();
            return;
        }

        ITaskFolder* pRootFolder = NULL;
        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        if (FAILED(hr))
        {
            MessageBoxA(0, "Не удалось получить корневую папку Task Scheduler", 0, 0);
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
            MessageBoxA(0, "Ошибка при инициализации COM", 0, 0);
            return false;
        }

        ITaskService* pService = NULL;
        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
        if (FAILED(hr))
        {
            MessageBoxA(0, "Не удалось создать экземпляр TaskScheduler", 0, 0);
            CoUninitialize();
            return false;
        }

        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr))
        {
            MessageBoxA(0, "Не удалось подключиться к Task Scheduler", 0, 0);
            pService->Release();
            CoUninitialize();
            return false;
        }

        ITaskFolder* pRootFolder = NULL;
        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        if (FAILED(hr))
        {
            MessageBoxA(0, "Не удалось получить корневую папку Task Scheduler", 0, 0);
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
            const char* appPath = std::format("\"{}\" -autostart", path).c_str();

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

    void resetKeysQueue()
    {
        for (int key = 0x03; key < 0xFE; ++key)
            GetAsyncKeyState(key);
    }

    std::string getKeyName(int key)
    {
        if (key == 0)
            return "None";

        char name[128];
        if (GetKeyNameText(MapVirtualKey(key, MAPVK_VK_TO_VSC) << 16, name, sizeof(name)) > 0) 
        {
            return std::string(name);
        }

        return "Unknown";
    }

    bool updateSettings(const json& settings, const std::string& name)
    {
        std::ofstream ofs(std::filesystem::current_path().string() + "\\" + name);
        if (ofs.is_open())
        {
            ofs << std::setw(4) << settings << std::endl;
            ofs.close();
        }
        else
            return false;

        return true;
    }

    bool recursiveSeachSettings(json& loaded_settings, const json& base_settings, bool check_old = false)
    {
        bool flag_update = false;

        if (!check_old)
        {
            std::vector<std::string> keys;
            for (const auto& [key, value] : base_settings.items()) 
            {
                keys.push_back(key);
            }

            for (const auto& key : keys)
            {
                const auto& value = base_settings[key];
                if (loaded_settings.find(key) == loaded_settings.end() || loaded_settings[key].type_name() != value.type_name())
                {
                    loaded_settings[key] = value;
                    flag_update = true;
                }
                else if (value.is_object() && loaded_settings[key].is_object())
                {
                    if (recursiveSeachSettings(loaded_settings[key], value, check_old))
                        flag_update = true;
                }
                else if (value.is_array() && loaded_settings[key].is_array()) 
                {
                    if (value.size() != loaded_settings[key].size()) 
                    {
                        loaded_settings[key] = value;
                        flag_update = true;
                    }
                    else 
                    {
                        for (size_t i = 0; i < value.size(); ++i) 
                        {
                            if (value[i].is_object() && loaded_settings[key][i].is_object()) 
                            {
                                if (recursiveSeachSettings(loaded_settings[key][i], value[i], check_old))
                                    flag_update = true;
                            }
                            else if (value[i].is_array() && loaded_settings[key][i].is_array()) 
                            {
                                if (recursiveSeachSettings(loaded_settings[key][i], value[i], check_old))
                                    flag_update = true;
                            }
                            else if (value[i] != loaded_settings[key][i]) 
                            {
                                loaded_settings[key][i] = value[i];
                                flag_update = true;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            std::vector<std::string> keys_to_erase;

            auto items = loaded_settings.items();
            for (const auto& [key, value] : items)
            {
                if (base_settings.find(key) == base_settings.end())
                {
                    keys_to_erase.push_back(key);
                    flag_update = true;
                }
                else if (value.is_object() && base_settings[key].is_object())
                {
                    if (recursiveSeachSettings(value, base_settings[key], check_old))
                        flag_update = true;
                }
                else if (value.is_array() && base_settings[key].is_array())
                {
                    if (value.size() != base_settings[key].size()) 
                    {
                        loaded_settings[key] = base_settings[key];
                        flag_update = true;
                    }
                    else {
                        for (size_t i = 0; i < value.size(); ++i) 
                        {
                            if (value[i].is_object() && base_settings[key][i].is_object()) 
                            {
                                if (recursiveSeachSettings(loaded_settings[key][i], base_settings[key][i], check_old))
                                    flag_update = true;
                            }
                            else if (value[i].is_array() && base_settings[key][i].is_array()) 
                            {
                                if (recursiveSeachSettings(loaded_settings[key][i], base_settings[key][i], check_old)) 
                                    flag_update = true;
                            }
                            else if (value[i] != base_settings[key][i]) 
                            {
                                loaded_settings[key][i] = base_settings[key][i];
                                flag_update = true;
                            }
                        }
                    }
                }
            }

            for (const auto& key : keys_to_erase) 
            {
                loaded_settings.erase(key);
            }
        }

        return flag_update;
    }

    json loadSettings(const json& default_settings, const std::string& name)
    {
        json settings;

        std::ifstream ifs(std::filesystem::current_path().string() + "\\" + name);
        if (!ifs.is_open())
        {
            updateSettings(default_settings, name);
            settings = default_settings;
        }
        else
        {
            ifs >> settings;
            ifs.close();
        }


        bool bNew = recursiveSeachSettings(settings, default_settings);
        bool bOld = recursiveSeachSettings(settings, default_settings, true);
        if (bNew || bOld)
            updateSettings(settings, name);

        return settings;
    }
}