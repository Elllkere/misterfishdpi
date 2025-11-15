#pragma once

#include "zapret.hpp"
#include "../tools/tools.hpp"
#include "../data.hpp"

#include <cctype>
#include <sstream>
#include <format>

namespace
{
    std::string trimCopy(const std::string& value)
    {
        size_t start = 0;
        while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
            ++start;
        size_t end = value.size();
        while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
            --end;
        return value.substr(start, end - start);
    }

    std::vector<std::string> splitArgsTokens(const std::string& input)
    {
        std::vector<std::string> tokens;
        std::string current;
        for (char ch : input)
        {
            if (std::isspace(static_cast<unsigned char>(ch)))
            {
                if (!current.empty())
                {
                    tokens.push_back(current);
                    current.clear();
                }
            }
            else
            {
                current.push_back(ch);
            }
        }

        if (!current.empty())
            tokens.push_back(current);

        return tokens;
    }

    std::vector<std::string> splitListValues(const std::string& input)
    {
        std::vector<std::string> values;
        std::string current;
        for (char ch : input)
        {
            if (ch == ',')
            {
                std::string token = trimCopy(current);
                if (!token.empty())
                    values.push_back(token);
                current.clear();
            }
            else
            {
                current.push_back(ch);
            }
        }

        std::string token = trimCopy(current);
        if (!token.empty())
            values.push_back(token);

        return values;
    }

    void appendUniqueOrdered(std::vector<std::string>& destination, const std::vector<std::string>& source)
    {
        for (const auto& value : source)
        {
            if (std::find(destination.begin(), destination.end(), value) == destination.end())
                destination.push_back(value);
        }
    }

    std::string joinTokens(const std::vector<std::string>& tokens)
    {
        std::string result;
        for (const auto& token : tokens)
        {
            if (token.empty())
                continue;
            if (!result.empty())
                result += ' ';
            result += token;
        }
        return result;
    }

    ZapretProcessEntry parseProcessEntry(const std::string& args)
    {
        ZapretProcessEntry entry;
        auto tokens = splitArgsTokens(args);
        std::vector<std::string> current;

        auto flush = [&]()
        {
            std::string joined = joinTokens(current);
            if (!joined.empty())
                entry.strategies.push_back(joined);
            current.clear();
        };

        for (const auto& token_raw : tokens)
        {
            std::string token = trimCopy(token_raw);
            if (token.empty())
                continue;

            if (token == "--new")
            {
                flush();
                continue;
            }

            const std::string wf_tcp_prefix = "--wf-tcp=";
            const std::string wf_udp_prefix = "--wf-udp=";

            if (token.rfind(wf_tcp_prefix, 0) == 0)
            {
                auto values = splitListValues(token.substr(wf_tcp_prefix.size()));
                appendUniqueOrdered(entry.wf_tcp, values);
                continue;
            }

            if (token.rfind(wf_udp_prefix, 0) == 0)
            {
                auto values = splitListValues(token.substr(wf_udp_prefix.size()));
                appendUniqueOrdered(entry.wf_udp, values);
                continue;
            }

            current.push_back(token);
        }

        flush();

        return entry;
    }
}

std::unique_ptr<Process> Zapret::s_process = nullptr;
std::map<std::string, ZapretProcessEntry> Zapret::s_entries = {};
std::vector<std::string> Zapret::s_arg_order = {};
std::string Zapret::s_current_cmdline = "";
std::string Zapret::s_executable_path = "";

ServiceBase::ServiceBase(const std::string& id_name, bool hidden)
    : id_name(id_name)
{
    hide = hidden;
    panel_hide = false;
}

ServiceBase::ServiceBase(int width, int height, const std::string& name, const std::string& id_name, ID3D11ShaderResourceView* texture)
    : width(width), height(height), name(name), id_name(id_name), texture(texture)
{
    hide = false;
    active = vars::json_settings["services"][id_name]["active"];
    hotkey = vars::json_settings["services"][id_name]["hotkey"];
    panel_hide = vars::json_settings["services"][id_name]["hide"];
}

void ServiceBase::toggleActive()
{
    if (active && !isRunning())
        active = true;
    else
        active = !active;

    vars::json_settings["services"][id_name]["active"] = active;
    tools::updateSettings(vars::json_settings, vars::json_setting_name);

    if (active)
        start();
    else
        terminate();
}

Zapret::Zapret(const std::string& id_name, const std::string& txt)
    : ServiceBase(id_name, true), txt("lists\\" + txt)
{
    registerOrder(this->id_name);
}

Zapret::Zapret(int width, int height, const std::string& name, const std::string& id_name, ID3D11ShaderResourceView* texture, const std::string& txt)
    : ServiceBase(width, height, name, id_name, texture), txt("lists\\" + txt)
{
    registerOrder(this->id_name);
}

void SharedZapret::restart()
{
    Zapret::removeArgs(this->info->shared_id);
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
            MessageBoxA(0, "Failed to start process: invalid shared service type", 0, 0);
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
        MessageBoxA(0, std::format("Failed to open file: {}", GetLastError()).c_str(), 0, 0);

    if (domains_used.empty())
        Zapret::removeArgs(this->info->shared_id);
    else
        Zapret::start(this->info->shared_id);
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
            MessageBoxA(0, "Failed to start process: invalid shared service type", 0, 0);
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
        MessageBoxA(0, std::format("Failed to start process: {}", GetLastError()).c_str(), 0, 0);

    Zapret::start(this->info->shared_id);
}

bool SharedZapret::isRunning()
{
    if (!active)
        return false;

    return Zapret::hasActiveEntry(this->info->shared_id);
}

bool Zapret::isRunning()
{
    return hasActiveEntry(this->id_name);
}

void Zapret::start(const std::string& id_name)
{
    std::string key = id_name.empty() ? this->id_name : id_name;

    std::string args = "";
    std::string cur_path = std::filesystem::current_path().string();

    getArgs(key, args, cur_path);
    upsertArgs(key, args, cur_path);
}

void Zapret::restart()
{
    removeArgs(this->id_name);
    start();
}

void Zapret::terminate()
{
    removeArgs(this->id_name);
}

void Zapret::upsertArgs(const std::string& key, const std::string& args, const std::string& cur_path)
{
    if (key.empty())
        return;

    if (s_executable_path.empty())
        s_executable_path = cur_path + "\\winws.exe";

    registerOrder(key);

    ZapretProcessEntry entry = parseProcessEntry(args);

    if (entry.strategies.empty())
    {
        removeArgs(key);
        return;
    }

    s_entries[key] = entry;

    rebuildProcess();
}

void Zapret::removeArgs(const std::string& key)
{
    if (key.empty())
        return;

    auto it = s_entries.find(key);
    if (it == s_entries.end())
        return;

    s_entries.erase(it);

    rebuildProcess();
}

bool Zapret::hasActiveEntry(const std::string& key)
{
    if (key.empty())
        return false;

    auto it = s_entries.find(key);
    if (it == s_entries.end())
        return false;

    if (it->second.strategies.empty())
        return false;

    return s_process && s_process->isRunning();
}

void Zapret::initializeOrder(const std::vector<Zapret*>& services)
{
    s_arg_order.clear();
    s_arg_order.reserve(services.size());

    for (const auto* service : services)
    {
        if (!service)
            continue;

        const auto* shared = dynamic_cast<const SharedZapret*>(service);
        const std::string& key = shared ? shared->info->shared_id : service->id_name;

        registerOrder(key);
    }
}

void Zapret::registerOrder(const std::string& key)
{
    if (key.empty())
        return;

    if (std::find(s_arg_order.begin(), s_arg_order.end(), key) == s_arg_order.end())
        s_arg_order.push_back(key);    
}

std::string Zapret::composeCommandLine()
{
    std::vector<std::string> wf_tcp;
    std::vector<std::string> wf_udp;
    std::vector<std::string> strategies;

    for (const auto& key : s_arg_order)
    {
        auto it = s_entries.find(key);
        if (it == s_entries.end())
            continue;

        appendUniqueOrdered(wf_tcp, it->second.wf_tcp);
        appendUniqueOrdered(wf_udp, it->second.wf_udp);

        for (const auto& strat : it->second.strategies)
        {
            if (!strat.empty())
                strategies.push_back(strat);
        }
    }

    auto joinList = [](const std::vector<std::string>& values)
    {
        std::string result;
        for (size_t i = 0; i < values.size(); ++i)
        {
            if (i > 0)
                result += ',';
            result += values[i];
        }
        return result;
    };

    std::string command;

    if (!wf_tcp.empty())
        command += std::format("--wf-tcp={}", joinList(wf_tcp));

    if (!wf_udp.empty())
    {
        if (!command.empty())
            command += ' ';
        command += std::format("--wf-udp={}", joinList(wf_udp));
    }

    for (size_t i = 0; i < strategies.size(); ++i)
    {
        if (strategies[i].empty())
            continue;

        if (!command.empty())
            command += ' ';

        if (i > 0)
            command += "--new ";

        command += strategies[i];
    }

    return command;
}

void Zapret::rebuildProcess()
{
    std::string command_line = composeCommandLine();

    if (command_line.empty())
    {
        stopProcess();
        s_current_cmdline.clear();
        return;
    }

    bool running = s_process && s_process->isRunning();
    if (running && command_line == s_current_cmdline)
        return;

    stopProcess();

    if (s_executable_path.empty())
        return;

    s_process = std::make_unique<Process>(s_executable_path, command_line);
    s_current_cmdline = command_line;
}

void Zapret::stopProcess()
{
    if (!s_process)
        return;

    s_process->terminate();
    s_process.reset();
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
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=11 --dpi-desync-fake-quic=\"{}\\fakes\\quic_initial_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-tcp=80 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=disorder2 --dpi-desync-split-pos=1,midsld --new ", cur_path, txt);
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync-any-protocol --dpi-desync=ipfrag2 --dpi-desync-ipfrag-pos-udp=8 --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=split --dpi-desync-fooling=md5sig --dpi-desync-split-pos=midsld --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-ttl=1 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --dpi-desync-fake-tls=\"{}\\fakes\\tls_clienthello_www_google_com.bin\"", cur_path, txt, cur_path);

            break;
        }

        case providers_list::PROVIDER_ROST:
        {
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=11 --dpi-desync-fake-quic=\"{}\\fakes\\quic_initial_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-tcp=80 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=disorder2 --dpi-desync-split-pos=1,midsld --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake,split --dpi-desync-ttl=6 --dpi-desync-split-pos=midsld --dpi-desync-fake-tls=\"{}\\fakes\\tls_clienthello_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-fooling=badseq --dpi-desync-split-pos=1,midsld --dpi-desync-fake-tls=\"{}\\fakes\\tls_clienthello_www_google_com.bin\"", cur_path, txt, cur_path);

            break;
        }

        case providers_list::PROVIDER_MTS:
        {
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{}\\fakes\\quic_initial_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-tcp=80 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-fake-tls=\"{}\\fakes\\tls_clienthello_www_google_com.bin\"", cur_path, txt, cur_path);

            break;
        }

        case providers_list::PROVIDER_LOVIT:
        {
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=5 --dpi-desync-fake-quic=\"{}\\fakes\\quic_initial_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync-any-protocol --dpi-desync=ipfrag2 --dpi-desync-ipfrag-pos-udp=16 --new ", cur_path, txt);
            args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=disorder --dpi-desync-fooling=badseq --dpi-desync-split-pos=midsld --dpi-desync-fake-tls=\"{}\\fakes\\tls_clienthello_www_google_com.bin\" --new ", cur_path, txt, cur_path);
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
        args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=11 --dpi-desync-fake-quic=\"{}\\fakes\\quic_initial_www_google_com.bin\" --new ", cur_path, txt, cur_path);
        args += std::format("--filter-udp=443 --filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake,disorder2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig", cur_path, txt);
    }
    else if (id_name == "cloudflare")
    {
        args = std::format("--wf-tcp=443 ");

        args += std::format("--filter-tcp=443 --ipset=\"{}\\{}\" --dpi-desync=multisplit --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=1 --dpi-desync-split-seqovl-pattern=\"{}\\fakes\\tls_clienthello_www_google_com.bin\"", cur_path, txt, cur_path);
    }
    else if (id_name == "akamai")
    {
        //https://suip.biz/?act=all-isp&isp=Akamai%20Connected%20Cloud
        std::string wf_filter = "";
        std::string tcp_filter = "";

        std::set<int> tcp_ports;
        addPorts("443", tcp_ports);
        addPorts("6695-6705", tcp_ports); //Warframe (chat)

        std::string tcp = portsToString(tcp_ports);

        tcp_filter = std::format("--filter-tcp={}", tcp);
        wf_filter = std::format("--wf-tcp={}", tcp);

        args = std::format("{} ", wf_filter);
        args += std::format("--ipset=\"{}\\{}\" {} --dpi-desync=multisplit --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=1 --dpi-desync-split-seqovl-pattern=\"{}\\fakes\\tls_clienthello_www_google_com.bin\"", cur_path, txt, tcp_filter, cur_path);

    }
    else if (id_name == "amazon")
    {
        //https://ip-ranges.amazonaws.com/ip-ranges.json

        std::vector<std::string> spl = tools::split(txt, "|");
        std::string list = spl[0];
        std::string ip = spl[1];

        std::string wf_filter = "";
        std::string tcp_filter = "";
        std::string udp_filter = "";
        switch (vars::amazon_type)
        {
        case 0:
        {
            tcp_filter = "--filter-tcp=21,22,443,1024-65535";
            udp_filter = "--filter-udp=21,22,80,443,1024-65535";
            wf_filter = "--wf-tcp=21,22,80,443,1024-65535 --wf-udp=21,22,80,443,1024-65535";
            break;
        }
        case 1:
        {
            std::set<int> udp_ports;
            addPorts("21,22,80,443", udp_ports);
            addPorts("7770-7790", udp_ports); //DBD
            addPorts("5055-5058, 27000-27003", udp_ports); //REPO (Photon)

            std::set<int> tcp_ports;
            addPorts("21,22,80,443", tcp_ports);
            //addPorts("40002", tcp_ports); //PUBG
            addPorts("9090-9093,19090-19093", tcp_ports); //REPO (Photon)
            addPorts("20000-22000", tcp_ports); //BF6

            std::string tcp = portsToString(tcp_ports);
            std::string udp = portsToString(udp_ports);

            tcp_filter = std::format("--filter-tcp={}", tcp);
            udp_filter = std::format("--filter-udp={}", udp);
            wf_filter = std::format("--wf-tcp={} --wf-udp={}", tcp, udp);

            break;
        }
        case 2:
        {
            tcp_filter = "--filter-tcp=21,22,443";
            udp_filter = "--filter-udp=21,22,80,443";
            wf_filter = "--wf-tcp=21,22,80,443 --wf-udp=21,22,80,443";
            break;
        }
        }

        args = std::format("{} ", wf_filter);

        args += std::format("--hostlist=\"{}\\{}\" --filter-tcp=80 --dpi-desync=fake,fakedsplit --dpi-desync-autottl=2 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=2 --new ", cur_path, list);
        args += std::format("--hostlist=\"{}\\{}\" {} --dpi-desync=fake --dpi-desync-fake-tls-mod=none --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=2 --new ", cur_path, list, tcp_filter);
        args += std::format("--hostlist=\"{}\\{}\" {} --dpi-desync=multisplit --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=2 --dpi-desync-split-seqovl-pattern=\"{}\\fakes\\tls_clienthello_www_google_com.bin\" --new ", cur_path, list, tcp_filter, cur_path);
        
        args += std::format("--ipset=\"{}\\{}\" --filter-tcp=80 --dpi-desync=fake,fakedsplit --dpi-desync-autottl=2 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=2 --new ", cur_path, list);
        args += std::format("--ipset=\"{}\\{}\" {} --dpi-desync=fake --dpi-desync-fake-tls-mod=none --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=2 --new ", cur_path, list, tcp_filter);
        args += std::format("--ipset=\"{}\\{}\" {} --dpi-desync=multisplit --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=2 --dpi-desync-split-seqovl-pattern=\"{}\\fakes\\tls_clienthello_www_google_com.bin\" --new ", cur_path, list, tcp_filter, cur_path);

        args += std::format("--hostlist=\"{}\\{}\" {} --dpi-desync-ttl=8 --dpi-desync-repeats=20 --dpi-desync-fooling=none --dpi-desync-any-protocol=1 --dpi-desync=fake --dpi-desync-cutoff=n10 --dpi-desync-fake-unknown-udp=\"{}\\fakes\\quic_initial_www_google_com.bin\" --new ", cur_path, list, udp_filter, cur_path);
        args += std::format("--ipset=\"{}\\{}\" {} --dpi-desync-ttl=8 --dpi-desync-repeats=20 --dpi-desync-fooling=none --dpi-desync-any-protocol=1 --dpi-desync=fake --dpi-desync-cutoff=n10 --dpi-desync-fake-unknown-udp=\"{}\\fakes\\quic_initial_www_google_com.bin\"", cur_path, ip, udp_filter, cur_path);
        
    }
    else if (id_name == "discord")
    {
        args = std::format("--wf-tcp=80,443,2053,2083,2087,2096,8443 --wf-udp=443,50000-50100 ");

        args += std::format("--filter-tcp=80 --hostlist=\"{}\\{}\" --dpi-desync=fake,multisplit --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --new ", cur_path, txt);
        args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=multisplit --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=1 --dpi-desync-split-seqovl-pattern=\"{}\\fakes\\tls_clienthello_www_google_com.bin\" --new ", cur_path, txt, cur_path);
        args += std::format("--filter-tcp=2053,2083,2087,2096,8443 --hostlist-domains=discord.media --dpi-desync=multisplit --dpi-desync-split-seqovl=681 --dpi-desync-split-pos=1 --dpi-desync-split-seqovl-pattern=\"{}\\fakes\\tls_clienthello_www_google_com.bin\" --new ", cur_path);

        switch (vars::provider)
        {
        case providers_list::PROVIDER_ROST:
        {
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-any-protocol --dpi-desync-repeats=7 --new ", cur_path, txt);
            break;
        }

        default:
        {
            args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-repeats=6 --dpi-desync-fake-quic=\"{}\\fakes\\quic_initial_www_google_com.bin\" --new ", cur_path, txt, cur_path);
            break;
        }
        }

        args += std::format("--filter-udp=50000-50100 --filter-l7=discord,stun --dpi-desync=fake --dpi-desync-fake-quic=\"{}\\fakes\\quic_initial_www_google_com.bin\"", cur_path, cur_path);
    }
    else if (id_name == "twitch")
    {
        args = std::format("--wf-tcp=443 --wf-udp=443 ");

        args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=disorder2 --dpi-desync-split-pos=1,sniext+1,host+1,midsld-2,midsld,midsld+2,endhost-1 --new ", cur_path, txt);
        args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-fooling=md5sig --new ", cur_path, txt);
        args += std::format("--filter-tcp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-fooling=datanoack --new ", cur_path, txt);

        args += std::format("--filter-udp=443 --hostlist=\"{}\\{}\" --dpi-desync=fake --dpi-desync-any-protocol --dpi-desync-repeats=7", cur_path, txt);
    }
    else if (id_name == "telegram")
    {
        args = std::format("--wf-tcp=80,443 ");

        //args += std::format("--ipset=\"{}\\{}\" --filter-tcp=80 --dpi-desync=fake,fakedsplit --dpi-desync-autottl=2 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=2 --new ", cur_path, txt);
        //args += std::format("--ipset=\"{}\\{}\" --filter-tcp=443 --dpi-desync=fake --dpi-desync-fake-tls-mod=none --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-badseq-increment=2 --new ", cur_path, txt);
        //args += std::format("--filter-tcp=80 --ipset=\"{}\\{}\" --dpi-desync=fake,split2 --dpi-desync-autottl=2 --dpi-desync-fooling=md5sig --new ", cur_path, txt);
        args += std::format("--filter-tcp=443 --ipset=\"{}\\{}\" --dpi-desync=multisplit --dpi-desync-split-seqovl=1 --dpi-desync-split-pos=midsld-1", cur_path, txt);
        //args += std::format("--filter-tcp=443 --ipset=\"{}\\{}\" --dpi-desync=fake,multidisorder --dpi-desync-split-pos=1,midsld --dpi-desync-repeats=6 --dpi-desync-fooling=badseq --dpi-desync-fake-tls-mod=rnd,dupsid,sni=www.google.com", cur_path, txt);
    }
}
