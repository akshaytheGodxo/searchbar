#include "IndexFiles.h"
#include <iostream>
IndexFiles::IndexFiles() {

    
}
void IndexFiles::openFile() {
    if (!out.is_open()) {
        out.open(
            L"C:\\Users\\aksha\\OneDrive\\Desktop\\SearchBarM\\index.db",
            std::ios::out | std::ios::app   // NO truncation
        );
    }
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
    std::wcout << L"Loading from the disksldfjgkl\n";
    
    std::wifstream in(L"C:\\Users\\aksha\\OneDrive\\Desktop\\SearchBarM\\index.db");
    if (!in.is_open()) {
        std::cout << "HERE\n";
        return;
    }
    std::wstring line;
	
    while (std::getline(in, line)) {
       
        size_t sep = line.find(L'|');
        if (sep == std::wstring::npos) continue;
        IndexMap.emplace(line.substr(0, sep), line.substr(sep + 1));
    }
    if (IndexMap.size() > 0) {
        std::wcout << L"Loaded " << IndexMap.size() << L" entries from disk.\n";
    }
    else {
		std::wcout << L"No entries found in index file.\n";
        std::wcout << IndexMap.size() << "\n";
    }
}

bool IndexFiles::_isEmpty() const {
    return IndexMap.empty();
}

bool IndexFiles::diskIndexExists() const {
    std::wifstream in(L"C:\\Users\\aksha\\OneDrive\\Desktop\\SearchBarM\\index.db");
    return in.good();
}