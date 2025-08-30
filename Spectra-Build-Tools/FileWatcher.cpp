#include "FileWatcher.h"

#ifdef _WIN32
using Callback = std::function<void(const std::string&)>;

FileWatcher::FileWatcher(const std::string& directory, Callback cb)
	: dir(directory), callback(cb)
{
    hDir = CreateFileA(
        directory.c_str(), FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (hDir == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to open directory handle");
    }
}

FileWatcher::~FileWatcher() {
    if (hDir != INVALID_HANDLE_VALUE) {
        CloseHandle(hDir);
    }
}

int counter = 0;

void FileWatcher::poll() {
    char buffer[1024];
    DWORD bytesReturned;

    if (ReadDirectoryChangesW(
        hDir,
        buffer,
        sizeof(buffer),
        FALSE,
        FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
        &bytesReturned,
        NULL,
        NULL)) {

        char* ptr = buffer;

        do {
            FILE_NOTIFY_INFORMATION* fni =
                reinterpret_cast<FILE_NOTIFY_INFORMATION*>(ptr);

            std::wstring wfilename(fni->FileName,
                fni->FileNameLength / sizeof(WCHAR));

            // Convert wide string to UTF-8
            int size_needed = WideCharToMultiByte(CP_UTF8, 0,
                wfilename.c_str(),
                (int)wfilename.size(),
                NULL, 0, NULL, NULL);
            std::string filename(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0,
                wfilename.c_str(),
                (int)wfilename.size(),
                &filename[0], size_needed,
                NULL, NULL);

            // Full path (optional, but safer)
            std::string fullpath = dir + "\\" + filename;

            // Filter: only trigger if it's really TestClass.cpp
            if (filename == "TestClass.cpp") {
                if (counter == 0) {
                    std::cout << "Detected change in " << fullpath << std::endl;
                    callback(fullpath);
                }
                counter++;
                if (counter == 2) {
                    counter = 0;
                }
            }

            // Move to next entry if present
            if (fni->NextEntryOffset == 0) {
                break;
            }
            ptr += fni->NextEntryOffset;

        } while (true);
    }
}
#endif

#ifdef __linux__
#include <sys/inotify.h>

using Callback = std::function<void(const std::string&)>;

FileWatcher::FileWatcher(const std::string& directory, Callback cb)
    : dir(directory), callback(cb) {
    fd = inotify_init1(IN_NONBLOCK);
    if (fd < 0) throw std::runtime_error("inotify_init failed");

    wd = inotify_add_watch(fd, directory.c_str(), IN_MODIFY);
    if (wd < 0) throw std::runtime_error("inotify_add_watch failed");
}

FileWatcher::~FileWatcher() {
    inotify_rm_watch(fd, wd);
    close(fd);
}

void FileWatcher::poll() {
    char buffer[1024];
    int length = read(fd, buffer, sizeof(buffer));
    if (length < 0) return;

    int i = 0;
    while (i < length) {
        struct inotify_event* event = (struct inotify_event*)&buffer[i];
        if (event->len && (event->mask & IN_MODIFY)) {
            callback(dir + "/" + std::string(event->name));
        }
        i += sizeof(struct inotify_event) + event->len;
    }
}
#endif

#ifdef __APPLE__
#include <sys/event.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

using Callback = std::function<void(const std::string&)>;

FileWatcher::FileWatcher(const std::string& file, Callback cb)
    : file(file), callback(cb) {
    fd = open(file.c_str(), O_EVTONLY);
    if (fd < 0) throw std::runtime_error("open failed");

    kq = kqueue();
    if (kq < 0) throw std::runtime_error("kqueue failed");

    EV_SET(&ke, fd, EVFILT_VNODE,
        EV_ADD | EV_ENABLE | EV_CLEAR,
        NOTE_WRITE, 0, NULL);
    if (kevent(kq, &ke, 1, NULL, 0, NULL) < 0) {
        throw std::runtime_error("kevent registration failed");
    }
}

FileWatcher::~FileWatcher() {
    close(fd);
    close(kq);
}

void FileWatcher::poll() {
    struct kevent event;
    timespec timeout = { 0, 0 };
    int nev = kevent(kq, NULL, 0, &event, 1, &timeout);
    if (nev > 0) {
        callback(file);
    }
}

#endif
