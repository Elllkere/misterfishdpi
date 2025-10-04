#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <format>

struct ID3D11ShaderResourceView;

namespace vars
{
    extern json json_settings;
}

class Process {
public:

    Process() {}

    Process(const std::string& appName, const std::string& args)
    {
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        std::string command = std::format("\"{}\" {}", appName, args);

        //CREATE_NO_WINDOW
        if (!CreateProcess(NULL, const_cast<char*>(command.c_str()), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) 
        {
            std::string err = ""; 
            DWORD err_code = GetLastError();
            
            if (err_code == 193)
                err = std::format("Failed to start process: {}\n{}\n{}", err_code, appName, command);
            else
                err = std::format("Failed to start process: {}", err_code);

            MessageBoxA(0, err.c_str(), 0, 0);
        }
        else
        {
            processHandle = pi.hProcess;
            processID = pi.dwProcessId;
        }
    }

    ~Process()
    {
        if (processHandle && processHandle != INVALID_HANDLE_VALUE)
            CloseHandle(processHandle);
        processHandle = NULL;
    }

    bool isRunning()
    {
        if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL)
            return false;

        DWORD exitCode = 0;
        if (GetExitCodeProcess(processHandle, &exitCode))
        {
            if (exitCode == STILL_ACTIVE)
                return true;

            CloseHandle(processHandle);
            processHandle = NULL;
            return false;
        }

        CloseHandle(processHandle);
        processHandle = NULL;
        return false;
    }

    void terminate()
    {
        if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL)
            return;

        if (isRunning())
        {
            if (!TerminateProcess(processHandle, 0))
            {
                DWORD err = GetLastError();
                if (err != ERROR_INVALID_HANDLE)
                    MessageBoxA(0, std::format("Failed to stop process: {}", err).c_str(), 0, 0);
            }
        }

        CloseHandle(processHandle);
        processHandle = NULL;
    }

private:
    HANDLE processHandle = NULL;
    DWORD processID = 0;
};

struct ZapretProcessEntry
{
    std::vector<std::string> strategies;
    std::vector<std::string> wf_tcp;
    std::vector<std::string> wf_udp;
};

class ServiceBase
{
public:
    ServiceBase(const std::string& id_name, bool hidden = true);
    ServiceBase(int width, int height, const std::string& name, const std::string& id_name, ID3D11ShaderResourceView* texture);
    virtual ~ServiceBase() = default;

    virtual bool isRunning() = 0;
    virtual void restart() = 0;
    virtual void terminate() = 0;
    virtual void start(const std::string& id_name = "") = 0;

    void toggleActive();

    int width = 0;
    int height = 0;
    std::string name;
    std::string id_name;
    int hotkey = 0;
    bool active = false;
    bool hide = false;
    bool panel_hide = false;
    ID3D11ShaderResourceView* texture = nullptr;
};

class ZapretServiceInfo
{
public:
    std::string shared_id;
    std::map<std::string/*id*/, std::string/*txt*/> shared_ids;
    std::string shared_txt;

    ZapretServiceInfo() {}

    ZapretServiceInfo(const std::string& shared_id, std::map<std::string, std::string> shared_ids, const std::string& shared_txt)
    {
        this->shared_id = shared_id;
        this->shared_ids = shared_ids;
        this->shared_txt = shared_txt;
    }
};

class Zapret : public ServiceBase
{
public:
    Zapret(const std::string& id_name, const std::string& txt);
    Zapret(int width, int height, const std::string& name, const std::string& id_name, ID3D11ShaderResourceView* texture, const std::string& txt);

    static void initializeOrder(const std::vector<Zapret*>& services);

    bool isRunning() override;
    void restart() override;
    void terminate() override;
    void start(const std::string& id_name = "") override;

    void getArgs(const std::string& id_name, std::string& args, const std::string& cur_path);

    std::string txt;
protected:
    static void upsertArgs(const std::string& key, const std::string& args, const std::string& cur_path);
    static void removeArgs(const std::string& key);
    static bool hasActiveEntry(const std::string& key);

private:
    void addPorts(const std::string& input, std::set<int>& port_set);
    std::string portsToString(const std::set<int>& ports);

    static std::unique_ptr<Process> s_process;
    static std::map<std::string, ZapretProcessEntry> s_entries;
    static std::vector<std::string> s_arg_order;
    static std::string s_current_cmdline;
    static std::string s_executable_path;

    static void registerOrder(const std::string& key);
    static void rebuildProcess();
    static void stopProcess();
    static std::string composeCommandLine();
};

class SharedZapret : public Zapret
{
public:
    ZapretServiceInfo* info;

    //for non interactive
    SharedZapret(const std::string& id_name, ZapretServiceInfo* _info) : Zapret(id_name, _info->shared_ids.find(id_name)->second)
    {
        ZapretServiceInfo* info = new ZapretServiceInfo(*_info);
        if (info->shared_ids.find(id_name) != info->shared_ids.end())
            info->shared_ids.erase(id_name);

        this->info = info;
    }

    SharedZapret(int width, int height, const std::string& name, const std::string& id_name, ID3D11ShaderResourceView* texture, ZapretServiceInfo* _info) : Zapret(width, height, name, id_name, texture, _info->shared_ids.find(id_name)->second)
    {
        ZapretServiceInfo* info = new ZapretServiceInfo(*_info);
        if (info->shared_ids.find(id_name) != info->shared_ids.end())
            info->shared_ids.erase(id_name);

        this->info = info;
    }

    bool isRunning() override;
    void restart() override;
    void terminate() override;
    void start(const std::string& id_name = "") override;
};
