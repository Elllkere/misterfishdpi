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

class Zapret
{
public:
    int width = 0;
    int height = 0;
    std::string name;
    std::string id_name;
    bool active = false;
    bool hide = false;
    ID3D11ShaderResourceView* texture = NULL;
    Process* prc = nullptr;

    //for non interactive
    Zapret(const std::string& id_name)
    {
        this->id_name = id_name;
        this->hide = true;
    }

    Zapret(int width, int height, const std::string& name, const std::string& id_name, bool active, ID3D11ShaderResourceView* texture)
    {
        this->width = width;
        this->height = height;
        this->name = name;
        this->id_name = id_name;
        this->active = active;
        this->texture = texture;
    }

    virtual bool isRunning();
    virtual void terminate();
    virtual void startProcces(const std::string& id_name = "");

    void getArgs(const std::string& id_name, std::string& args, const std::string& cur_path, bool hostlist = false);
};

class SharedZapret : public Zapret
{
public:
    std::vector<std::string> shared_with;
    std::string shared_id;

    //for non interactive
    SharedZapret(const std::string& id_name, std::vector<std::string> shared_with, const std::string& shared_id) : Zapret(id_name)
    {
        auto it = std::find(shared_with.begin(), shared_with.end(), id_name);
        if (it != shared_with.end())
            shared_with.erase(it);

        this->shared_with = shared_with;
        this->shared_id = shared_id;
        this->hide = true;
    }

    SharedZapret(int width, int height, const std::string& name, const std::string& id_name, bool active, ID3D11ShaderResourceView* texture, std::vector<std::string> shared_with, const std::string& shared_id) : Zapret(width, height, name, id_name, active, texture)
    {
        auto it = std::find(shared_with.begin(), shared_with.end(), id_name);
        if (it != shared_with.end())
            shared_with.erase(it);

        this->shared_with = shared_with;
        this->shared_id = shared_id;
    }

    bool isOnlyOneRunning() const;
    bool isRunning() override;
    void terminate() override;
    void startProcces(const std::string& id_name = "") override;
};