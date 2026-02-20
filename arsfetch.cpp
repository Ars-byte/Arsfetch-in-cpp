#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <pwd.h>
namespace fs = std::filesystem;
using namespace std;
void portada() {
    cout << R"(
  /\___/\
  )     (
 =\     /=
   )   (
  /     \
  )     (
 /       \
 \       /
  \__ __/
    ))
   //
  ((
   \)
)" << endl;
}

string getUptime() {
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        return "Not avaible";
    }
    long seconds = info.uptime;
    const int day_in_seconds = 86400;
    const int hour_in_seconds = 3600;
    
    int days = seconds / day_in_seconds;
    seconds %= day_in_seconds;
    int hours = seconds / hour_in_seconds;
    seconds %= hour_in_seconds;
    int minutes = seconds / 60;
    seconds %= 60;
    
    stringstream result;
    if (days > 0) result << days << " days, ";
    if (hours > 0) result << hours << " hours, ";
    if (minutes > 0) result << minutes << " minutes, ";
    result << seconds << " seconds";
    
    return result.str();
}

string find_value_in_proc(const string& file_path, const string& key_name) {
    ifstream file(file_path);
    if (!file) return "";

    string line;
    while (getline(file, line)) {
        if (line.rfind(key_name, 0) == 0) {
            return line.substr(line.find(':') + 2);
        }
    }
    return "";
}

string getCpuInfo() {
    string info = find_value_in_proc("/proc/cpuinfo", "model name");
    return info.empty() ? "Unknown" : info;
}

string getShell() {
    if (const char* shell = getenv("SHELL")) {
        return shell;
    }
    if (const struct passwd* pw = getpwuid(getuid())) {
        if (pw->pw_shell) return pw->pw_shell;
    }
    return "Unknown";
}


pair<double, double> getMemoryInfo() {
    ifstream meminfo_file("/proc/meminfo");
    if (!meminfo_file) return {0.0, 0.0};

    long memTotal = 0, memAvailable = 0;
    string line;
    while (getline(meminfo_file, line) && (memTotal == 0 || memAvailable == 0)) {
        stringstream ss(line);
        string key;
        long value;
        ss >> key >> value;
        if (key == "MemTotal:") memTotal = value;
        else if (key == "MemAvailable:") memAvailable = value;
    }

    if (memTotal == 0) return {0.0, 0.0};

    double totalGB = static_cast<double>(memTotal) / (1024.0 * 1024.0);
    double usagePercent = 100.0 * (memTotal - memAvailable) / memTotal;

    return {totalGB, usagePercent};
}


string getCpuTemperature() {
    const string hwmon_path = "/sys/class/hwmon/";
    if (fs::exists(hwmon_path)) {
        for (const auto& entry : fs::directory_iterator(hwmon_path)) {
            for (const auto& file : fs::directory_iterator(entry.path())) {
                const string filename = file.path().filename().string();
                if (filename.rfind("temp", 0) == 0 && filename.find("_input") != string::npos) {
                    ifstream temp_file(file.path());
                    string value_str;
                    if (temp_file && getline(temp_file, value_str)) {
                        try {
                            double temp = stod(value_str) / 1000.0;
                            stringstream ss;
                            ss << fixed << setprecision(1) << temp << "°C";
                            return ss.str();
                        } catch (...) { continue; }
                    }
                }
            }
        }
    }
    return "Not avaible";
}

string getBatteryInfo() {
    const string power_supply_path = "/sys/class/power_supply/";
    if (fs::exists(power_supply_path)) {
        for (const auto& entry : fs::directory_iterator(power_supply_path)) {
            if (fs::exists(entry.path() / "capacity")) {
                ifstream capacity_file(entry.path() / "capacity");
                ifstream status_file(entry.path() / "status");

                if (capacity_file && status_file) {
                    string capacity, status;
                    getline(capacity_file, capacity);
                    getline(status_file, status);

                    if (status == "Charging") {
                        return capacity + "% (Cargando)";
                    } else if (status == "Full") {
                        return capacity + "% (Completa)";
                    }
                    return capacity + "%";
                }
            }
        }
    }
    return "Not avaible";
}


int main() {
    portada();
    cout << "\nArsFetch" << endl;
    cout << "--------" << endl;

    cout << "Uptime            : " << getUptime() << endl;
    const auto now = chrono::system_clock::now();
    const time_t now_time = chrono::system_clock::to_time_t(now);
    const tm local_tm = *localtime(&now_time);
    cout << "            : " << put_time(&local_tm, "%Y-%m-%d %H:%M:%S") << endl;

    struct utsname sysinfo_data;
    if (uname(&sysinfo_data) == 0) {
        cout << "Operative system:" << sysinfo_data.sysname << " " << sysinfo_data.release << endl;
        cout << "Name of system: " << sysinfo_data.nodename << endl;
        cout << "Architecture: " << sysinfo_data.machine << endl;
    }

    const auto [ram_total, ram_usage] = getMemoryInfo();
    cout << "RAM usage       : " << fixed << setprecision(1) << ram_usage << "% de " << setprecision(2) << ram_total << " GB" << endl;
    cout << "Processor        : " << getCpuInfo() << endl;
    cout << "Temperature CPU   : " << getCpuTemperature() << endl;
    
    cout << "Battery           : " << getBatteryInfo() << endl;
    cout << "Shell             : " << getShell() << endl;

    cout << "\nBye bye :)" << endl;
    return 0;
}
