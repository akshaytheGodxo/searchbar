#pragma once
#include <windows.h>
#include <string>
#include <unordered_map>

class IndexFiles {
public:
    void addFile(const std::wstring& fileName,
        const std::wstring& fullPath);

    const std::unordered_multimap<std::wstring, std::wstring>& getIndex() const;

private:
    std::unordered_multimap<std::wstring, std::wstring> IndexMap;
};
