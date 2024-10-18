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

    bool isRunning() const 
    {
        if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL)
            return false;

        DWORD exitCode;
        if (GetExitCodeProcess(processHandle, &exitCode)) {
            return exitCode == STILL_ACTIVE;
        }
        return false;
    }

    void terminate() 
    {
        if (!isRunning())
            return;

        if (!TerminateProcess(processHandle, 0))
            MessageBoxA(0, std::format("Ошибка завершения процесса: {}", GetLastError()).c_str(), 0, 0);
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
	ID3D11ShaderResourceView* texture = NULL;
    Process* prc = nullptr;

    Zapret(int width, int height, std::string name, std::string id_name, bool active, ID3D11ShaderResourceView* texture)
    {
        this->width = width;
        this->height = height;
        this->name = name;
        this->id_name = id_name;
        this->active = active;
        this->texture = texture;
    }

    bool isRunning() const
    {
        if (prc == nullptr)
            return false;

        return prc->isRunning();
    }

    void terminate()
    {
        if (prc == nullptr)
            return;

        prc->terminate();
    }

    void startProcces();
};

namespace vars
{
    json json_settings = {
        {"log", false },
        {"win_start", false},
        {"tray_start", false},
        {"start_version_check", true},
        {"provider", 0},
        {"auto_start", 0},
        {"services",
        {
            {"youtube", false},
            {"discord", false},
            {"7tv", false}
        }}
    };

    int provider = 0;
    int auto_start = 0;
    bool bLog = false;
    bool bWin_start = false;
    bool bTray_start = false;
    bool bStart_v_check = false;

	std::vector<Zapret*> services;
    std::map<int, std::string> providers =
    {
        {0, u8"Другой"},
        {1, u8"Ростелеком"}
    };

    std::map<int, std::string> auto_starts =
    {
        {0, u8"Планировщик задач (с вкл. UAC)"},
        {1, u8"Реестр (с выкл. UAC)"},
    };

	std::string version = "v1.0.3";

    void init()
    {
        json_settings = tools::loadSettings(json_settings);

        provider = json_settings["provider"];
        auto_start = json_settings["auto_start"];
        bLog = json_settings["log"];
        bWin_start = json_settings["win_start"];
        bTray_start = json_settings["tray_start"];
        bStart_v_check = json_settings["start_version_check"];
    }
}

void Zapret::startProcces()
{
    std::string args = "";
    std::string cur_path = std::filesystem::current_path().string();

    if (id_name == "youtube")
    {
        args = std::format("--wf-tcp=80,443 --wf-udp=443 --filter-udp=443 --hostlist=\"{}\\list-youtube.txt\" --dpi-desync=fake --dpi-desync-repeats=11 --dpi-desync-fake-quic=\"{}\\quic_initial_www_google_com.bin\" --new ", cur_path, cur_path);
        args += std::format("--filter-udp=443 --hostlist=\"{}\\list-youtube.txt\" --dpi-desync=fake --dpi-desync-repeats=11 --new ", cur_path);
        args += std::format("--filter-tcp=80 --hostlist=\"{}\\list-youtube.txt\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --new ", cur_path);
        args += std::format("--hostlist=\"{}\\list-youtube.txt\" --dpi-desync=fake,disorder2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --new ", cur_path);

        if (vars::provider == 0)
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\list-youtube.txt\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --dpi-desync-fake-tls=\"{}\\tls_clienthello_www_google_com.bin\"", cur_path, cur_path);
        else
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\list-youtube.txt\" --dpi-desync=fake,split --dpi-desync-autottl=5 --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-fake-tls=\"{}\\tls_clienthello_www_google_com.bin\"", cur_path, cur_path);
    }
    else if (id_name == "discord")
    {
        args = std::format("--wf-tcp=443 --wf-udp=443,50000-50099 --filter-udp=443 --hostlist=\"{}\\list-discord.txt\" --dpi-desync=fake --dpi-desync-udplen-increment=10 --dpi-desync-repeats=6 --dpi-desync-udplen-pattern=0xDEADBEEF --dpi-desync-fake-quic=\"{}\\quic_initial_www_google_com.bin\" --new ", cur_path, cur_path);
        args += std::format("--filter-udp=50000-50099 --ipset=\"{}\\list-discord-ip.txt\" --dpi-desync=fake --dpi-desync-any-protocol --dpi-desync-fake-quic=\"{}\\quic_initial_www_google_com.bin\" --new ", cur_path, cur_path);
        args += std::format("--filter-tcp=443 --hostlist=\"{}\\list-discord.txt\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --dpi-desync-fake-tls=\"{}\\tls_clienthello_www_google_com.bin", cur_path, cur_path);
    }
    else if (id_name == "7tv")
    {
        args = std::format("--wf-tcp=443 --wf-udp=443 --filter-tcp=443 --hostlist=\"{}\\list-7tv.txt\" --dpi-desync=fake,split2 --dpi-desync-ttl=8 --new ", cur_path);
        args += std::format("--filter-udp=443 --hostlist=\"{}\\list-7tv.txt\" --dpi-desync=fake", cur_path);
    }

    prc = new Process(cur_path + "\\winws.exe", args);
}