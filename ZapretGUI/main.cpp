#include <Windows.h>
#include <iostream>
#include <string>
#include <dwmapi.h> 
#include <map>
#include <set>
#include <vector>

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

int page = 0;

bool IsRunAsAdmin() {
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

void RelaunchAsAdmin(LPSTR lpCmdLine) 
{
    char exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
    sei.lpVerb = "runas";
    if (strlen(lpCmdLine) > 0)
        sei.lpParameters = lpCmdLine;
    sei.lpFile = exePath;
    sei.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteEx(&sei)) {
        MessageBox(NULL, "Не удалось перезапустить приложение с правами администратора.", "Ошибка", MB_OK | MB_ICONERROR);
    }
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    tools::setWorkingDirectoryToExecutablePath();
    vars::init();

#ifndef _DEBUG
    if (!IsRunAsAdmin())
    {
        RelaunchAsAdmin(lpCmdLine);
        return 0;
    }
#else
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
#endif

    HANDLE hMutexOnce;
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
        MessageBoxA(0, "Ошибка загрузки тектсуры", 0, 0);
        ::DestroyWindow(g_hWnd);
        return 0;
    }

    int ds_width = 0;
    int ds_height = 0;
    ID3D11ShaderResourceView* discord_texture = NULL;
    if (!LoadTextureFromMemory(discord_data, sizeof(discord_data), &discord_texture, &ds_width, &ds_height))
    {
        MessageBoxA(0, "Ошибка загрузки тектсуры", 0, 0);
        ::DestroyWindow(g_hWnd);
        return 0;
    }

    int _7tv_width = 0;
    int _7tv_height = 0;
    ID3D11ShaderResourceView* _7tv_texture = NULL;
    if (!LoadTextureFromMemory(_7tv_data, sizeof(_7tv_data), &_7tv_texture, &_7tv_width, &_7tv_height))
    {
        MessageBoxA(0, "Ошибка загрузки тектсуры", 0, 0);
        ::DestroyWindow(g_hWnd);
        return 0;
    }

    int proton_width = 0;
    int proton_height = 0;
    ID3D11ShaderResourceView* proton_texture = NULL;
    if (!LoadTextureFromMemory(proton_data, sizeof(proton_data), &proton_texture, &proton_width, &proton_height))
    {
        MessageBoxA(0, "Ошибка загрузки тектсуры", 0, 0);
        ::DestroyWindow(g_hWnd);
        return 0;
    }

    int ph_width = 0;
    int ph_height = 0;
    ID3D11ShaderResourceView* ph_texture = NULL;
    if (!LoadTextureFromMemory(ph_data, sizeof(ph_data), &ph_texture, &ph_width, &ph_height))
    {
        MessageBoxA(0, "Ошибка загрузки тектсуры", 0, 0);
        ::DestroyWindow(g_hWnd);
        return 0;
    }

    std::vector<std::string> sharing_youtube_service = { "cf-ech", "pornhub", "proton", "youtube" };
    SharedZapret* cf_ech = new SharedZapret("cf-ech", sharing_youtube_service, "shared_youtube_service");

    vars::services =
    {
        new SharedZapret(yt_width, yt_height, "Youtube", "youtube", vars::json_settings["services"]["youtube"], youtube_texture, sharing_youtube_service, "shared_youtube_service"),
        new Zapret(ds_width, ds_height, "Discord", "discord", vars::json_settings["services"]["discord"], discord_texture),
        new Zapret(ds_width, ds_height, "7tv", "7tv", vars::json_settings["services"]["7tv"], _7tv_texture),
        new SharedZapret(proton_width, proton_height, u8"Proton (без mail)", "proton", vars::json_settings["services"]["proton"], proton_texture, sharing_youtube_service, "shared_youtube_service"),
        new SharedZapret(ph_width, ph_height, "PornHub", "pornhub", vars::json_settings["services"]["pornhub"], ph_texture, sharing_youtube_service, "shared_youtube_service"),
        cf_ech,
    };

    bool failed_ver_check = false;
    if (vars::bStart_v_check)
    {
        std::string answer;
        bool result = tools::request("https://elllkere.top/misterfish/version.txt", &answer);
        if (!result || answer.empty())
            failed_ver_check = true;
        else if (answer != vars::version)
            MessageBoxA(0, "Вышла новая версия, скачать можно в настройках", window::window_name, MB_OK);
    }

    if (!failed_ver_check)
    {
        if (!(strstr(lpCmdLine, "/autostart") && vars::bTray_start == true))
            ::ShowWindow(g_hWnd, nCmdShow);
    }
    else
    {
        ::ShowWindow(g_hWnd, nCmdShow);
        MessageBoxA(g_hWnd, "Не удалось проверить версию", window::window_name, MB_OK);
    }

    CreateTrayIcon();

    ::UpdateWindow(g_hWnd);

    tools::killAll();

    if (vars::bUnlock_ech == true)
        cf_ech->active = true;

    for (auto& s : vars::services)
    {
        if (s->active)
            s->startProcces();
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
        ImGui::PushStyleColor(ImGuiCol_Button, ImColor(40, 40, 40).Value);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor(45, 45, 45).Value);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor(45, 45, 45).Value);
        ImGui::BeginChild("##left", ImVec2(150, 0));
        {
            if (ImGui::Button(u8"Сервисы", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
                page = 0;

            if (ImGui::Button(u8"Настройки", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
                page = 1;

            ImVec2 prev_cursor = ImGui::GetCursorPos();

            ImVec2 text_size = ImGui::CalcTextSize(vars::version.c_str());
            ImGui::SetCursorPosX(prev_cursor.x + ImGui::GetContentRegionAvail().x / 2 - text_size.x / 2);
            ImGui::SetCursorPosY(prev_cursor.y + ImGui::GetContentRegionAvail().y - text_size.y - window::top_padding);
            ImGui::Text(vars::version.c_str());

        }
        ImGui::EndChild();
        ImGui::PopStyleColor(4);

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
                for (int i = 0, num = 0; i < vars::services.size(); i++, num++)
                {
                    Zapret* service = vars::services[i];
                    if (service->hide)
                        continue;

                    ImVec2 text_scale = ImGui::CalcTextSize(service->name.c_str());

                    if (i > 0)
                    {
                        if (i % 5 == 0)
                        {
                            start.y += service->height + text_scale.y + style.FramePadding.y + 20;
                            num = 0;
                        }

                        ImGui::SetCursorPosX(start.x + (service->width + style.FramePadding.x + 10) * num);
                    }

                    ImGui::SetCursorPosY(start.y);

                    if (ImGui::ImageButton(service->name.c_str(), service->texture, ImVec2(service->width, service->height)))
                    {
                        if (service->active && !service->isRunning())
                            service->active = 1;
                        else
                            service->active ^= 1;

                        vars::json_settings["services"][service->id_name] = service->active;
                        tools::updateSettings(vars::json_settings);

                        if (service->active)
                            service->startProcces();
                        else
                            service->terminate();
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        ImGui::SetTooltip(service->isRunning() ? u8"Выключить" : u8"Включить");
                    }

                    ImGui::SetCursorPosX((start.x + service->width + style.FramePadding.x + 10) * num + start.x + style.FramePadding.x + service->width / 2 - text_scale.x / 2);

                    if (service->isRunning())
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 255, 100, 255));
                    else
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120, 120, 120, 255));

                    ImGui::Text(service->name.c_str());

                    ImGui::PopStyleColor();
                }

                break;
            }
            case 1:
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor(40, 40, 40).Value);
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImColor(45, 45, 45).Value);
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImColor(45, 45, 45).Value);
                ImGui::PushStyleColor(ImGuiCol_CheckMark, ImColor(10, 10, 10).Value);

                if (ImGui::Checkbox(u8"Проверка версии при запуске", &vars::bStart_v_check))
                {
                    vars::json_settings["start_version_check"] = vars::bStart_v_check;
                    tools::updateSettings(vars::json_settings);
                }

                if (ImGui::Checkbox(u8"Автозапуск с windows", &vars::bWin_start))
                {
                    vars::json_settings["win_start"] = vars::bWin_start;
                    vars::json_settings["tray_start"] = false;
                    vars::bTray_start = false;
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
                    else
                        vars::json_settings["tray_start"] = vars::bTray_start;
                    tools::updateSettings(vars::json_settings);
                }

                if (ImGui::Checkbox(u8"Разблокировать протокол Cloudflare ECH", &vars::bUnlock_ech))
                {
                    cf_ech->active = vars::bUnlock_ech;
                    if (vars::bUnlock_ech)
                        cf_ech->startProcces();
                    else
                        cf_ech->terminate();

                    vars::json_settings["unlock_ech"] = vars::bUnlock_ech;
                    tools::updateSettings(vars::json_settings);
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(u8"Если Вы не знаете что такое Cloudflare, то не стоит выключать.\nРазблокирует протокол ECH у Cloudflare, который включен всегда на бесплатном тарифе\nиз-за чего много обычных не забаненных сайтов не загружаются");

                if (ImGui::Combo(u8"Провайдер", &vars::provider, tools::convertMapToCharArray(vars::providers).data(), vars::providers.size()))
                {
                    vars::json_settings["provider"] = vars::provider;
                    tools::updateSettings(vars::json_settings);

                    for (auto& s : vars::services)
                    {
                        if (s->active)
                        {
                            s->terminate();
                            s->startProcces();
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
                            if (!tools::existTaskSchedulerEntry())
                                tools::createTaskSchedulerEntry();

                            if (tools::isInStartup())
                                tools::removeFromStartup();

                            break;
                        }
                        
                        case 1:
                        {
                            if (!tools::isInStartup())
                                tools::addToStartup();

                            if (tools::existTaskSchedulerEntry())
                                tools::deleteTaskSchedulerEntry();

                            break;
                        }
                        }
                    }

                    vars::json_settings["auto_start"] = vars::auto_start;
                    tools::updateSettings(vars::json_settings);
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(u8"UAC - контроль учетных записей\n(окошко с подтверждением при запуске программы)");

                if (ImGui::Combo(u8"Действие при Х", &vars::x_method, tools::convertMapToCharArray(vars::x_methods).data(), vars::x_methods.size()))
                {
                    vars::json_settings["x_method"] = vars::x_method;
                    tools::updateSettings(vars::json_settings);
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(u8"Что делать при нажатии на Х");

                ImGui::PushStyleColor(ImGuiCol_Button, ImColor(40, 40, 40).Value);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor(45, 45, 45).Value);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor(45, 45, 45).Value);

                if (ImGui::Button(u8"Завершить все обходы", ImVec2(200, 30)))
                    tools::killAll();

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(u8"Завершит все прошлые и нынешние процессы zapret");

                if (ImGui::Button(u8"Проверить версию", ImVec2(200, 30)))
                {
                    std::string answer;
                    bool result = tools::request("https://elllkere.top/misterfish/version.txt", &answer);
                    if (!result || answer.empty())
                        MessageBoxA(g_hWnd, "Не удалось проверить версию", window::window_name, MB_OK);
                    else
                    {
                        if (answer == vars::version)
                            MessageBoxA(g_hWnd, "Версия актуальна", window::window_name, MB_OK);
                        else
                            system("start https://github.com/Elllkere/misterfishdpi/releases/latest");
                    }
                }

                if (ImGui::Button("GitHub", ImVec2(200, 30)))
                    system("start https://github.com/Elllkere/misterfishdpi/");

                ImGui::PopStyleColor(3);
                ImGui::PopStyleColor(4);
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

    tools::killAll();

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(g_hWnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    DestroyTrayIcon();

    system("sc stop WinDivert");

    if (hMutexOnce)
    {
        ReleaseMutex(hMutexOnce);
        CloseHandle(hMutexOnce);
    }

    return 0;
}