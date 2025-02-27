#pragma once

typedef enum _providers_list
{
    PROVIDER_OTHER          = 0,
    PROVIDER_ROST           = 1,
    PROVIDER_MTS            = 2,
    PROVIDER_LOVIT          = 3
} providers_list;

namespace vars
{
    json json_settings = {
        {"first_note", false},
        {"win_start", false},
        {"tray_start", false},
        {"start_version_check", true},
        {"update_notify", true},
        {"auto_update", true},
        {"unlock_ech", true},
        {"hotkeys", false},
        {"show_hide", false},
        {"provider", 0},
        {"auto_start", 0},
        {"x_method", 0},
        {"notif", 0},
        {"start_delay", 5},
        {"services",
        {
            {"youtube",
            {
                {"active", false},
                {"hide", false},
                {"hotkey", 0}
            }},
        
            {"discord",
            {
                {"active", false},
                {"hide", false},
                {"hotkey", 0}
            }},
        
            {"7tv",
            {
                {"active", false},
                {"hide", false},
                {"hotkey", 0}
            }},
        
            {"proton",
            {
                {"active", false},
                {"hide", false},
                {"hotkey", 0}
            }},
        
            {"pornhub",
            {
                {"active", false},
                {"hide", false},
                {"hotkey", 0}
            }},
        
            {"patreon",
            {
                {"active", false},
                {"hide", false},
                {"hotkey", 0}
            }},
        
            {"tempmail",
            {
                {"active", false},
                {"hide", false},
                {"hotkey", 0}
            }},

            {"thatpervert",
            {
                {"active", false},
                {"hide", false},
                {"hotkey", 0}
            }},

            {"bestchange",
            {
                {"active", false},
                {"hide", false},
                {"hotkey", 0}
            }},

            {"custom",
            {
                {"active", false},
                {"hide", false},
                {"hotkey", 0}
            }}
        }}
    };

    int provider = 0;
    int auto_start = 0;
    int x_method = 0;
    int notif = 0;
    //sec
    int start_delay = 5;
    bool bFirst_note = false;
    bool bWin_start = false;
    bool bTray_start = false;
    bool bStart_ver_check = false;
    bool bNotify_changes = false;
    bool bAuto_update = false;
    bool bUnlock_ech = true;
    bool bHotkeys = true;
    bool bShow_hide = true;

    std::vector<Zapret*> services;
    std::map<int, std::string> providers =
    {
        {providers_list::PROVIDER_OTHER, u8"Другой"},
        {providers_list::PROVIDER_ROST, u8"Ростелеком"},
        {providers_list::PROVIDER_MTS, u8"МТС"},
        {providers_list::PROVIDER_LOVIT, "Lovit"}
    };

    std::map<int, std::string> auto_starts =
    {
        {0, u8"Планировщик задач (с вкл. UAC)"},
        {1, u8"Реестр (с выкл. UAC)"},
        {2, u8"Пользовательский"},
    };

    std::map<int, std::string> x_methods =
    {
        {0, u8"Закрыть"},
        {1, u8"Свернуть"},
    };
    
    std::map<int, std::string> notifs =
    {
        {0, u8"Windows уведомления"},
        {1, u8"Messagebox"},
    };

    std::string version = "v25.0223.1755";

    void init()
    {
        json_settings = tools::loadSettings(json_settings);

        provider = json_settings["provider"];
        auto_start = json_settings["auto_start"];
        x_method = json_settings["x_method"];
        notif = json_settings["notif"];
        start_delay = json_settings["start_delay"];

        bFirst_note = json_settings["first_note"];
        bWin_start = json_settings["win_start"];
        bTray_start = json_settings["tray_start"];

        bStart_ver_check = json_settings["start_version_check"];
        if (!bStart_ver_check)
        {
            bAuto_update = json_settings["auto_update"] = false;
            bNotify_changes = json_settings["update_notify"] = false;
        }
        else
        {
            bAuto_update = json_settings["auto_update"];
            bNotify_changes = json_settings["update_notify"];
        }

        bUnlock_ech = json_settings["unlock_ech"];
        bHotkeys = json_settings["hotkeys"];
        bShow_hide = json_settings["show_hide"];
    }
}
