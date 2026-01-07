#include "IndexFiles.h"
#include <iostream>
IndexFiles::IndexFiles() {

    out.open(L"C:\\Users\\aksha\\OneDrive\\Desktop\\SearchBarM\\index.db");
}

void IndexFiles::addFile(const std::wstring& fileName,
    const std::wstring& fullPath)
{
    IndexMap.emplace(fileName, fullPath);

    if (out.is_open()) {
        out << fileName << L"|" << fullPath << L"\n";
        out.flush();
    }
}

const std::unordered_multimap<std::wstring, std::wstring>&
IndexFiles::getIndex() const
{
    return IndexMap;
}

void IndexFiles::loadFromDisk() {
    std::wcout << L"Loading in the disks\n";
    std::wifstream in(L"C:\\Users\\aksha\\OneDrive\\Desktop\\SearchBarM\\index.db");
    if (!in.is_open()) {
        return;
    }
    std::wstring line;

    while (std::getline(in, line)) {
        size_t sep = line.find(L'|');
        if (sep == std::wstring::npos) continue;
        IndexMap.emplace(line.substr(0, sep), line.substr(sep + 1));
    }
}

bool IndexFiles::_isEmpty() const {
    return IndexMap.empty();
}

bool IndexFiles::diskIndexExists() const {
    std::wifstream in(L"C:\\Users\\aksha\\OneDrive\\Desktop\\SearchBarM\\index.db");
    return in.good();
}