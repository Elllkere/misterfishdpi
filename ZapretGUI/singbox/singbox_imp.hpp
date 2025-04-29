#pragma once
#include "singbox.hpp"

void Singbox::restart()
{
	tools::sendStop("sing-box.exe");
	this->start();
}

void Singbox::terminate()
{
	if (!this->isRunning())
		return;

	auto& rules = vars::json_singbox["route"]["rules"];
	for (auto it = rules.begin(); it != rules.end(); ++it)
	{
		if (it->contains(this->key_rule) && (*it)[this->key_rule] == this->domains)
		{
			rules.erase(it);
			break;
		}
	}

	tools::updateSettings(vars::json_singbox, vars::json_singbox_name);
	tools::sendStop("sing-box.exe");

	if (this->status != -1)
		this->status = -1;

	this->startInternal();
}

void Singbox::startInternal()
{
	auto& rules = vars::json_singbox["route"]["rules"];
	if (rules.size() <= 2)
		return;

	std::string cur_path = std::filesystem::current_path().string();

	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;

	std::string command = cur_path + "\\sing-box.exe" + " " + "run -c " + vars::json_singbox_name;

	if (!CreateProcess(NULL, const_cast<char*>(command.c_str()), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		MessageBoxA(0, std::format("Ошибка запуска процесса: {}", GetLastError()).c_str(), 0, 0);
}

void Singbox::writeRule()
{
	json new_rule = {
		{"outbound", "proxy"},
		{this->key_rule, this->domains},
	};

	auto& rules = vars::json_singbox["route"]["rules"];

	if (!this->isRunningInternal())
	{
		rules.insert(rules.end() - 1, new_rule);
	}

	tools::updateSettings(vars::json_singbox, vars::json_singbox_name);
}

void Singbox::start(const std::string& _l)
{
	this->writeRule();

	if (this->status != -1)
		this->status = -1;

	if (this->isPrcsRunning())
		this->restart();
	else
		this->startInternal();
}

bool Singbox::isRunning()
{
	if (!this->isPrcsRunning())
		return false;

	if (this->status == -1)
	{
		bool result = this->status = this->isRunningInternal();
		return result;
	}

	return this->status;
}

bool Singbox::isPrcsRunning()
{
	PROCESSENTRY32   pe32;
	HANDLE         hSnapshot = NULL;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (Process32First(hSnapshot, &pe32))
	{
		do
		{
			std::string str = pe32.szExeFile;
			std::transform(str.begin(), str.end(), str.begin(),
				[](unsigned char c) { return std::tolower(c); });

			if (strcmp(str.c_str(), "sing-box.exe") == 0)
			{
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
				if (hProcess == NULL)
					continue;

				if (hSnapshot != INVALID_HANDLE_VALUE)
					CloseHandle(hSnapshot);

				return true;
			}

		} while (Process32Next(hSnapshot, &pe32));
	}

	if (hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(hSnapshot);

	return false;
}

bool Singbox::isRunningInternal()
{
    json settings;

	std::ifstream ifs(std::filesystem::current_path().string() + "\\" + vars::json_singbox_name);
    if (ifs.is_open())
    {
        ifs >> settings;
        ifs.close();
    }
	else
	{
		tools::sendNotif(std::format(u8"Неудалось прочитать конфиг. er {}", GetLastError()), "", true);
		return false;
	}

	auto& rules = settings["route"]["rules"];
	for (auto it = rules.begin(); it != rules.end(); ++it) 
	{
		if (it->contains(this->key_rule) && (*it)[this->key_rule] == this->domains) 
		{
			return true;
		}
	}

	return false;
}