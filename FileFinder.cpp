#include "FileFinder.h"
#include <windows.h>
#include <vector>
#include <string>
#include <iostream>

void FileFinder::findFilesForMe(const std::wstring& startDir,
    const std::wstring& target)
{
    std::vector<std::wstring> stack;
    stack.push_back(startDir);

    while (!stack.empty()) {
        std::wstring dir = stack.back();
        stack.pop_back();

        WIN32_FIND_DATAW data;
        HANDLE hFind = FindFirstFileW((dir + L"\\*").c_str(), &data);
        if (hFind == INVALID_HANDLE_VALUE)
            continue;

        do {
            if (wcscmp(data.cFileName, L".") == 0 ||
                wcscmp(data.cFileName, L"..") == 0)
                continue;

            std::wstring path = dir + L"\\" + data.cFileName;

            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                stack.push_back(path);
            }
            else if (_wcsicmp(data.cFileName, target.c_str()) == 0) {
                std::wcout << L"Found: " << path << L"\n";
                FindClose(hFind);
                return;
            }

        } while (FindNextFileW(hFind, &data));

        FindClose(hFind);
    }
}
