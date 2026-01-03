#include <windows.h>
#include <string>
#include <iostream>
#include <chrono>
#include <vector>

void FindFileIterative(const std::string& startDir, const std::string& target) {
    std::vector<std::string> stack;
    stack.push_back(startDir);

    while (!stack.empty()) {
        std::string dir = stack.back();
        stack.pop_back();

        WIN32_FIND_DATAA data;
        HANDLE hFind = FindFirstFileA((dir + "\\*").c_str(), &data);
        if (hFind == INVALID_HANDLE_VALUE) continue;

        do {
            if (!strcmp(data.cFileName, ".") || !strcmp(data.cFileName, ".."))
                continue;

            std::string path = dir + "\\" + data.cFileName;

            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                stack.push_back(path);
            }
            else if (_stricmp(data.cFileName, target.c_str()) == 0) {
                std::cout << "Found: " << path << "\n";
                FindClose(hFind);
                return; // early exit
            }
        } while (FindNextFileA(hFind, &data));

        FindClose(hFind);
    }
}


int main() {
    auto start = std::chrono::steady_clock::now();

    FindFileIterative("C:\\", "photo.jpg");

    auto end = std::chrono::steady_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Search time: " << ms.count() << " ms\n";

    return 0;

}
