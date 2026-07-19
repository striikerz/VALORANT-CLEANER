#pragma warning(disable : 4996)

#include <windows.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <cctype>

struct CleanTask {
    std::string command;
    std::string description;
    bool isAggressive;
};

void setColor(HANDLE hConsole, int color) {
    SetConsoleTextAttribute(hConsole, color);
}

bool IsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin != 0;
}

void executeStep(HANDLE hConsole, const std::string& command, const std::string& description, int& currentStep, int totalSteps) {
    std::cout << "\r\033[K";
    setColor(hConsole, 11);
    std::cout << "[TASK " << currentStep << "/" << totalSteps << "] ";
    setColor(hConsole, 15);
    std::cout << description << "... " << std::flush;

    system(command.c_str());
    Sleep(100);

    setColor(hConsole, 10);
    std::cout << "DONE!" << std::endl;
    currentStep++;
}

void drawProgressBar(HANDLE hConsole, int percentage, const std::string& status) {
    int barWidth = 30;
    int pos = (int)((float)percentage / 100.0f * barWidth);

    std::cout << "\r  [";
    setColor(hConsole, 10);
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "#";
        else if (i == pos) std::cout << "#";
        else std::cout << ".";
    }
    setColor(hConsole, 15);
    std::cout << "] ";
    setColor(hConsole, 14);
    std::cout << percentage << "%  ";
    setColor(hConsole, 13);
    std::cout << status << "   " << std::flush;
}

void cleanHostsFile() {
    const std::string hostsPath = "C:\\Windows\\System32\\drivers\\etc\\hosts";
    std::ifstream in(hostsPath);
    if (!in.is_open()) return;

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("riotgames") == std::string::npos &&
            lower.find("valorant") == std::string::npos &&
            lower.find("vanguard") == std::string::npos) {
            lines.push_back(line);
        }
    }
    in.close();

    std::ofstream out(hostsPath);
    if (out.is_open()) {
        for (const auto& l : lines) out << l << "\n";
        out.close();
    }
}

bool askYesNo(const std::string& question) {
    std::string input;
    while (true) {
        std::cout << question << " (y/n): ";
        std::getline(std::cin, input);
        if (input == "y" || input == "Y") return true;
        if (input == "n" || input == "N") return false;
        std::cout << "Invalid input. Please type 'y' or 'n'.\n";
    }
}

int main() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTitleA("VGC CLEANER - StriikerZ");

    if (!IsAdmin()) {
        setColor(hConsole, 12);
        std::cout << "\n[ERROR] Please run as Administrator!" << std::endl;
        system("pause");
        return 1;
    }

    system("start https://guns.lol/striikerz");

    system("cls");

    setColor(hConsole, 13);
    std::cout << "  +----------------------------------------------+" << std::endl;
    std::cout << "  |";
    setColor(hConsole, 15);
    std::cout << "          VGC CLEANER - StriikerZ              ";
    setColor(hConsole, 13);
    std::cout << "|" << std::endl;
    std::cout << "  |";
    setColor(hConsole, 8);
    std::cout << "      Advanced Vanguard / Riot Trace Remover    ";
    setColor(hConsole, 13);
    std::cout << "|" << std::endl;
    std::cout << "  +----------------------------------------------+" << std::endl;
    std::cout << std::endl;

    setColor(hConsole, 14);
    std::cout << "  Select cleaning mode:\n";
    std::cout << "    [1] Quick Clean (fast, basic traces)\n";
    std::cout << "    [2] Deep Clean (aggressive, includes Event Logs, DNS, ARP, Firewall, Hosts)\n";
    std::cout << "    [3] Custom Clean (choose each step)\n";
    std::cout << "  > ";

    std::string modeInput;
    std::getline(std::cin, modeInput);
    int mode = 0;
    try { mode = std::stoi(modeInput); }
    catch (...) { mode = 0; }

    bool doEventLogs = false, doDNSARP = false, doFirewall = false, doHosts = false, doTakeown = false, doVolumeSerial = false;

    if (mode == 1) {
        doEventLogs = false;
        doDNSARP = false;
        doFirewall = false;
        doHosts = false;
        doTakeown = false;
        doVolumeSerial = false;
    }
    else if (mode == 2) {
        doEventLogs = true;
        doDNSARP = true;
        doFirewall = true;
        doHosts = true;
        doTakeown = true;
        doVolumeSerial = true;
    }
    else if (mode == 3) {
        doEventLogs = askYesNo("  Clear Windows Event Logs (System, Application, Security)?");
        doDNSARP = askYesNo("  Flush DNS cache and clear ARP table?");
        doFirewall = askYesNo("  Remove Riot Games firewall rules?");
        doHosts = askYesNo("  Clean hosts file (remove Riot/Valorant entries)?");
        doTakeown = askYesNo("  Force takeown of Riot folders (for stubborn files)?");
        doVolumeSerial = askYesNo("  Reset Volume Serial Number (requires VolumeId.exe)?");
    }
    else {
        setColor(hConsole, 12);
        std::cout << "\n  [ERROR] Invalid option. Exiting.\n";
        system("pause");
        return 1;
    }

    system("cls");
    setColor(hConsole, 11);
    std::cout << "\n  >> STARTING CLEANING PROCEDURE...\n";
    std::cout << "  >> " << std::string(45, '=') << std::endl << std::endl;

    std::vector<CleanTask> allTasks;

    allTasks.push_back({ "sc stop vgc 2>nul", "Stopping VGC service", false });
    allTasks.push_back({ "sc stop vgk 2>nul", "Stopping VGK driver", false });
    allTasks.push_back({ "sc delete vgc 2>nul", "Removing VGC service", false });
    allTasks.push_back({ "sc delete vgk 2>nul", "Removing VGK driver", false });

    allTasks.push_back({ "rmdir /S /Q \"%LOCALAPPDATA%\\VALORANT\" 2>nul", "Deleting VALORANT cache", false });
    allTasks.push_back({ "rmdir /S /Q \"%LOCALAPPDATA%\\Riot Games\" 2>nul", "Deleting Riot local data", false });
    allTasks.push_back({ "rmdir /S /Q \"%APPDATA%\\Riot Games\" 2>nul", "Deleting Riot roaming data", false });
    allTasks.push_back({ "rmdir /S /Q \"%PROGRAMDATA%\\Riot Games\" 2>nul", "Deleting ProgramData Riot", false });
    allTasks.push_back({ "rmdir /S /Q \"C:\\Program Files\\Riot Vanguard\" 2>nul", "Removing Vanguard folder", false });
    allTasks.push_back({ "rmdir /S /Q \"C:\\Program Files\\Riot Games\" 2>nul", "Removing Program Files Riot", false });
    allTasks.push_back({ "rmdir /S /Q \"C:\\Riot Games\" 2>nul", "Removing root Riot folder", false });

    allTasks.push_back({ "del /f /q \"%LOCALAPPDATA%\\CrashDumps\\*.dmp\" 2>nul", "Cleaning Crash Dumps", false });
    allTasks.push_back({ "del /f /q \"%WINDIR%\\memory.dmp\" 2>nul", "Cleaning Memory Dump", false });
    allTasks.push_back({ "rmdir /S /Q \"%WINDIR%\\Temp\" 2>nul", "Cleaning Windows Temp", false });
    allTasks.push_back({ "rmdir /S /Q \"%LOCALAPPDATA%\\Temp\" 2>nul", "Cleaning User Temp", false });
    allTasks.push_back({ "rmdir /S /Q \"%LOCALAPPDATA%\\D3DSCache\" 2>nul", "Cleaning D3D Shader Cache", false });
    allTasks.push_back({ "rmdir /S /Q \"%PROGRAMDATA%\\NVIDIA Corporation\\NV_Cache\" 2>nul", "Cleaning NVIDIA Cache", false });

    allTasks.push_back({ "reg delete \"HKCU\\Software\\Riot Games\" /f 2>nul", "Removing HKCU Riot key", false });
    allTasks.push_back({ "reg delete \"HKLM\\SOFTWARE\\Riot Games\" /f 2>nul", "Removing HKLM Riot key", false });
    allTasks.push_back({ "reg delete \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\vgk\" /f 2>nul", "Removing VGK driver registry", false });
    allTasks.push_back({ "reg delete \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\vgc\" /f 2>nul", "Removing VGC service registry", false });
    allTasks.push_back({ "reg delete \"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Riot Vanguard\" /f 2>nul", "Removing uninstall entry", false });
    allTasks.push_back({ "reg delete \"HKCR\\riotclient\" /f 2>nul", "Removing riotclient association", false });
    allTasks.push_back({ "reg delete \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\UserAssist\" /f 2>nul", "Cleaning UserAssist history", false });

    allTasks.push_back({ "reg delete \"HKLM\\SOFTWARE\\Microsoft\\Cryptography\" /v MachineGuid /f 2>nul", "Resetting MachineGuid (main)", false });
    allTasks.push_back({ "reg delete \"HKLM\\SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\" /f 2>nul", "Cleaning Hardware Profiles", false });
    allTasks.push_back({ "reg delete \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\" /f 2>nul", "Cleaning ProfileList", false });

    allTasks.push_back({ "del /f /q \"%LOCALAPPDATA%\\IconCache.db\" 2>nul", "Deleting icon cache", false });
    allTasks.push_back({ "del /f /q \"%LOCALAPPDATA%\\Microsoft\\Windows\\Explorer\\thumbcache_*.db\" 2>nul", "Deleting thumbnail cache", false });

    allTasks.push_back({ "del /f /q \"%WINDIR%\\Prefetch\\*.pf\" 2>nul", "Cleaning Prefetch traces", false });

    allTasks.push_back({ "del /f /q \"%WINDIR%\\WindowsUpdate.log\" 2>nul", "Cleaning Windows Update log", false });

    allTasks.push_back({ "netsh winsock reset 2>nul", "Resetting Winsock (Network)", false });

    if (doEventLogs) {
        allTasks.push_back({ "wevtutil cl System 2>nul", "Clearing System Event Log", true });
        allTasks.push_back({ "wevtutil cl Application 2>nul", "Clearing Application Event Log", true });
        allTasks.push_back({ "wevtutil cl Security 2>nul", "Clearing Security Event Log", true });
    }

    if (doDNSARP) {
        allTasks.push_back({ "ipconfig /flushdns 2>nul", "Flushing DNS cache", true });
        allTasks.push_back({ "arp -d * 2>nul", "Clearing ARP table", true });
        allTasks.push_back({ "nbtstat -R 2>nul", "Resetting NetBIOS cache", true });
    }

    if (doFirewall) {
        allTasks.push_back({ "netsh advfirewall firewall delete rule name=\"Riot Games\" 2>nul", "Removing Riot firewall rules", true });
        allTasks.push_back({ "netsh advfirewall firewall delete rule name=\"VALORANT\" 2>nul", "Removing VALORANT firewall rules", true });
    }

    if (doTakeown) {
        allTasks.push_back({ "takeown /f \"C:\\Program Files\\Riot Vanguard\" /r /d y 2>nul", "Taking ownership of Vanguard folder", true });
        allTasks.push_back({ "icacls \"C:\\Program Files\\Riot Vanguard\" /grant administrators:F /t 2>nul", "Granting full control to Vanguard folder", true });
        allTasks.push_back({ "takeown /f \"C:\\Program Files\\Riot Games\" /r /d y 2>nul", "Taking ownership of Riot folder", true });
        allTasks.push_back({ "icacls \"C:\\Program Files\\Riot Games\" /grant administrators:F /t 2>nul", "Granting full control to Riot folder", true });
    }

    if (doVolumeSerial) {
        setColor(hConsole, 14);
        std::cout << "\n  [!] Volume Serial reset requires 'VolumeId.exe' from Microsoft Sysinternals.\n";
        std::cout << "  [!] Please download it and place in the same folder as this cleaner.\n";
        std::cout << "  [!] Skipping Volume Serial reset for now.\n";
    }

    std::vector<CleanTask> finalTasks;
    bool skipAggressive = (mode == 1);

    for (const auto& task : allTasks) {
        if (skipAggressive && task.isAggressive) continue;
        finalTasks.push_back(task);
    }

    bool hostsAdded = false;
    if (doHosts) {
        hostsAdded = true;
    }

    int totalSteps = (int)finalTasks.size() + (hostsAdded ? 1 : 0);
    int step = 1;

    auto startTime = std::chrono::steady_clock::now();

    for (const auto& task : finalTasks) {
        executeStep(hConsole, task.command, task.description, step, totalSteps);
    }

    if (doHosts) {
        std::cout << "\r\033[K";
        setColor(hConsole, 11);
        std::cout << "[TASK " << step << "/" << totalSteps << "] ";
        setColor(hConsole, 15);
        std::cout << "Cleaning hosts file (removing Riot entries)... " << std::flush;
        cleanHostsFile();
        Sleep(100);
        setColor(hConsole, 10);
        std::cout << "DONE!" << std::endl;
        step++;
    }

    std::cout << "\n  ";
    setColor(hConsole, 10);
    for (int i = 0; i <= 100; i += 2) {
        drawProgressBar(hConsole, i, "Finalizing cleaning...");
        Sleep(15);
    }
    std::cout << std::endl << std::endl;

    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();

    setColor(hConsole, 13);
    std::cout << "  +----------------------------------------------+" << std::endl;
    std::cout << "  |";
    setColor(hConsole, 10);
    std::cout << "       CLEANING COMPLETED SUCCESSFULLY!        ";
    setColor(hConsole, 13);
    std::cout << "|" << std::endl;
    std::cout << "  |";
    setColor(hConsole, 14);
    std::cout << "       Time elapsed: " << elapsed << " seconds                  ";
    setColor(hConsole, 13);
    std::cout << "|" << std::endl;
    std::cout << "  |";
    setColor(hConsole, 8);
    std::cout << "     Reboot recommended before running spoofer   ";
    setColor(hConsole, 13);
    std::cout << "|" << std::endl;
    std::cout << "  +----------------------------------------------+" << std::endl;
    std::cout << std::endl;

    setColor(hConsole, 15);
    system("pause");
    system("cls");
    return 0;
}
