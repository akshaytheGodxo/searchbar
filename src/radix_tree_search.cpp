#include <iostream>
#include <filesystem>
#include <minwinbase.h>
#include <unordered_map>
#include <memory>
#include <vector>
#include <fstream>
#include "sqllite/sqlite3.h"
#include "nlohmann/json.h"
#include <WinBase.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

struct FileInfo
{
    std::string filename;
    std::string fullpath;
    std::string extension;
    std::string file_id;
    
};

struct RadixNode
{
    std::unordered_map<char, std::unique_ptr<RadixNode>> children;
    std::vector<FileInfo> files;
    bool isEnd = false;
};

class RadixTree
{
private:
    std::unique_ptr<RadixNode> root;

public:
    RadixTree()
    {
        root = std::make_unique<RadixNode>();
    }

    void Insert(const FileInfo& file)
    {
        RadixNode* current = root.get();

        for (char c : file.filename)
        {
            if (!current->children.count(c))
            {
                current->children[c] =
                    std::make_unique<RadixNode>();
            }

            current = current->children[c].get();
        }

        current->isEnd = true;
        current->files.push_back(file);
    }

    void SearchPrefix(
        const std::string& prefix,
        std::vector<FileInfo>& results)
    {
        RadixNode* current = root.get();

        for (char c : prefix)
        {
            if (!current->children.count(c))
                return;

            current = current->children[c].get();
        }

        Collect(current, results);
    }

    void Collect(
        RadixNode* node,
        std::vector<FileInfo>& results)
    {
        if (!node)
            return;

        for (const auto& file : node->files)
            results.push_back(file);

        for (auto& child : node->children)
            Collect(child.second.get(), results);
    }
};

void SaveIndexToDB(
    const std::vector<FileInfo>& files,
    const std::string& dbPath)
{

    sqlite3* db = nullptr;

    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK)
    {
        std::cout << "Failed to open database: "
            << sqlite3_errmsg(db)
            << '\n';
        return;
    }

    const char* createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            filename TEXT NOT NULL,
            fullpath TEXT NOT NULL UNIQUE,
            extension TEXT,
			file_id VARCHAR(255)
        );
    )";

    if (sqlite3_exec(
        db,
        createTableSQL,
        nullptr,
        nullptr,
        nullptr) != SQLITE_OK)
    {
        std::cout << "Failed to create table: "
            << sqlite3_errmsg(db)
            << '\n';

        sqlite3_close(db);
        return;
    }

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        db,
        "INSERT OR IGNORE INTO files "
        "(filename, fullpath, extension) "
        "VALUES (?, ?, ?);",
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        std::cout << "Prepare failed: "
            << sqlite3_errmsg(db)
            << '\n';

        sqlite3_close(db);
        return;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    for (const auto& file : files)
    {
        sqlite3_bind_text(
            stmt,
            1,
            file.filename.c_str(),
            -1,
            SQLITE_TRANSIENT);

        sqlite3_bind_text(
            stmt,
            2,
            file.fullpath.c_str(),
            -1,
            SQLITE_TRANSIENT);

        sqlite3_bind_text(
            stmt,
            3,
            file.extension.c_str(),
            -1,
            SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE)
        {
            std::cout << "Insert failed: "
                << sqlite3_errmsg(db)
                << '\n';
        }

        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::cout
        << "Saved "
        << files.size()
        << " files to "
        << dbPath
        << '\n';
}

void TraverseDirectory(
    const fs::path& root,
    RadixTree& tree,
    std::vector<FileInfo>& allFiles)
{
    std::error_code ec;
    auto it = fs::recursive_directory_iterator(
        root,
        fs::directory_options::skip_permission_denied,
        ec);

    if (ec)
    {
        std::cout << "Failed to open directory: " << ec.message() << '\n';
        return;
    }

    for (; it != fs::recursive_directory_iterator(); it.increment(ec))
    {
        if (ec)
        {
            ec.clear();  // clear and keep going
            continue;
        }

        const auto& entry = *it;

        if (!entry.is_regular_file(ec) || ec)
        {
            ec.clear();
            continue;
        }

        FileInfo file;
        file.filename = entry.path().filename().string();
        file.fullpath = entry.path().string();
        file.extension = entry.path().extension().string();

        tree.Insert(file);
        allFiles.push_back(file);
    }
}

void SaveIndex(
    const std::vector<FileInfo>& files,
    const std::string& outputFile)
{
    json data = json::array();

    for (const auto& file : files)
    {
        data.push_back(
            {
                {"filename", file.filename},
                {"fullpath", file.fullpath},
                {"extension", file.extension}
            });
    }

    std::ofstream out(outputFile);

    if (!out.is_open())
    {
        std::cout << "Failed to create file\n";
        return;
    }

    out << data.dump(4);

    std::cout
        << "Saved "
        << files.size()
        << " files to "
        << outputFile
        << '\n';
}

int main()
{
    RadixTree tree;
    std::vector<FileInfo> allFiles;

    fs::path desktop =
        "C:\\Users\\aksha\\OneDrive\\Desktop";

    TraverseDirectory(desktop, tree, allFiles);

    SaveIndexToDB(
        allFiles,
        "C:\\Users\\aksha\\source\\repos\\BetterSearch v0.0.1\\src\\index.db");

    return 0;
}
