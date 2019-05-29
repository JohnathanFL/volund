#pragma once

#include <unordered_map>


using AppList = std::unordered_map<std::string, std::string>;




class AppDB {
public:
    AppDB(){}
    void addPath(const std::string& path) {
        addApps(db, path);
    }

    void clear() {
        db.clear();
    }

    unsigned numApps() {
        return db.size();
    }

    operator const AppList&() const {
        return db;
    }

private:
    AppList db;

    bool replace(std::string& str, const std::string& from, const std::string& to);
    void addApps(AppList& apps, std::string path);

};
