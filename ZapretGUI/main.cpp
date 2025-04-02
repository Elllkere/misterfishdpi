#include <Windows.h>
#include <iostream>
#include <string>
#include <dwmapi.h> 
#include <map>
#include <set>
#include <vector>

#include <minizip/unzip.h>

#include <wintoastlib.h>

#include <wininet.h>
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "Wininet.lib")

#include "resource.h"

#include "imgui/imgui_impl.hpp"
#include "fonts/sf_pro_display_medium.h"

#include "tools/tools.hpp"
#include "tools/json.hpp"

#include "zapret/zapret.hpp"
#include "data.hpp"
#include "zapret/zapret_imp.hpp"

#include "icons/youtube.hpp"
#include "icons/discord.hpp"
#include "icons/7tv.hpp"
#include "icons/ph.hpp"
#include "icons/proton.hpp"
#include "icons/patreon.hpp"
#include "icons/tempmail.hpp"
#include "icons/thatpervert.hpp"
#include "icons/avatar.hpp"

int page = 0;

bool extractFile(const std::string& zipFilePath, const std::string& outputDir) 
{
    bool result = true;

    unzFile zipFile = unzOpen(zipFilePath.c_str());
    if (zipFile == nullptr) 
    {
        tools::sendNotif(u8"Не удалось открыть архив", "", true);
        return false;
    }

    if (unzGoToFirstFile(zipFile) != UNZ_OK) 
    {
        tools::sendNotif(u8"Не удалось найти первый файл", "", true);
        unzClose(zipFile);
        return false;
    }

    do {
        char fileName[256];
        unz_file_info fileInfo;

        if (unzGetCurrentFileInfo(zipFile, &fileInfo, fileName, sizeof(fileName), nullptr, 0, nullptr, 0) != UNZ_OK) 
        {
            tools::sendNotif(u8"Не удалось получить файл", "", true);

            result = false;
            break;
        }

        if (unzOpenCurrentFile(zipFile) != UNZ_OK) 
        {
            tools::sendNotif(u8"Не удалось открыть файл", "", true);

            result = false;
            break;
        }

        std::vector<char> buffer(fileInfo.uncompressed_size);
        int bytesRead = unzReadCurrentFile(zipFile, buffer.data(), buffer.size());

        if (bytesRead < 0) 
        {
            tools::sendNotif(u8"Не удалось прочитать файл", "", true);
            unzCloseCurrentFile(zipFile);

            result = false;
            break;
        }

        std::string outputPath = outputDir + "/" + fileName;
        if (fileName[strlen(fileName) - 1] == '/')
        {
            std::filesystem::create_directory(outputPath);
        }
        else
        {
            std::ofstream outputFile(outputPath, std::ios::binary);

            if (!outputFile.is_open())
            {
                tools::sendNotif(u8"Не удалось разархивировать файл", "", true);
                unzCloseCurrentFile(zipFile);

                result = false;
                break;
            }

            outputFile.write(buffer.data(), bytesRead);
            outputFile.close();
        }

        unzCloseCurrentFile(zipFile);

    } while (unzGoToNextFile(zipFile) == UNZ_OK);

    unzClose(zipFile);

    return result;
}

bool IsRunAsAdmin() 
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;

    // Создаем SID для группы администраторов
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &adminGroup)) {
        // Проверяем, входит ли текущий процесс в группу администраторов
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    return isAdmin == TRUE;
}

void relaunch(LPSTR lpCmdLine, bool admin = true) 
{
    char exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
    if (admin)
        sei.lpVerb = "runas";
    if (strlen(lpCmdLine) > 0)
        sei.lpParameters = lpCmdLine;
    sei.lpFile = exePath;
    sei.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteEx(&sei))
        MessageBox(NULL, "Не удалось перезапустить приложение с правами администратора.", "Ошибка", MB_OK | MB_ICONERROR);
}

void update(const std::string& version, const LPSTR& lpCmdLine)
{
    std::string path = std::filesystem::current_path().string() + "/new.zip";

    DeleteUrlCacheEntry(std::format("https://github.com/Elllkere/misterfishdpi/releases/download/{}/MisterFish-{}-x64.zip", version, version).c_str());
    HRESULT res = URLDownloadToFile(NULL, std::format("https://github.com/Elllkere/misterfishdpi/releases/download/{}/MisterFish-{}-x64.zip", version, version).c_str(), path.c_str(), 0, NULL);

    if (res == INET_E_DOWNLOAD_FAILURE) 
    {
        tools::sendNotif(u8"Не удалось найти ссылку на новую версию", "", vars::notif != 0);
        return;
    }
    else if (res != S_OK)
    {
        tools::sendNotif(std::format(u8"Не удалось скачать новую версию: {}", res), "", vars::notif != 0);
        return;
    }

    tools::killAll();
    system("sc stop WinDivert");

    rename("MisterFish.exe", "MisterFish.exe.old");
    extractFile(path, std::filesystem::current_path().string());
    remove(path.c_str());
    if (strlen(lpCmdLine) <= 0)
        relaunch((LPSTR)"/waitupdate");
    else
        relaunch((lpCmdLine + std::string(" /waitupdate")).data());

    exit(0);
}

HANDLE hMutexOnce;

void cleanup()
{
    tools::killAll();
    DestroyTrayIcon();
    system("sc stop WinDivert");

    if (hMutexOnce)
    {
        ReleaseMutex(hMutexOnce);
        CloseHandle(hMutexOnce);
    }
}

BOOL WINAPI ConsoleHandler(DWORD signal) 
{
    if (signal == CTRL_CLOSE_EVENT) 
    {
        cleanup();
        return TRUE;
    }

    return FALSE;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    tools::setWorkingDirectoryToExecutablePath();
    vars::init();

#ifndef _DEBUG
    if (!IsRunAsAdmin())
    {
        relaunch(lpCmdLine);
        return 0;
    }
#else
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
#endif

    if (strstr(lpCmdLine, "/waitupdate") || strstr(lpCmdLine, "-waitupdate"))
    {
        Sleep(5 * 1000);
        if (vars::bNotify_changes && vars::notif == 0)
        {
            std::string answer;
            bool result = tools::request("https://elllkere.top/misterfish/update.txt", &answer);

            if (result && !answer.empty() && !strstr(answer.c_str(), "error"))
            {
                answer += u8"\nбольше информации на github";
                tools::sendNotif(std::format(u8"Обновление {}", vars::version), answer, false, false);
            }
        }
    }
    else if (strstr(lpCmdLine, "/autostart") || strstr(lpCmdLine, "-autostart"))
        Sleep(vars::start_delay * 1000);

    std::string old_file = std::filesystem::current_path().string() + "\\MisterFish.exe.old";
    if (std::filesystem::exists(old_file))
        remove(old_file.c_str());

    {
        char mutex_name[128];
        snprintf(mutex_name, sizeof(mutex_name), "Global\\misterfish");

        hMutexOnce = CreateMutexA(NULL, TRUE, mutex_name);
        if (hMutexOnce && GetLastError() == ERROR_ALREADY_EXISTS)
        {
            CloseHandle(hMutexOnce);	hMutexOnce = NULL;
            return 0;
        }
    }

    if (vars::bWin_start)
    {
        if ((vars::auto_start == 1 && !tools::isInStartup()) || (vars::auto_start == 0 && !tools::existTaskSchedulerEntry()))
        {
            vars::json_settings["win_start"] = vars::bWin_start = false;
            vars::json_settings["tray_start"] = vars::bTray_start = false;
            tools::updateSettings(vars::json_settings);
        }
    }

    RECT rc;
    GetWindowRect(GetDesktopWindow(), &rc);
    uint32_t x = rc.right / 2 - window::g_Width / 2;
    uint32_t y = rc.bottom / 2 - window::g_Height / 2;
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    //HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASSEX wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance, LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)), nullptr, nullptr, nullptr, window::class_name, nullptr };
    ::RegisterClassEx(&wc);
    g_hWnd = ::CreateWindowExA(0, wc.lpszClassName, window::window_name, WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU, x, y, window::g_Width, window::g_Height, nullptr, nullptr, wc.hInstance, nullptr);

    RECT rcWindow;
    GetWindowRect(g_hWnd, &rcWindow);

    // Initialize Direct3D
    if (!CreateDeviceD3D(g_hWnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    //idx
    const int DWMWA_WINDOW_CORNER_PREFERENCE = 33;

    enum DWM_WINDOW_CORNER_PREFERENCE {
        DWMWCP_DEFAULT = 0,
        DWMWCP_DONOTROUND = 1,
        DWMWCP_ROUND = 2,
        DWMWCP_ROUNDSMALL = 3
    };

    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    DwmSetWindowAttribute(g_hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::GetIO().IniFilename = nullptr;

    //default
    io.Fonts->AddFontFromMemoryCompressedTTF(sf_pro_display_meduim_data, sf_pro_display_meduim_size, 20.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    for (int i = 1; i <= 30; i++)
        window::font[i] = io.Fonts->AddFontFromMemoryCompressedTTF(sf_pro_display_meduim_data, sf_pro_display_meduim_size, i, NULL, io.Fonts->GetGlyphRangesCyrillic());

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 5.f;
    style.PopupRounding = 5.f;
    style.ChildRounding = 5.f;
    style.ChildBorderSize = 0.f;
    style.WindowBorderSize = 0.f;

    window::top_padding = style.WindowPadding.y;
    window::right_padding = style.WindowPadding.x;

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.12f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.17f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.17f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.17f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.16f, 0.17f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.19f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    int yt_width = 0;
    int yt_height = 0;
    ID3D11ShaderResourceView* youtube_texture = NULL;
    if (!LoadTextureFromMemory(youtube_data, sizeof(youtube_data), &youtube_texture, &yt_width, &yt_height))
    {
        tools::sendNotif(u8"Ошибка загрузки текстуры", "", vars::notif != 0);
        ::DestroyWindow(g_hWnd);
        return 0;
    }

    int ds_width = 0;
    int ds_height = 0;
    ID3D11ShaderResourceView* discord_texture = NULL;
    if (!LoadTextureFromMemory(discord_data, sizeof(discord_data), &discord_texture, &ds_width, &ds_height))
    {
        tools::sendNotif(u8"Ошибка загрузки текстуры", "", vars::notif != 0);
        ::DestroyWindow(g_hWnd);
        return 0;
    }

    int _7tv_width = 0;
    int _7tv_height = 0;
    ID3D11ShaderResourceView* _7tv_texture = NULL;
    if (!LoadTextureFromMemory(_7tv_data, sizeof(_7tv_data), &_7tv_texture, &_7tv_width, &_7tv_height))
    {
        tools::sendNotif(u8"Ошибка загрузки текстуры", "", vars::notif != 0);
        ::DestroyWindow(g_hWnd);
        return 0;
    }

    int proton_width = 0;
    int proton_height = 0;
    ID3D11ShaderResourceView* proton_texture = NULL;
    if (!LoadTextureFromMemory(proton_data, sizeof(proton_data), &proton_texture, &proton_width, &proton_height))
    {
        tools::sendNotif(u8"Ошибка загрузки текстуры", "", vars::notif != 0);
        ::DestroyWindow(g_hWnd);
        return 0;
    }

    int ph_width = 0;
    int ph_height = 0;
    ID3D11ShaderResourceView* ph_texture = NULL;
    if (!LoadTextureFromMemory(ph_data, sizeof(ph_data), &ph_texture, &ph_width, &ph_height))
    {
        tools::sendNotif(u8"Ошибка загрузки текстуры", "", vars::notif != 0);
        ::DestroyWindow(g_hWnd);
        return 0;
    }
    
    int patreon_width = 0;
    int patreon_height = 0;
    ID3D11ShaderResourceView* patreon_texture = NULL;
    if (!LoadTextureFromMemory(patreon_data, sizeof(patreon_data), &patreon_texture, &patreon_width, &patreon_height))
    {
        tools::sendNotif(u8"Ошибка загрузки текстуры", "", vars::notif != 0);
        ::DestroyWindow(g_hWnd);
        return 0;
    }
    
    int tempmail_width = 0;
    int tempmail_height = 0;
    ID3D11ShaderResourceView* tempmail_texture = NULL;
    if (!LoadTextureFromMemory(tempmail_data, sizeof(tempmail_data), &tempmail_texture, &tempmail_width, &tempmail_height))
    {
        tools::sendNotif(u8"Ошибка загрузки текстуры", "", vars::notif != 0);
        ::DestroyWindow(g_hWnd);
        return 0;
    }
    
    int thatpervert_width = 0;
    int thatpervert_height = 0;
    ID3D11ShaderResourceView* thatpervert_texture = NULL;
    if (!LoadTextureFromMemory(thatpervert_data, sizeof(thatpervert_data), &thatpervert_texture, &thatpervert_width, &thatpervert_height))
    {
        tools::sendNotif(u8"Ошибка загрузки текстуры", "", vars::notif != 0);
        ::DestroyWindow(g_hWnd);
        return 0;
    }

    int custom_width = 0;
    int custom_height = 0;
    ID3D11ShaderResourceView* custom_texture = NULL;
    if (!LoadTextureFromMemory(custom_data, sizeof(custom_data), &custom_texture, &custom_width, &custom_height))
    {
        tools::sendNotif(u8"Ошибка загрузки текстуры", "", vars::notif != 0);
        ::DestroyWindow(g_hWnd);
        return 0;
    }

    std::map<std::string, std::string> shared_youtube =
    {
        {"pornhub", "list-ph.txt"},
        {"proton", "list-proton.txt"},
        {"youtube", "list-youtube.txt"},
        {"patreon", "list-patreon.txt"},
        {"custom", "..\\list-custom.txt"}
    };
    
    std::map<std::string, std::string> shared_7tv =
    {
        {"7tv", "list-7tv.txt"},
        {"tempmail", "list-tempmail.txt"},
        {"thatpervert", "list-thatpervert.txt"},
    };

    ZapretServiceInfo* shared_service_youtube = new ZapretServiceInfo{ "shared_service_youtube", shared_youtube, "list-youtube-service.txt" };
    ZapretServiceInfo* shared_service_7tv = new ZapretServiceInfo{ "shared_service_7tv", shared_7tv, "list-7tv-service.txt" };
    Zapret* cf_ech = new Zapret("cf-ech", "list-cf-ech.txt|list-cf-ech-ip.txt");

    vars::services =
    {
        new SharedZapret(yt_width, yt_height, "Youtube", "youtube", youtube_texture, shared_service_youtube),
        new Zapret(ds_width, ds_height, "Discord", "discord", discord_texture, "list-discord.txt|list-discord-ip.txt"),
        new SharedZapret(_7tv_width, _7tv_height, "7tv", "7tv", _7tv_texture, shared_service_7tv),
        new SharedZapret(proton_width, proton_height, u8"Proton (без mail)", "proton", proton_texture, shared_service_youtube),
        new SharedZapret(ph_width, ph_height, "PornHub", "pornhub", ph_texture, shared_service_youtube),
        new SharedZapret(patreon_width, patreon_height, "Patreon", "patreon", patreon_texture, shared_service_youtube),
        new SharedZapret(tempmail_width, tempmail_height, "temp-mail.org", "tempmail", tempmail_texture, shared_service_7tv),
        new SharedZapret(thatpervert_width, thatpervert_height, "thatpervert", "thatpervert", thatpervert_texture, shared_service_7tv),
        new SharedZapret(custom_width, custom_height, u8"свой список", "custom", custom_texture, shared_service_youtube),
        cf_ech,
    };

    delete shared_service_youtube;
    delete shared_service_7tv;

    bool failed_ver_check = false;
    if (vars::bStart_ver_check)
    {
        std::string answer;
        bool result = tools::request("https://elllkere.top/misterfish/version.txt", &answer, true);
        if (!result || (answer.empty() || strstr(answer.c_str(), "error")))
            failed_ver_check = true;
        else if (answer != vars::version)
        {
            if (!vars::bAuto_update)
                tools::sendNotif(u8"Вышла новая версия, скачать можно в настройках", "", vars::notif != 0);
#ifndef _DEBUG
            else
            {
                tools::sendNotif(u8"Вышла новая версия", u8"Программа обновляется в фоне", false, false);
                update(answer, lpCmdLine);
            }
#endif
        }
    }

    bool console = strstr(lpCmdLine, "/console") || strstr(lpCmdLine, "-console");
    bool silent = strstr(lpCmdLine, "/silent") || strstr(lpCmdLine, "-silent");
    bool very_silent = strstr(lpCmdLine, "/verysilent") || strstr(lpCmdLine, "-verysilent");

    if (!failed_ver_check)
    {
        if (silent || very_silent || console)
        {
            vars::json_settings["start_version_check"] = vars::bStart_ver_check = false;
            vars::json_settings["auto_update"] = vars::bAuto_update = false;
            tools::updateSettings(vars::json_settings);
        }
        else if (!((strstr(lpCmdLine, "/autostart") || strstr(lpCmdLine, "-autostart")) && vars::bTray_start == true))
        {
            ::ShowWindow(g_hWnd, nCmdShow);
            ::UpdateWindow(g_hWnd);
        }
    }
    else
    {
        ::ShowWindow(g_hWnd, nCmdShow);
        ::UpdateWindow(g_hWnd);

        tools::sendNotif(u8"Не удалось проверить версию", "", vars::notif != 0);
    }

    if (!very_silent)
        CreateTrayIcon();

    if (console)
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
    }

    tools::killAll();

    if (vars::bUnlock_ech == true)
        cf_ech->active = true;

    for (auto& s : vars::services)
    {
        if (s->active)
        {
            s->start();
            if (console && !s->name.empty())
                printf("запуск %s обхода\n", s->name.c_str());
        }
    }

    // Main loop
    bool quit = false;
    while (!quit)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                quit = true;
        }
        if (quit)
            break;

        if (vars::bHotkeys)
        {
            for (auto& services : vars::services)
            {
                if (services->hotkey != 0)
                    if (GetAsyncKeyState(services->hotkey) & 1)
                        services->toggleActive();
            }
        }

        if (silent || very_silent|| console)
        {
            ::Sleep(10);
            continue;
        }

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(window::g_Width, window::g_Height)); ImGui::SetNextWindowPos(ImVec2(0, 0));
        if (!ImGui::Begin("##begin", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::End();
            return 0;
        }

        //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
        if(ImGui::BeginPopupModal("###note", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove))
        {
            ImGui::SetNextWindowSize(ImGui::GetContentRegionAvail());
            
            std::string text = u8"Это ваш первый запуск программы, это сообщение будет показано только 1 раз.\n";
            text += u8"Пожалуйста посетите раздел настройки и выберите нужного для себя провайдера,\n";
            text += u8"если вашего провайдера в списке нет и с выбранным 'другой' сервисы все равно не работают,\n";
            text += u8"то вы можете попробовать других провайдеров, возможно их настройки подойдут для вашего.\n";
            text += u8"Если с никаким провайдером не работают сервисы тогда можно создать issue на github";

            ImGui::Text(text.c_str());

            ImGui::PushStyleColor(ImGuiCol_Button, ImColor(40, 40, 40).Value);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor(45, 45, 45).Value);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor(45, 45, 45).Value);

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x / 2.f) - (ImGui::CalcTextSize("OK").x / 2));
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
            if (ImGui::Button("OK"))
            {
                vars::json_settings["first_note"] = vars::bFirst_note = true;
                tools::updateSettings(vars::json_settings);
                ImGui::CloseCurrentPopup();
            }

            ImGui::PopStyleColor(3);

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();

        if (!vars::bFirst_note)
            ImGui::OpenPopup("###note");

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImColor(30, 30, 30).Value); ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.f);
        ImGui::BeginChild("##header", ImVec2(ImGui::GetContentRegionAvail().x, window::header_size));
        {
            ImGui::PushFont(window::font[18]);
            ImVec2 text_size = ImGui::CalcTextSize(window::window_name);

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x / 2.f - text_size.x / 2.f);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y / 2.f - text_size.y / 2.f);
            ImGui::Text(window::window_name);

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 20 * 2);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y - window::header_size);

            //ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor(30, 30, 30).Value);
            ImGui::PushStyleColor(ImGuiCol_Button, ImColor(30, 30, 30).Value);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor(35, 35, 35).Value);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor(35, 35, 35).Value);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

            if (ImGui::Button("_", ImVec2(20, window::header_size)))
            {
                ShowWindow(g_hWnd, SW_HIDE);
            }

            ImGui::SameLine();
            if (ImGui::Button("X", ImVec2(20, window::header_size)))
            {
                if (vars::x_method == 0)
                    quit = true;
                else
                {
                    ShowWindow(g_hWnd, SW_HIDE);
                }
            }                

            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();

            ImGui::PopFont();

            ImGui::EndChild();
        }
        ImGui::PopStyleColor(); ImGui::PopStyleVar();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImColor(40, 40, 40).Value);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor(45, 45, 45).Value);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor(45, 45, 45).Value);
        ImGui::BeginChild("##left", ImVec2(150, 0));
        {
            std::vector<std::string> buttons = { u8"Сервисы", u8"Настройки" };
            for (int i = 0; i < buttons.size(); i++)
            {
                if (page == i)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImColor(50, 50, 50).Value);
                else
                    ImGui::PushStyleColor(ImGuiCol_Button, ImColor(40, 40, 40).Value);

                if (ImGui::Button(buttons[i].c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 30)))
                    page = i;

                ImGui::PopStyleColor();
            }

            ImVec2 prev_cursor = ImGui::GetCursorPos();

            ImVec2 text_size = ImGui::CalcTextSize(vars::version.c_str());
            ImGui::SetCursorPosX(prev_cursor.x + ImGui::GetContentRegionAvail().x / 2 - text_size.x / 2);
            ImGui::SetCursorPosY(prev_cursor.y + ImGui::GetContentRegionAvail().y - text_size.y - window::top_padding);
            ImGui::Text(vars::version.c_str());

        }
        ImGui::EndChild();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ChildBg]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_ChildBg]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_ChildBg]);
        ImGui::BeginChild("##right");
        {
            switch (page)
            {
            case 0:
            {
                ImVec2 start = ImGui::GetCursorPos();
                for (int i = 0, showed = 0; i < vars::services.size(); i++)
                {
                    Zapret* service = vars::services[i];
                    if (service->hide)
                        continue;

                    if (service->panel_hide && !vars::bShow_hide)
                        continue;

                    ImVec2 text_scale = ImGui::CalcTextSize(service->name.c_str());

                    if (i > 0)
                    {
                        if (showed == 5)
                        {
                            start.y += service->height + text_scale.y + style.FramePadding.y + 20;
                            showed = 0;
                        }

                        ImGui::SetCursorPosX(start.x + (service->width + style.FramePadding.x + 10) * showed);
                    }

                    ImGui::SetCursorPosY(start.y);

                    ImVec4 tint = service->panel_hide ? ImVec4(1, 1, 1, .15) : ImVec4(1, 1, 1, 1);
                    if (ImGui::ImageButton(service->id_name.c_str(), service->texture, ImVec2(service->width, service->height), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tint))
                    {
                        service->toggleActive();
                    }

                    bool open_key_bind = false;
                    if (ImGui::BeginPopupContextItem((std::string("##pop_") + service->id_name).c_str()))
                    {
                        bool hided = service->panel_hide;
                        if (ImGui::MenuItem(hided ? u8"Показать" : u8"Скрыть"))
                        {
                            service->panel_hide = vars::json_settings["services"][service->id_name]["hide"] = !hided;
                            tools::updateSettings(vars::json_settings);
                        }

                        if (vars::bHotkeys)
                        {
                            if (ImGui::MenuItem(u8"Задать клавишу"))
                            {
                                open_key_bind = true;
                            }

                            ImGui::MenuItem((u8"Клавиша: " + std::string(tools::getKeyName(service->hotkey))).c_str(), 0, false, false);
                        }
                        ImGui::EndPopup();
                    }

                    if (open_key_bind)
                        ImGui::OpenPopup((std::string("##key_") + service->id_name).c_str());;

                    if (ImGui::BeginPopup((std::string("##key_") + service->id_name).c_str(), ImGuiWindowFlags_AlwaysAutoResize))
                    {
                        ImGui::Text(u8"Нажмите любую клавишу...");

                        for (int i = 3; i <= 0xFE; i++)
                        {
                            if (ImGui::IsKeyPressed((ImGuiKey)i) && i != ImGuiKey_MouseLeft && i != ImGuiKey_MouseRight)
                            {
                                vars::json_settings["services"][service->id_name]["hotkey"] = service->hotkey = i;
                                tools::updateSettings(vars::json_settings);

                                ImGui::CloseCurrentPopup();

                                tools::resetKeysQueue();
                            }
                        }

                        if (ImGui::Button(u8"Сбросить"))
                        {
                            vars::json_settings["services"][service->id_name]["hotkey"] = service->hotkey = 0;
                            tools::updateSettings(vars::json_settings);

                            ImGui::CloseCurrentPopup();
                        }

                        if (ImGui::Button(u8"Отмена"))
                            ImGui::CloseCurrentPopup();

                        ImGui::EndPopup();
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        std::string status = service->isRunning() ? u8"Выключить" : u8"Включить";
                        if (service->id_name == "custom")
                            status += u8"\n(URL добавлять в list-custom.txt)";

                        ImGui::SetTooltip(status.c_str());
                    }

                    ImGui::SetCursorPosX((start.x + service->width + style.FramePadding.x + 10) * showed + start.x + style.FramePadding.x + service->width / 2 - text_scale.x / 2);

                    int alpha = service->panel_hide ? 100 : 255;
                    if (service->isRunning())
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 255, 100, alpha));
                    else
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120, 120, 120, alpha));

                    ImGui::Text(service->name.c_str());

                    ImGui::PopStyleColor();
                    showed++;
                }

                break;
            }
            case 1:
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor(40, 40, 40).Value);
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImColor(45, 45, 45).Value);
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImColor(45, 45, 45).Value);
                ImGui::PushStyleColor(ImGuiCol_CheckMark, ImColor(10, 10, 10).Value);
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImColor(10, 10, 10).Value);
                ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImColor(25, 25, 25).Value);

                if (ImGui::Checkbox(u8"Проверка версии при запуске", &vars::bStart_ver_check))
                {
                    vars::json_settings["start_version_check"] = vars::bStart_ver_check;
                    vars::json_settings["auto_update"] = vars::bAuto_update = false;
                    tools::updateSettings(vars::json_settings);
                }
                
                if (ImGui::Checkbox(u8"Уведомлять об изменениях", &vars::bNotify_changes))
                {
                    if (!vars::bStart_ver_check)
                        vars::bNotify_changes = false;

                    vars::json_settings["notify_update"] = vars::bNotify_changes;
                    tools::updateSettings(vars::json_settings);
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(u8"Будет работать только при типе уведомления - Windows уведомления");

                if (ImGui::Checkbox(u8"Автоматическое обновление", &vars::bAuto_update))
                {
                    if (!vars::bStart_ver_check)
                        vars::bAuto_update = false;

                    vars::json_settings["auto_update"] = vars::bAuto_update;
                    tools::updateSettings(vars::json_settings);
                }

                if (ImGui::Checkbox(u8"Автозапуск с windows", &vars::bWin_start))
                {
                    vars::json_settings["win_start"] = vars::bWin_start;
                    vars::json_settings["tray_start"] = vars::bTray_start = false;
                    tools::updateSettings(vars::json_settings);

                    switch (vars::auto_start)
                    {
                    case 0:
                    {
                        if (vars::bWin_start)
                            tools::createTaskSchedulerEntry();
                        else if (tools::existTaskSchedulerEntry())
                            tools::deleteTaskSchedulerEntry();

                        break;
                    }
                    case 1:
                    {
                        if (vars::bWin_start)
                            tools::addToStartup();
                        else if (tools::isInStartup())
                            tools::removeFromStartup();

                        break;
                    }
                    }
                }

                if (ImGui::Checkbox(u8"Автозапуск в свернутом состоянии", &vars::bTray_start))
                {
                    if (!vars::bWin_start)
                        vars::bTray_start = false;

                    vars::json_settings["tray_start"] = vars::bTray_start;
                    tools::updateSettings(vars::json_settings);
                }

                if (ImGui::Checkbox(u8"Разблокировать протокол Cloudflare ECH", &vars::bUnlock_ech))
                {
                    cf_ech->active = vars::bUnlock_ech;
                    if (vars::bUnlock_ech)
                        cf_ech->start();
                    else
                        cf_ech->terminate();

                    vars::json_settings["unlock_ech"] = vars::bUnlock_ech;
                    tools::updateSettings(vars::json_settings);
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(u8"Если Вы не знаете что такое Cloudflare, то не стоит выключать.\nРазблокирует протокол ECH у Cloudflare, который включен всегда на бесплатном тарифе\nиз-за чего много обычных не забаненных сайтов не загружаются");

                if (ImGui::Checkbox(u8"Горячие клавиши", &vars::bHotkeys))
                {
                    vars::json_settings["hotkeys"] = vars::bHotkeys;
                    tools::updateSettings(vars::json_settings);

                    tools::resetKeysQueue();
                }

                if (ImGui::Checkbox(u8"Показать скрытые сервисы", &vars::bShow_hide))
                {
                    vars::json_settings["show_hide"] = vars::bShow_hide;
                    tools::updateSettings(vars::json_settings);
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(u8"Для установки клавиши нужно нажать ПКМ по сервису");

                if (ImGui::Combo(u8"Провайдер", &vars::provider, tools::convertMapToCharArray(vars::providers).data(), vars::providers.size()))
                {
                    vars::json_settings["provider"] = vars::provider;
                    tools::updateSettings(vars::json_settings);

                    for (auto& s : vars::services)
                    {
                        if (s->active)
                        {
                            s->restart();
                        }
                    }
                }

                if (ImGui::Combo(u8"Тип автозапуска", &vars::auto_start, tools::convertMapToCharArray(vars::auto_starts).data(), vars::auto_starts.size()))
                {
                    if (vars::bWin_start)
                    {
                        switch (vars::auto_start)
                        {
                        case 0:
                        {
                            if (tools::isInStartup())
                                tools::removeFromStartup();

                            if (tools::existUserStartup())
                                tools::deleteUserStartup();

                            if (!tools::existTaskSchedulerEntry())
                                tools::createTaskSchedulerEntry();
                            

                            break;
                        }
                        
                        case 1:
                        {
                            if (tools::existUserStartup())
                                tools::deleteUserStartup();

                            if (tools::existTaskSchedulerEntry())
                                tools::deleteTaskSchedulerEntry();

                            if (!tools::isInStartup())
                                tools::addToStartup();

                            break;
                        }

                        case 2:
                        {
                            if (tools::isInStartup())
                                tools::removeFromStartup();

                            if (tools::existTaskSchedulerEntry())
                                tools::deleteTaskSchedulerEntry();

                            if (!tools::existUserStartup())
                                tools::createUserStartup();

                            break;
                        }
                        }
                    }

                    vars::json_settings["auto_start"] = vars::auto_start;
                    tools::updateSettings(vars::json_settings);
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(u8"UAC - контроль учетных записей\n(окошко с подтверждением при запуске программы)\nПользовательский - запускает только когда вход выполнит конкретный пользователь");

                if (ImGui::Combo(u8"Действие при Х", &vars::x_method, tools::convertMapToCharArray(vars::x_methods).data(), vars::x_methods.size()))
                {
                    vars::json_settings["x_method"] = vars::x_method;
                    tools::updateSettings(vars::json_settings);
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(u8"Что делать при нажатии на Х");

                if (ImGui::Combo(u8"Тип уведомлений", &vars::notif, tools::convertMapToCharArray(vars::notifs).data(), vars::notifs.size()))
                {
                    vars::json_settings["notif"] = vars::notif;
                    tools::updateSettings(vars::json_settings);
                }

                ImGui::PushItemWidth(349);

                if (ImGui::SliderInt(u8"Задержка автозапуска", &vars::start_delay, 1, 120))
                {
                    vars::json_settings["start_delay"] = vars::start_delay;
                    tools::updateSettings(vars::json_settings);
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(u8"Задержка перед запуском приложения при включенном автозапуске\n(для того чтобы интернет успел подключится)\nв секундах\nCTRL+клик \\ Tab чтобы активировать ввод");

                ImGui::PopItemWidth();

                ImGui::PushStyleColor(ImGuiCol_Button, ImColor(40, 40, 40).Value);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor(45, 45, 45).Value);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor(45, 45, 45).Value);

                if (ImGui::Button(u8"Завершить все обходы", ImVec2(200, 30)))
                {
                    tools::killAll();

                    bool cf_prev = cf_ech->active;

                    for (auto& service : vars::services)
                        service->active = false;

                    if (cf_prev)
                    {
                        cf_ech->active = true;
                        cf_ech->start();
                    }
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(u8"Завершит все прошлые и нынешние процессы zapret");

                if (ImGui::Button(u8"Проверить версию", ImVec2(200, 30)))
                {
                    std::string answer;
                    bool result = tools::request("https://elllkere.top/misterfish/version.txt", &answer, true);
                    if (!result || (answer.empty() || strstr(answer.c_str(), "error")))
                        tools::sendNotif(u8"Не удалось проверить версию", "", vars::notif != 0);
                    else
                    {
                        if (answer == vars::version)
                            tools::sendNotif(u8"Версия актуальна", "", vars::notif != 0);
                        else
                        {
                            if (vars::bAuto_update)
                            {
                                tools::sendNotif(u8"Вышла новая версия", u8"Программа обновляется в фоне", false, false);
                                update(answer, lpCmdLine);
                            }
                            else
                            {
                                tools::sendNotif(u8"Вышла новая версия", "", false, false);
                                system("start https://github.com/Elllkere/misterfishdpi/releases/latest");
                            }
                        }
                    }
                }
                
                if (ImGui::Button("GitHub", ImVec2(200, 30)))
                    system("start https://github.com/Elllkere/misterfishdpi/");

                ImGui::PopStyleColor(3);
                ImGui::PopStyleColor(6);
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor(3);

        ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(g_hWnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    cleanup();

    return 0;
}