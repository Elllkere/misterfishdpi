#pragma once

class Process {
public:

    Process() {}

    Process(const std::string& appName, const std::string& args)
    {
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        std::string command = appName + " " + args;

        //CREATE_NO_WINDOW
        if (!CreateProcess(NULL, const_cast<char*>(command.c_str()), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            MessageBoxA(0, std::format("Ошибка запуска процесса: {}", GetLastError()).c_str(), 0, 0);
        }
        else
        {
            processHandle = pi.hProcess;
            processID = pi.dwProcessId;
        }
    }

    ~Process()
    {
        if (processHandle)
            CloseHandle(processHandle);
    }

    bool isRunning()
    {
        if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL)
            return false;

        try
        {
            DWORD exitCode;
            if (GetExitCodeProcess(processHandle, &exitCode)) {
                return exitCode == STILL_ACTIVE;
            }
        }
        catch (std::exception ex)
        {
            CloseHandle(processHandle);
            processHandle = NULL;

            return false;
        }

        return false;
    }

    void terminate()
    {
        if (!isRunning())
            return;

        if (!TerminateProcess(processHandle, 0))
            MessageBoxA(0, std::format("Ошибка завершения процесса: {}", GetLastError()).c_str(), 0, 0);

        CloseHandle(processHandle);
    }

private:
    HANDLE processHandle = NULL;
    DWORD processID = 0;
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

class Zapret
{
public:
    int width = 0;
    int height = 0;
    std::string name;
    std::string id_name;
    std::string txt;
    int hotkey = 0;
    bool active = false;
    bool hide = false;
    ID3D11ShaderResourceView* texture = NULL;
    Process* prc = nullptr;

    //for non interactive
    Zapret(const std::string& id_name, const std::string& txt)
    {
        this->id_name = id_name;
        this->hide = true;
        this->txt = "lists\\" + txt;
    }

    Zapret(int width, int height, const std::string& name, const std::string& id_name, bool active, int hotkey, ID3D11ShaderResourceView* texture, const std::string& txt)
    {
        this->width = width;
        this->height = height;
        this->name = name;
        this->id_name = id_name;
        this->active = active;
        this->texture = texture;
        this->hotkey = hotkey;
        this->txt = "lists\\" + txt;
    }

    void toggleActive();

    virtual bool isRunning();
    virtual void restart();
    virtual void terminate();
    virtual void start(const std::string& id_name = "");

    void getArgs(const std::string& id_name, std::string& args, const std::string& cur_path);
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

    SharedZapret(int width, int height, const std::string& name, const std::string& id_name, bool active, int hotkey, ID3D11ShaderResourceView* texture, ZapretServiceInfo* _info) : Zapret(width, height, name, id_name, active, hotkey, texture, _info->shared_ids.find(id_name)->second)
    {
        ZapretServiceInfo* info = new ZapretServiceInfo(*_info);
        if (info->shared_ids.find(id_name) != info->shared_ids.end())
            info->shared_ids.erase(id_name);

        this->info = info;
    }

    bool isOnlyOneRunning() const;
    bool isRunning() override;
    void restart() override;
    void terminate() override;
    void start(const std::string& id_name = "") override;
};