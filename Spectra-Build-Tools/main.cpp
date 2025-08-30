#include "main.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <windows.h>
#include <fstream>

#include "FileWatcher.h"

// Utility: copy file to another path
void copyFile(const std::string& src, const std::string& dst) {
    std::ifstream in(src, std::ios::binary);
    std::ofstream out(dst, std::ios::binary);
    out << in.rdbuf();
}

using GetValueFn = int (*)();

int main() {
    std::string sourceDir = ".";                 // watch current dir
    std::string libName = "runtime/TestClass.dll";
    std::string hotName = "runtime/TestClass_hot.dll";

    HMODULE lib = nullptr;

    FileWatcher watcher(sourceDir, [&](const std::string& changedFile) {
        std::cout << changedFile << std::endl;
        if (changedFile == ".\\TestClass.cpp") {
            std::cout << "[SRC] Detected change in " << changedFile << ". Rebuilding...\n";
            system("build.bat");

            std::cout << "[SRC] Reloading DLL...\n";
            if (lib) {
                FreeLibrary(lib);
                lib = nullptr;
                //std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // Copy DLL -> temporary file
            copyFile(libName, hotName);

            lib = LoadLibraryA(hotName.c_str());
            if (!lib) {
                std::cerr << "Failed to load DLL!\n";
                return;
            }

            auto getValue = reinterpret_cast<GetValueFn>(GetProcAddress(lib, "getValue"));
            if (getValue) {
                std::cout << "getValue() -> " << getValue() << "\n";
            }
            else {
                std::cerr << "Failed to find symbol 'getValue'\n";
            }
        }
	});

    std::cout << "[SRC] Watching for changes...\n";

    // Main loop
    while (true) {
        watcher.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  
    // <<<<<<<<<<< -------------------------------------------------------------------- >>>>>>>>>
    // 
    //std::string sourceFile = "TestClass.cpp";
    //std::string libName = "runtime/TestClass.dll";
    //std::string hotName = "runtime/TestClass_hot.dll";

    //auto lastWrite = std::filesystem::last_write_time(sourceFile);
    //HMODULE lib = nullptr;

    //while (true) {
    //    // Poll for changes in the source file
    //    auto currentWrite = std::filesystem::last_write_time(sourceFile);
    //    if (currentWrite != lastWrite) {
    //        lastWrite = currentWrite;

    //        std::cout << "[SRC] Detected change in " << sourceFile << ". Rebuilding...\n";
    //        system("build.bat");

    //        std::cout << "[SRC] Reloading DLL...\n";
    //        if (lib) {
    //            FreeLibrary(lib);
    //            lib = nullptr;
    //            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //        }

    //        // Copy compiled DLL into a temp file before loading
    //        copyFile(libName, hotName);

    //        lib = LoadLibraryA(hotName.c_str());
    //        if (!lib) {
    //            std::cerr << "Failed to load DLL!\n";
    //            continue;
    //        }

    //        auto getValue = reinterpret_cast<GetValueFn>(GetProcAddress(lib, "getValue"));
    //        if (getValue) {
    //            std::cout << "getValue() -> " << getValue() << "\n";
    //        }
    //        else {
    //            std::cerr << "Failed to find symbol 'getValue'\n";
    //        }
    //    }

    //    std::this_thread::sleep_for(std::chrono::seconds(1));
    //}
}
