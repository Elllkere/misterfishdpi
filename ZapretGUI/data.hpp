#pragma once

namespace vars
{
    json json_settings = {
        {"log", false },
        {"win_start", false},
        {"tray_start", false},
        {"start_version_check", true},
        {"unlock_ech", true},
        {"provider", 0},
        {"auto_start", 0},
        {"x_method", 0},
        {"services",
        {
            {"youtube", false},
            {"discord", false},
            {"7tv", false}
        }}
    };

    json services_txt = {
        {"shared_youtube_service", "list-youtube-service.txt"},
        {"youtube", "list-youtube.txt"},
        {"cf-ech", "list-cf-ech.txt"},
        {"discord", "list-discord.txt|list-discord-ip.txt"},
        {"7tv", "list-7tv.txt"},
    };

    int provider = 0;
    int auto_start = 0;
    int x_method = 0;
    bool bLog = false;
    bool bWin_start = false;
    bool bTray_start = false;
    bool bStart_v_check = false;
    bool bUnlock_ech = true;

    std::vector<Zapret*> services;
    std::map<int, std::string> providers =
    {
        {0, u8"Другой"},
        {1, u8"Ростелеком"},
        {2, u8"МТС"}
    };

    std::map<int, std::string> auto_starts =
    {
        {0, u8"Планировщик задач (с вкл. UAC)"},
        {1, u8"Реестр (с выкл. UAC)"},
    };

    std::map<int, std::string> x_methods =
    {
        {0, u8"Закрыть"},
        {1, u8"Свернуть"},
    };

    std::string version = "v1.8.0";

    void init()
    {
        json_settings = tools::loadSettings(json_settings);

        provider = json_settings["provider"];
        auto_start = json_settings["auto_start"];
        x_method = json_settings["x_method"];
        bLog = json_settings["log"];
        bWin_start = json_settings["win_start"];
        bTray_start = json_settings["tray_start"];
        bStart_v_check = json_settings["start_version_check"];
        bUnlock_ech = json_settings["unlock_ech"];
    }
}
