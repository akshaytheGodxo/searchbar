#include "FileFinder.h"

FileFinder::FileFinder(const std::wstring& directoryPath) : path(directoryPath) {

}

std::vector<std::wstring> FileFinder::findFiles() const {
    std::vector<std::wstring> files;
    WIN32_FIND_DATAW data;

    HANDLE h = FindFirstFileW(path.c_str(), &data);
    if (h == INVALID_HANDLE_VALUE)
        return files;

    do {
        if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            files.emplace_back(data.cFileName);
        }
    } while (FindNextFileW(h, &data));

    FindClose(h);
    return files;
}