#pragma once
#include <windows.h>
#include <string>
#include <unordered_map>
#include <fstream>

class IndexFiles {
public:
    IndexFiles();


    void addFile(const std::wstring& fileName,
        const std::wstring& fullPath);
    void loadFromDisk();
    void openFile();
    const std::unordered_multimap<std::wstring, std::wstring>& getIndex() const;
    bool _isEmpty() const;
    bool diskIndexExists() const;

private:
    std::unordered_multimap<std::wstring, std::wstring> IndexMap;
    std::wofstream out;
    
};
