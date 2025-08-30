#pragma once
#include <string>
#include <functional>
#include <windows.h>
#include <iostream>
#include <vector>
#include <chrono>

class FileWatcher {
public:
    using Callback = std::function<void(const std::string&)>;

    FileWatcher(const std::string& directory, Callback cb);
    ~FileWatcher();

    void poll();

#ifdef _WIN32
private:
    HANDLE hDir;
    std::string dir;
    Callback callback;
};
#endif

#ifdef __linux__
private:
    int fd;
    int wd;
    std::string dir;
    Callback callback;
};
#endif

#ifdef __APPLE__
private:
    int fd;
    int kq;
    struct kevent ke;
    std::string file;
    Callback callback;
};
#endif