#pragma once

namespace vars
{
    json json_settings = {
        {"win_start", false},
        {"tray_start", false},
        {"start_version_check", true},
        {"unlock_ech", true},
        {"hotkeys", false},
        {"provider", 0},
        {"auto_start", 0},
        {"x_method", 0},
        {"services",
        {
            {"youtube",
            {
                {"active", false},
                {"hotkey", 0}
            }},
        
            {"discord",
            {
                {"active", false},
                {"hotkey", 0}
            }},
        
            {"7tv",
            {
                {"active", false},
                {"hotkey", 0}
            }},
        
            {"proton",
            {
                {"active", false},
                {"hotkey", 0}
            }},
        
            {"pornhub",
            {
                {"active", false},
                {"hotkey", 0}
            }}
        }}
    };

    json services_txt = {
        {"shared_youtube_service", "list-youtube-service.txt"},
        {"youtube", "list-youtube.txt"},
        {"cf-ech", "list-cf-ech.txt"},
        {"discord", "list-discord.txt|list-discord-ip.txt"},
        {"7tv", "list-7tv.txt"},
        {"proton", "list-proton.txt"},
        {"pornhub", "list-ph.txt"},
    };

    int provider = 0;
    int auto_start = 0;
    int x_method = 0;
    bool bWin_start = false;
    bool bTray_start = false;
    bool bStart_v_check = false;
    bool bUnlock_ech = true;
    bool bHotkeys = true;

    std::vector<Zapret*> services;
    std::map<int, std::string> providers =
    {
        {0, u8"������"},
        {1, u8"����������"},
        {2, u8"���"}
    };

    std::map<int, std::string> auto_starts =
    {
        {0, u8"����������� ����� (� ���. UAC)"},
        {1, u8"������ (� ����. UAC)"},
    };

    std::map<int, std::string> x_methods =
    {
        {0, u8"�������"},
        {1, u8"��������"},
    };

    std::string version = "v1.10.0";

    void init()
    {
        json_settings = tools::loadSettings(json_settings);

        provider = json_settings["provider"];
        auto_start = json_settings["auto_start"];
        x_method = json_settings["x_method"];
        bWin_start = json_settings["win_start"];
        bTray_start = json_settings["tray_start"];
        bStart_v_check = json_settings["start_version_check"];
        bUnlock_ech = json_settings["unlock_ech"];
        bHotkeys = json_settings["hotkeys"];
    }
}
