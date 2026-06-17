#include "FileEntry.h"

using json = nlohmann::json;
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FileLogs, filename, fullpath, extension)

void FileEntry::IndexDirectory(const fs::path& root)
{    

    for (const auto& entry :
        fs::recursive_directory_iterator(root))
    {
        if (!entry.is_regular_file())
            continue;

        FileLogs file;

        file.filename =
            entry.path().filename().string();

        file.fullpath =
            entry.path().string();

        file.extension =
            entry.path().extension().string();

        g_FileIndex.push_back(file);
    }
}

void FileEntry::StartIndexing()
{
    std::cout << "StartIndexing called\n";
    std::thread([this]()
        {
            IndexDirectory("C:\\Users\\aksha\\OneDrive\\Desktop");
            std::cout << "Finished Desktop\n";
            
            SaveInDB("index.db");
        }).detach();
}


void FileEntry::SaveIndex(const std::string& outputFile)
{

    std::cout << "Saving to: "
        << std::filesystem::current_path()
        << std::endl;
    json data = json::array();

    for (const auto& file : g_FileIndex)
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
        return;

    out << data.dump(4); 
}



void FileEntry::LoadIndex(const std::string& filename)
{
    std::ifstream in(filename);
    if (!in.is_open())
        return;

   
    FileSaxHandler handler;

  

    bool success = json::sax_parse(in, &handler);

    if (success) {
        g_FileIndex = std::move(handler.files);
        std::cout << g_FileIndex.size() << " files loaded from index.";
    }
}


void FileEntry::MigrateJsonToDb()
{
    // This function can be used to migrate from JSON file storage to a database solution in the future.
    // It would read the JSON file, parse it, and then insert the records into a database.

    LoadIndex("C:\\Users\\aksha\\source\\repos\\BetterSearch v0.0.1\\src\\index.json");

    sqlite3* db = nullptr;

    if (sqlite3_open("index.db", &db) != SQLITE_OK)
        return;

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        db,
        "INSERT OR IGNORE INTO files(filename, fullpath, extension) VALUES(?,?,?);",
        -1,
        &stmt,
        nullptr
    );

    if (rc != SQLITE_OK)
    {
        std::cout << "Prepare failed: "
            << sqlite3_errmsg(db)
            << '\n';

        return;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    for (const auto& file : g_FileIndex)
    {
        sqlite3_bind_text(stmt, 1, file.filename.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, file.fullpath.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, file.extension.c_str(), -1, SQLITE_TRANSIENT);

        sqlite3_step(stmt);

        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::cout << "Migrated "
        << g_FileIndex.size()
        << " files to SQLite.\n";


}

void FileEntry::SaveInDB(const std::string& dbFile)
{
    sqlite3* db = nullptr;

    if (sqlite3_open(dbFile.c_str(), &db) != SQLITE_OK)
    {
        std::cout << "Failed to open database: "
            << sqlite3_errmsg(db) << '\n';
        return;
    }

    const char* createTableSQL =
        R"(
        CREATE TABLE IF NOT EXISTS files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            filename TEXT NOT NULL,
            fullpath TEXT NOT NULL UNIQUE,
            extension TEXT
        );
        )";

    if (sqlite3_exec(db, createTableSQL, nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        std::cout << "Failed to create table: "
            << sqlite3_errmsg(db) << '\n';

        sqlite3_close(db);
        return;
    }

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        db,
        "INSERT OR IGNORE INTO files(filename, fullpath, extension) VALUES(?,?,?);",
        -1,
        &stmt,
        nullptr
    );

    if (rc != SQLITE_OK)
    {
        std::cout << "Prepare failed: "
            << sqlite3_errmsg(db) << '\n';

        sqlite3_close(db);
        return;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    for (const auto& file : g_FileIndex)
    {
        sqlite3_bind_text(stmt, 1,
            file.filename.c_str(),
            -1,
            SQLITE_TRANSIENT);

        sqlite3_bind_text(stmt, 2,
            file.fullpath.c_str(),
            -1,
            SQLITE_TRANSIENT);

        sqlite3_bind_text(stmt, 3,
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

}

std::vector<FileLogs> FileEntry::SearchFiles(const std::string& prefix) {
    std::vector<FileLogs> results;

    sqlite3* db = nullptr;
    if (sqlite3_open("C:\\Users\\aksha\\source\\repos\\BetterSearch v0.0.1\\src\\index.db", &db) != SQLITE_OK) {
        return results;
	}

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT filename, fullpath, extension "
        "FROM files "
        "WHERE filename LIKE ? "
        "LIMIT 50;";

    if (sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &stmt,
        nullptr) != SQLITE_OK)
    {
        sqlite3_close(db);
        return results;
    }


    std::string search = prefix + "%";

    sqlite3_bind_text(
        stmt,
        1,
        search.c_str(),
        -1,
        SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
       

        
        FileLogs file;

        file.filename =
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 0));

        file.fullpath =
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 1));

        file.extension =
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 2));

        results.push_back(file);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return results;
}