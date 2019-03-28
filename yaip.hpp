#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>

struct INIFile {
   using Section        = std::unordered_map<std::string, std::string>;
   using SectionList    = std::unordered_map<std::string, Section>;
   SectionList sections = {{"global", {}}};
   Section&    operator[](const std::string& i) { return sections[i]; }

   static void trim(std::string& str) {
      static std::string spaces = " \t\n";
      str                       = str.substr(str.find_first_not_of(spaces));
      while (spaces.find(*str.rbegin()) != str.npos)
         str.pop_back();
   }

   std::string parseSectionHeader(std::string& buff) { return buff.substr(1, buff.size() - 2); }

   std::pair<std::string, std::string> parseKeyValuePair(std::string& buff) {
      std::string key{10}, value{10};

      size_t midpoint = buff.find('=');

      key = buff.substr(buff.find_first_not_of(" \t"), midpoint);
      if (midpoint != buff.size())
         value = buff.substr(midpoint + 1);


      trim(key);
      trim(value);
      return {key, value};
   }

   void parse(std::istream& stream) {
      std::string curSection = "global";
      std::string buff;

      while (getline(stream, buff)) {
          if(buff.empty())
              continue;

         if (buff[0] == '[')
            curSection = parseSectionHeader(buff);
         else {
            auto kvp = parseKeyValuePair(buff);
            if (kvp.first.empty() || kvp.second.empty())
               continue;
            sections[curSection][kvp.first] = kvp.second;
         }

         buff.clear();
      }
   }
};
