#include "AppDB.hpp"

#include <filesystem>
#include "yaip.hpp"
#include <fstream>

void AppDB::addApps(AppList &apps, std::string path) {
  for (auto& f : std::filesystem::directory_iterator(path)) {
    if (!f.is_regular_file())
      continue;

    INIFile ini;
    auto    file = std::ifstream(f.path());
    ini.parse(file);

    auto& entry = ini["Desktop Entry"];
    auto& name  = entry["Name"];
    auto  exec  = entry["Exec"];
    replace(exec, "%f", "");
    replace(exec, "%F", "");
    replace(exec, "%d", "");
    replace(exec, "%D", "");
    replace(exec, "%u", "");
    replace(exec, "%U", "");
    replace(exec, "%N", "");
    replace(exec, "%k", "");
    replace(exec, "%v", "");

    if (name.empty() || exec.empty())
      continue;

    apps.insert({name, exec});
  }
}



bool AppDB::replace(std::string &str, const std::string &from, const std::string &to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, from.length(), to);
  return true;
}
