#pragma once

#include "zapret.hpp"

void SharedZapret::terminate()
{
    std::string args = "";
    std::string cur_path = std::filesystem::current_path().string();

    std::vector<json> elements;

    std::string txt = "";
    getArgs(id_name, txt, cur_path, true);

    elements.push_back({ {"id_name", id_name}, {"txt", txt}, {"active", false} });

    for (auto& service : shared_with)
    {
        txt = "";
        getArgs(service, txt, cur_path, true);
        elements.push_back({ {"id_name", service}, {"txt", txt}, {"active", false} });
    }

    json j = elements;

    for (auto& service : vars::services)
    {
        SharedZapret* shared = dynamic_cast<SharedZapret*>(service);
        if (!shared && service->id_name == id_name)
        {
            MessageBoxA(0, "Ошибка запуска процесса: ошибка преобразования класса", 0, 0);
            return;
        }
        else if (!shared)
            continue;

        for (auto& elem : j)
        {
            if (shared->id_name == std::string(elem["id_name"]))
            {
                elem["active"] = shared->active;
            }
        }
    }

    std::map<std::string, std::set<std::string>> domains;
    std::set<std::string> domains_used;

    for (auto& elem : j)
    {
        if (domains.find(elem["id_name"]) == domains.end())
            domains[elem["id_name"]] = {};

        tools::getDomains(std::format("{}\\{}", cur_path, std::string(elem["txt"])), domains[elem["id_name"]]);
    }

    tools::getDomains(std::format("{}\\{}", cur_path, std::string(vars::services_txt[shared_id])), domains_used);

    for (auto& elem : j)
    {
        for (auto& domain : domains[elem["id_name"]])
        {
            if (elem["id_name"] == id_name)
            {
                if (domains_used.find(domain) != domains_used.end())
                    domains_used.erase(domain);
            }
            else
            {
                if (elem["active"] == true)
                {
                    if (domains_used.find(domain) == domains_used.end())
                        domains_used.insert(domain);
                }
                else
                {
                    if (domains_used.find(domain) != domains_used.end())
                        domains_used.erase(domain);
                }
            }
        }
    }

    std::ofstream service_file(std::format("{}\\{}", cur_path, std::string(vars::services_txt[shared_id])));
    if (service_file.is_open())
    {
        for (auto it = domains_used.begin(); it != domains_used.end(); ++it)
        {
            service_file << *it;
            if (std::next(it) != domains_used.end())
                service_file << '\n';
        }

        service_file.close();
    }
    else
        MessageBoxA(0, std::format("Ошибка остановления процесса: {}", GetLastError()).c_str(), 0, 0);

    if (isOnlyOneRunning())
        Zapret::terminate();
}

void SharedZapret::startProcces(const std::string& _id_name)
{
    std::string args = "";
    std::string cur_path = std::filesystem::current_path().string();

    std::vector<json> elements;

    std::string txt = "";
    getArgs(id_name, txt, cur_path, true);

    elements.push_back({ {"id_name", id_name}, {"txt", txt}, {"active", false} });

    for (auto& service : shared_with)
    {
        txt = "";
        getArgs(service, txt, cur_path, true);
        elements.push_back({ {"id_name", service}, {"txt", txt}, {"active", false} });
    }

    json j = elements;

    for (auto& service : vars::services)
    {
        SharedZapret* shared = dynamic_cast<SharedZapret*>(service);
        if (!shared && service->id_name == id_name)
        {
            MessageBoxA(0, "Ошибка запуска процесса: ошибка преобразования класса", 0, 0);
            return;
        }
        else if (!shared)
            continue;

        for (auto& elem : j)
        {
            if (shared->id_name == std::string(elem["id_name"]))
            {
                elem["active"] = shared->active;
            }
        }
    }

    std::map<std::string, std::set<std::string>> domains;
    std::set<std::string> domains_used;

    for (auto& elem : j)
    {
        if (domains.find(elem["id_name"]) == domains.end())
            domains[elem["id_name"]] = {};

        tools::getDomains(std::format("{}\\{}", cur_path, std::string(elem["txt"])), domains[elem["id_name"]]);
    }

    tools::getDomains(std::format("{}\\{}", cur_path, std::string(vars::services_txt[shared_id])), domains_used);

    for (auto& elem : j)
    {
        for (auto& domain : domains[elem["id_name"]])
        {
            if (elem["id_name"] == id_name)
            {
                if (domains_used.find(domain) == domains_used.end())
                    domains_used.insert(domain);
            }
            else
            {
                if (elem["active"] == true)
                {
                    if (domains_used.find(domain) == domains_used.end())
                        domains_used.insert(domain);
                }
                else
                {
                    if (domains_used.find(domain) != domains_used.end())
                        domains_used.erase(domain);
                }
            }
        }
    }

    std::ofstream service_file(std::format("{}\\{}", cur_path, std::string(vars::services_txt[shared_id])));
    if (service_file.is_open())
    {
        for (auto it = domains_used.begin(); it != domains_used.end(); ++it)
        {
            service_file << *it;
            if (std::next(it) != domains_used.end())
                service_file << '\n';
        }

        service_file.close();
    }
    else
        MessageBoxA(0, std::format("Ошибка запуска процесса: {}", GetLastError()).c_str(), 0, 0);

    if (!isRunning())
        Zapret::startProcces(shared_id);
}

bool SharedZapret::isOnlyOneRunning() const
{
    for (auto& service : vars::services)
    {
        if (std::find(shared_with.begin(), shared_with.end(), service->id_name) != shared_with.end() && service->isRunning())
            return false;
    }

    return true;
}

bool SharedZapret::isRunning()
{
    if (!active)
        return false;

    if (Zapret::isRunning())
        return true;

    for (auto& service : vars::services)
    {
        if (std::find(shared_with.begin(), shared_with.end(), service->id_name) != shared_with.end())
        {
            if (static_cast<Zapret*>(service)->Zapret::isRunning())
                return true;
        }
    }

    return false;
}

void Zapret::toggleActive()
{
    if (active && !isRunning())
        active = 1;
    else
        active ^= 1;

    vars::json_settings["services"][id_name]["active"] = active;
    tools::updateSettings(vars::json_settings);

    if (active)
        startProcces();
    else
        terminate();
}

bool Zapret::isRunning()
{
    if (prc == nullptr)
        return false;

    return prc->isRunning();
}

void Zapret::startProcces(const std::string& id_name)
{
    if (isRunning())
        return;

    std::string args = "";
    std::string cur_path = std::filesystem::current_path().string();

    if (id_name.size() > 0)
        getArgs(id_name, args, cur_path);
    else
        getArgs(this->id_name, args, cur_path);

    prc = new Process(cur_path + "\\winws.exe", args);
}

void Zapret::terminate()
{
    if (prc == nullptr)
        return;

    prc->terminate();
}

void Zapret::getArgs(const std::string& id_name, std::string& args, const std::string& cur_path, bool hostlist)
{
    if (hostlist)
    {
        args = vars::services_txt[id_name];
        return;
    }

    std::string txt = vars::services_txt[id_name];

    if (id_name == "shared_youtube_service")
    {
        if (vars::provider < 2)
        {
            args = std::format("--wf-tcp=80,443 --wf-udp=443 ");
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=11 --dpi-desync-fake-quic=\"{}\\quic_initial_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=ipfrag2 --dpi-desync-ipfrag-pos-udp=8 --new ", cur_path, txt);
            args += std::format("--filter-tcp=80 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --new ", cur_path, txt);

            if (vars::provider == 0)
                args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-ttl=1 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --dpi-desync-fake-tls=\"{}\\tls_clienthello_www_google_com.bin\"", cur_path, txt, cur_path);
            else
            {
                args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake,split --dpi-desync-autottl=5 --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-fake-tls=\"{}\\tls_clienthello_www_google_com.bin\"", cur_path, txt, cur_path);
                args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake,disorder2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --new ", cur_path, txt);
            }
        }
        else if (vars::provider == 2)
        {
            args = std::format("--wf-tcp=80,443 --wf-udp=443 ");
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{}\\quic_initial_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-tcp=80 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-fake-tls=\"{}\\tls_clienthello_www_google_com.bin\"", cur_path, txt, cur_path);
        }
        else if (vars::provider == 3)
        {
            args = std::format("--wf-tcp=80,443 --wf-udp=443 ");
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=5 --dpi-desync-fake-quic=\"{}\\quic_initial_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=ipfrag2 --dpi-desync-ipfrag-pos-udp=16 --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=disorder --dpi-desync-fooling=badseq --dpi-desync-split-pos=midsld --dpi-desync-fake-tls=\"{}\\tls_clienthello_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-ttl=1 --dpi-desync-autottl=1 --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake,disorder2 --dpi-desync-split-pos=midsld --dpi-desync-repeats=6 --dpi-desync-fooling=badseq,md5sig --new ", cur_path, txt);
            args += std::format("--filter-tcp=80 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig", cur_path, txt);
        }
    }
    else if (id_name == "discord")
    {
        std::string full = vars::services_txt[id_name];
        auto spl = tools::split(full, "|");

        std::string discord = spl[0].data();
        std::string discord_ip = spl[1].data();

        args = std::format("--wf-tcp=443 --wf-udp=443,50000-50100 ");
        args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-ttl=4 --new ", cur_path, discord);
        args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=6 --new ", cur_path, discord);
        args += std::format("--filter-udp=50000-50100 --ipset=\"{}\\{}\" --dpi-desync=fake --dpi-desync-any-protocol --dpi-desync-fake-quic=\"{}\\quic_initial_www_google_com.bin\"", cur_path, discord_ip, cur_path);
    }
    else if (id_name == "7tv")
    {
        args = std::format("--wf-tcp=443 --wf-udp=443 --filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-ttl=3 --new ", cur_path, txt);
        args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=6", cur_path, txt);
    }
}