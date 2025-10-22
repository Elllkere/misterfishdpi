#pragma once

class Zapret;
class Singbox;
class ServiceBase;

typedef enum _providers_list
{
    PROVIDER_OTHER          = 0,
    PROVIDER_ROST           = 1,
    PROVIDER_MTS            = 2,
    PROVIDER_LOVIT          = 3,
    MAX
} providers_list;

namespace vars
{
    std::string json_setting_name = "settings.json";
    std::string json_singbox_name = "singbox.json";

    int provider = 0;
    int auto_start = 0;
    int x_method = 0;
    int notif = 0;
    int amazon_type = 2;
    int start_delay = 5; //sec

    bool bFirst_note = false;
    bool bWin_start = false;
    bool bTray_start = false;
    bool bStart_ver_check = false;
    bool bNotify_changes = false;
    bool bAuto_update = false;
    bool bUnlock_cf = false;
    bool bUnlock_akamai = false;
    bool bUnlock_amazon = false;
    bool bHotkeys = false;
    bool bShow_hide = false;
    bool console_mode = false;

    json json_settings = {
        {"first_note", bFirst_note},
        {"win_start", bWin_start},
        {"tray_start", bTray_start},
        {"start_version_check", bStart_ver_check},
        {"update_notify", bNotify_changes},
        {"auto_update", bAuto_update},
        {"unlock_cf", bUnlock_cf},
        {"unlock_akamai", bUnlock_akamai},
        {"unlock_amazon", bUnlock_amazon},
        {"hotkeys", bHotkeys},
        {"show_hide", bShow_hide},
        {"provider", provider},
        {"auto_start", auto_start},
        {"x_method", x_method},
        {"amazon_type", amazon_type},
        {"notif", 0},
        {"start_delay", start_delay},
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

            {"twitch",
            {
                {"active", false},
                {"hide", false},
                {"hotkey", 0}
            }},

            {"spotify",
            {
                {"active", false},
                {"hide", false},
                {"hotkey", 0}
            }},

            {"chatgpt",
            {
                {"active", false},
                {"hide", false},
                {"hotkey", 0}
            }},

            {"gemini",
            {
                {"active", false},
                {"hide", false},
                {"hotkey", 0}
            }},

            {"grok",
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

    json json_singbox =
    {
        {"log", { {"disabled", true}}},
        {"dns",
        {
            {"servers", json::array({
                {
                    {"tag", "local"},
                    {"address", "local"},
                    {"detour", "direct"}
                }
            })},
            {"rules", json::array({
                {
                    {"server", "local"},
                    {"outbound", "any"}
                }
            })},
            {"final", "local"}
        }
        },
        {"inbounds", json::array(
            {
                {
                    {"type", "mixed"},
                    {"tag", "socks"},
                    {"listen", "127.0.0.1"},
                    {"listen_port", 10808},
                    {"set_system_proxy", true}
                }
            })
        },
        {"outbounds", json::array(
            {
                {
                    {"type", "http"},
                    {"tag", "proxy"},
                    {"server", ""},
                    {"server_port", 0},
                    {"username", ""},
                    {"password", ""}
                },
                {
                    {"type", "direct"},
                    {"tag", "direct"},
                }
            })
        },
        {"route",
        {
            {"rules", json::array({
                {
                    {"action", "sniff"}
                },
                {
                    {"protocol", "dns"},
                    {"action", "hijack-dns"}
                }
            })},
            {"final", "direct"}
        }
        }
    };

    std::string proxy_user = "";
    std::string proxy_password = "";
    std::string proxy_ip = "";
    std::string proxy_port = "";

    std::vector<Zapret*> services;
    std::vector<Singbox*> singbox_services;
    std::vector<ServiceBase*> render_services;
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

    std::map<int, std::string> amazon_types =
    {
        {0, u8"Вся сеть"},
        {1, u8"Порты игр + сайты"},
        {2, u8"Только сайты"}
    };

    std::string version = "v25.1022.2316";

    void init()
    {
        json_settings = tools::loadSettings(json_settings, json_setting_name);
        json_singbox = tools::loadSettings(json_singbox, json_singbox_name);
        if (json_settings["provider"] >= providers_list::MAX)
            json_settings["provider"] = 0;

        bool updade_singbox = false;
        for (auto& outbound : json_singbox["outbounds"]) 
        {
            if (outbound.contains("type") && outbound["type"].get<std::string>() == "http") 
            {
                if (outbound.contains("username"))
                {
                    vars::proxy_user = outbound["username"].get<std::string>();
                    if (vars::proxy_user == "")
                    {
                        updade_singbox = true;
                        outbound.erase("username");
                    }
                }

                if (outbound.contains("password"))
                {
                    vars::proxy_password = outbound["password"].get<std::string>();
                    if (vars::proxy_password == "")
                    {
                        updade_singbox = true;
                        outbound.erase("password");
                    }
                }
                
                if (outbound.contains("server"))
                    vars::proxy_ip = outbound["server"].get<std::string>();
                
                if (outbound.contains("server_port"))
                    vars::proxy_port = std::to_string(outbound["server_port"].get<int>());
            }
        }

        if (updade_singbox)
            tools::updateSettings(json_singbox, json_singbox_name);

        provider = json_settings["provider"];
        auto_start = json_settings["auto_start"];
        x_method = json_settings["x_method"];
        amazon_type = json_settings["amazon_type"];
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

        bUnlock_cf = json_settings["unlock_cf"];
        bUnlock_akamai = json_settings["unlock_akamai"];
        bUnlock_amazon = json_settings["unlock_amazon"];
        bHotkeys = json_settings["hotkeys"];
        bShow_hide = json_settings["show_hide"];
    }
}
