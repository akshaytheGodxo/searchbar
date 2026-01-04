#include "IndexFiles.h"

void IndexFiles::addFile(const std::wstring& fileName,
    const std::wstring& fullPath)
{
    IndexMap.emplace(fileName, fullPath);
}

const std::unordered_multimap<std::wstring, std::wstring>&
IndexFiles::getIndex() const
{
    return IndexMap;
}
