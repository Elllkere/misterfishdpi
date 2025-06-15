#pragma once

#include "zapret.hpp"

void SharedZapret::restart()
{
    Zapret::terminate();
    start();
}

void SharedZapret::terminate()
{
    std::string cur_path = std::filesystem::current_path().string();

    std::string txt = this->txt;
    std::set<std::string> domains_used;

    auto elements = this->info->shared_ids;
    elements.insert({ id_name, txt });

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
        else if (elements.find(service->id_name) == elements.end())
            continue;
        else if (!service->active)
            continue;

        tools::getDomains(std::format("{}\\{}", cur_path, service->txt), domains_used);
    }

    std::ofstream service_file(std::format("{}\\{}", cur_path, this->info->shared_txt));
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
    {
        if (Zapret::isRunning())
            Zapret::terminate();
        else
        {
            for (auto& service : vars::services)
            {
                if (this->info->shared_ids.find(service->id_name) != this->info->shared_ids.end())
                {
                    if (Zapret* base = dynamic_cast<Zapret*>(service))
                    {
                        if (base->Zapret::isRunning())
                        {
                            base->Zapret::terminate();
                            break;
                        }
                    }
                }
            }
        }
    }
}

void SharedZapret::start(const std::string& _l)
{
    std::string cur_path = std::filesystem::current_path().string();

    std::string txt = this->txt;
    std::set<std::string> domains_used;

    auto elements = this->info->shared_ids;
    elements.insert({ id_name, txt });

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
        else if (elements.find(service->id_name) == elements.end())
            continue;
        else if (!service->active)
            continue;

        tools::getDomains(std::format("{}\\{}", cur_path, service->txt), domains_used);
    }

    std::ofstream service_file(std::format("{}\\{}", cur_path, this->info->shared_txt));
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
        Zapret::start(this->info->shared_id);
}

bool SharedZapret::isOnlyOneRunning() const
{
    for (auto& service : vars::services)
    {
        if (this->info->shared_ids.find(service->id_name) != this->info->shared_ids.end() && service->isRunning())
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
        if (this->info->shared_ids.find(service->id_name) != this->info->shared_ids.end())
        {
            if (Zapret* base = dynamic_cast<Zapret*>(service))
            {
                if (base->Zapret::isRunning())
                    return true;
            }
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
    tools::updateSettings(vars::json_settings, vars::json_setting_name);

    if (active)
        start();
    else
        terminate();
}

bool Zapret::isRunning()
{
    if (prc == nullptr)
        return false;

    return prc->isRunning();
}

void Zapret::start(const std::string& id_name)
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

void Zapret::restart()
{
    terminate();
    start();
}

void Zapret::terminate()
{
    if (prc == nullptr)
        return;

    prc->terminate();
}

void Zapret::addPorts(const std::string& input, std::set<int>& port_set)
{
    std::stringstream ss(input);
    std::string token;

    while (std::getline(ss, token, ','))
    {
        size_t dash_pos = token.find('-');
        if (dash_pos != std::string::npos)
        {
            int start = std::stoi(token.substr(0, dash_pos));
            int end = std::stoi(token.substr(dash_pos + 1));
            for (int p = start; p <= end; ++p)
            {
                port_set.insert(p);
            }
        }
        else
        {
            int port = std::stoi(token);
            port_set.insert(port);
        }
    }
}

std::string Zapret::portsToString(const std::set<int>& ports) 
{
    if (ports.empty()) return "";

    std::ostringstream result;
    auto it = ports.begin();
    int range_start = *it;
    int range_end = *it;

    ++it;
    for (; it != ports.end(); ++it) 
    {
        if (*it == range_end + 1) 
        {
            range_end = *it;
        }
        else 
        {
            if (range_start == range_end) 
            {
                result << range_start;
            }
            else 
            {
                result << range_start << "-" << range_end;
            }

            result << ",";
            range_start = range_end = *it;
        }
    }

    if (range_start == range_end) 
    {
        result << range_start;
    }
    else 
    {
        result << range_start << "-" << range_end;
    }

    return result.str();
}

void Zapret::getArgs(const std::string& id_name, std::string& args, const std::string& cur_path)
{
    std::string txt = "";

    SharedZapret* shared = dynamic_cast<SharedZapret*>(this);
    if (shared)
        txt = shared->info->shared_txt;
    else
        txt = this->txt;

    if (id_name == "shared_service_youtube")
    {
        args = std::format("--wf-tcp=80,443 --wf-udp=443 ");

        switch (vars::provider)
        {
        default:
        {
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=11 --dpi-desync-fake-quic=\"{}\\quic_initial_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-tcp=80 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=disorder2 --dpi-desync-split-pos=1,midsld --new ", cur_path, txt);
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync-any-protocol --dpi-desync=ipfrag2 --dpi-desync-ipfrag-pos-udp=8 --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=split --dpi-desync-fooling=md5sig --dpi-desync-split-pos=midsld --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-ttl=1 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --dpi-desync-fake-tls=\"{}\\tls_clienthello_www_google_com.bin\"", cur_path, txt, cur_path);

            break;
        }

        case providers_list::PROVIDER_ROST:
        {
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=11 --dpi-desync-fake-quic=\"{}\\quic_initial_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-tcp=80 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=disorder2 --dpi-desync-split-pos=1,midsld --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake,split --dpi-desync-ttl=6 --dpi-desync-split-pos=midsld --dpi-desync-fake-tls=\"{}\\tls_clienthello_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-fooling=badseq --dpi-desync-split-pos=1,midsld --dpi-desync-fake-tls=\"{}\\tls_clienthello_www_google_com.bin\"", cur_path, txt, cur_path);

            break;
        }

        case providers_list::PROVIDER_MTS:
        {
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{}\\quic_initial_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-tcp=80 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-fake-tls=\"{}\\tls_clienthello_www_google_com.bin\"", cur_path, txt, cur_path);

            break;
        }

        case providers_list::PROVIDER_LOVIT:
        {
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=5 --dpi-desync-fake-quic=\"{}\\quic_initial_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync-any-protocol --dpi-desync=ipfrag2 --dpi-desync-ipfrag-pos-udp=16 --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=disorder --dpi-desync-fooling=badseq --dpi-desync-split-pos=midsld --dpi-desync-fake-tls=\"{}\\tls_clienthello_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-ttl=1 --dpi-desync-autottl=1 --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake,disorder2 --dpi-desync-split-pos=midsld --dpi-desync-repeats=6 --dpi-desync-fooling=badseq,md5sig --new ", cur_path, txt);
            args += std::format("--filter-tcp=80 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig", cur_path, txt);

            break;
        }
        }
    }
    else if (id_name == "shared_service_7tv")
    {
        args = std::format("--wf-tcp=443 --wf-udp=443 ");
        args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=11 --dpi-desync-fake-quic=\"{}\\quic_initial_www_google_com.bin\" --new ", cur_path, txt, cur_path);
        args += std::format("--hostlist=\"{}\\{}\" --dpi-desync=fake,disorder2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig", cur_path, txt);
    }
    else if (id_name == "cf-ech")
    {
        args = std::format("--wf-tcp=443 ");

        switch (vars::provider)
        {
        case providers_list::PROVIDER_OTHER:

            args += std::format("--ipset=\"{}\\{}\" --dpi-desync=fake,multidisorder --dpi-desync-fake-tls-mod=rnd,dupsid --dpi-desync-repeats=4 --dpi-desync-split-pos=100,midsld,sniext+1,endhost-2,-10 --dpi-desync-ttl=4", cur_path, txt);
            break;

        default:

            args += std::format("--ipset=\"{}\\{}\" --dpi-desync=fake,split --dpi-desync-autottl=2 --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-fake-tls=\"{}\\tls_clienthello_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--ipset=\"{}\\{}\" --dpi-desync=fake,disorder2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig", cur_path, txt);
        }

    }
    else if (id_name == "amazon")
    {
        //https://ip-ranges.amazonaws.com/ip-ranges.json

        std::string wf_filter = "";
        std::string tcp_filter = "";
        std::string udp_filter = "";
        switch (vars::amazon_type)
        {
        case 0:
        {
            tcp_filter = "--filter-tcp=21,22,80,443,1024-65535";
            udp_filter = "--filter-udp=21,22,80,443,1024-65535";
            wf_filter = "--wf-tcp=21,22,80,443,1024-65535 --wf-udp=21,22,80,443,1024-65535";
            break;
        }
        case 1:
        {
            std::set<int> udp_ports;
            addPorts("21,22,80,443", udp_ports);
            addPorts("7778-7781", udp_ports); //DBD
            addPorts("5055-5058, 27000-27003", udp_ports); //REPO (Photon)

            std::set<int> tcp_ports;
            addPorts("21,22,80,443", tcp_ports);
            //addPorts("40002", tcp_ports); //PUBG
            addPorts("9090-9093,19090-19093", tcp_ports); //REPO (Photon)

            std::string tcp = portsToString(tcp_ports);
            std::string udp = portsToString(udp_ports);

            tcp_filter = std::format("--filter-tcp={}", tcp);
            udp_filter = std::format("--filter-udp={}", udp);
            wf_filter = std::format("--wf-tcp={} --wf-udp={}", tcp, udp);

            break;
        }
        case 2:
        {
            tcp_filter = "--filter-tcp=21,22,80,443";
            udp_filter = "--filter-udp=21,22,80,443";
            wf_filter = "--wf-tcp=21,22,80,443 --wf-udp=21,22,80,443";
            break;
        }
        }

        args = std::format("{} ", wf_filter);

        switch (vars::provider)
        {
        case providers_list::PROVIDER_MTS:
        case providers_list::PROVIDER_ROST:

            args += std::format("--ipset=\"{}\\{}\" {} --dpi-desync=fake,split --dpi-desync-autottl=2 --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-fake-tls=\"{}\\tls_clienthello_www_google_com.bin\" --new ", cur_path, txt, tcp_filter, cur_path);
            args += std::format("--ipset=\"{}\\{}\" {} --dpi-desync=fake,disorder2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --new ", cur_path, txt, tcp_filter);

            args += std::format("--ipset=\"{}\\{}\" {} --dpi-desync-any-protocol --dpi-desync=ipfrag2 --dpi-desync-ipfrag-pos-udp=16", cur_path, txt, udp_filter);
            break;

        default:

            args += std::format("--ipset=\"{}\\{}\" {} --dpi-desync=fake,multidisorder --dpi-desync-fake-tls-mod=rnd,dupsid --dpi-desync-repeats=4 --dpi-desync-split-pos=100,midsld,sniext+1,endhost-2,-10 --dpi-desync-ttl=4 --new ", cur_path, txt, tcp_filter);

            args += std::format("--ipset=\"{}\\{}\" {} --dpi-desync-any-protocol --dpi-desync=ipfrag2 --dpi-desync-ipfrag-pos-udp=8", cur_path, txt, udp_filter);
            break;
        }
        
    }
    else if (id_name == "discord")
    {
        std::string discord = txt;

        args = std::format("--wf-tcp=443 --wf-udp=443,50000-50100 ");

        switch (vars::provider)
        {
        case providers_list::PROVIDER_ROST:
        {
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=disorder2 --dpi-desync-split-pos=1,sniext+1,host+1,midsld-2,midsld,midsld+2,endhost-1 --new ", cur_path, discord);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-fooling=md5sig --new ", cur_path, discord);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-fooling=datanoack --new ", cur_path, discord);

            break;
        }

        default:
        {
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake,split --dpi-desync-ttl=4 --dpi-desync-split-pos=1 --dpi-desync-repeats=8 --new ", cur_path, discord);
            break;
        }
        }            

        args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-any-protocol --dpi-desync-repeats=7 --new ", cur_path, discord);
        args += std::format("--filter-udp=50000-50100 --filter-l7=discord,stun --dpi-desync=fake --dpi-desync-fake-quic=\"{}\\quic_initial_www_google_com.bin\"", cur_path, cur_path);
    }
}