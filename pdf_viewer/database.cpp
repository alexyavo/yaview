#include "database.h"
#include "utils.h"

#include <iostream>
#include <sstream>

namespace {

    std::string esc(const std::wstring& text) {
        return utf8_encode(QString::fromStdWString(text).replace("'", "''").toStdWString());
    }

    std::string number(std::optional<float> value) {
        return value ? std::to_string(value.value()) : "NULL";
    }

    std::optional<float> column_float(const char* value) {
        if (value == nullptr) return {};
        return static_cast<float>(std::atof(value));
    }

    int document_callback(void* res_vector, int argc, char** argv, char** col_name) {
        auto* res = static_cast<std::vector<OpenedBookState>*>(res_vector);
        OpenedBookState state;
        state.zoom_level = column_float(argv[0]).value_or(1);
        state.offset_x = column_float(argv[1]).value_or(0);
        state.offset_y = column_float(argv[2]).value_or(0);
        res->push_back(state);
        return 0;
    }

    int paths_and_names_callback(void* res_vector, int argc, char** argv, char** col_name) {
        auto* res = static_cast<std::vector<std::pair<std::wstring, std::wstring>>*>(res_vector);
        res->push_back({ utf8_decode(argv[0]), argv[1] ? utf8_decode(argv[1]) : L"" });
        return 0;
    }

    int layout_size_callback(void* res_vector, int argc, char** argv, char** col_name) {
        auto* res = static_cast<std::vector<std::pair<float, float>>*>(res_vector);
        std::optional<float> width = column_float(argv[0]);
        std::optional<float> height = column_float(argv[1]);
        if (width && height) {
            res->push_back({ width.value(), height.value() });
        }
        return 0;
    }

    int global_mark_callback(void* res_vector, int argc, char** argv, char** col_name) {
        auto* res = static_cast<std::vector<GlobalMark>*>(res_vector);
        GlobalMark mark;
        mark.symbol = argv[0][0];
        mark.document_path = utf8_decode(argv[1]);
        mark.offset_y = column_float(argv[2]).value_or(0);
        mark.offset_x = column_float(argv[3]);
        mark.zoom_level = column_float(argv[4]);
        res->push_back(mark);
        return 0;
    }
}

bool DatabaseManager::run(const char* function_name, const std::string& sql, int (*callback)(void*, int, char**, char**), void* callback_data) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    if (db == nullptr) return false;
    char* error_message = nullptr;
    int error_code = sqlite3_exec(db, sql.c_str(), callback, callback_data, &error_message);
    if (error_code != SQLITE_OK) {
        std::cerr << "SQL error in " << function_name << ": " << (error_message ? error_message : "") << std::endl;
        sqlite3_free(error_message);
        return false;
    }
    return true;
}

bool DatabaseManager::open(const std::wstring& database_file_path) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    std::string path = utf8_encode(database_file_path);
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "could not open the database " << path << ": " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        db = nullptr;
        return false;
    }
    return create_tables();
}

bool DatabaseManager::create_tables() {
    const char* documents_sql = "CREATE TABLE IF NOT EXISTS documents ("
        "path TEXT PRIMARY KEY,"
        "name TEXT,"
        "zoom_level REAL,"
        "offset_x REAL,"
        "offset_y REAL,"
        "last_access_time TEXT,"
        "layout_width REAL,"
        "layout_height REAL);";
    const char* global_marks_sql = "CREATE TABLE IF NOT EXISTS global_marks ("
        "symbol TEXT PRIMARY KEY,"
        "path TEXT,"
        "offset_y REAL,"
        "offset_x REAL,"
        "zoom_level REAL);";
    return run("create_tables", documents_sql) && run("create_tables", global_marks_sql);
}

bool DatabaseManager::update_document(const std::wstring& path, float zoom_level, float offset_x, float offset_y, const std::wstring& name) {
    std::stringstream ss;
    ss << "INSERT INTO documents (path, name, zoom_level, offset_x, offset_y, last_access_time) VALUES ('"
        << esc(path) << "', '" << esc(name) << "', " << zoom_level << ", " << offset_x << ", " << offset_y << ", datetime('now'))"
        << " ON CONFLICT(path) DO UPDATE SET name=excluded.name, zoom_level=excluded.zoom_level, offset_x=excluded.offset_x,"
        << " offset_y=excluded.offset_y, last_access_time=excluded.last_access_time;";
    return run("update_document", ss.str());
}

std::optional<OpenedBookState> DatabaseManager::select_document(const std::wstring& path) {
    std::vector<OpenedBookState> states;
    run("select_document", "SELECT zoom_level, offset_x, offset_y FROM documents WHERE path='" + esc(path) + "' AND zoom_level IS NOT NULL;", document_callback, &states);
    if (states.empty()) return {};
    return states[0];
}

bool DatabaseManager::select_documents(std::vector<std::pair<std::wstring, std::wstring>>& out_paths_and_names) {
    return run("select_documents", "SELECT path, name FROM documents WHERE zoom_level IS NOT NULL ORDER BY datetime(last_access_time) DESC;", paths_and_names_callback, &out_paths_and_names);
}

bool DatabaseManager::delete_document(const std::wstring& path) {
    return run("delete_document", "DELETE FROM documents WHERE path='" + esc(path) + "';");
}

bool DatabaseManager::rename_document(const std::wstring& old_path, const std::wstring& new_path) {
    return run("rename_document", "UPDATE documents SET path='" + esc(new_path) + "' WHERE path='" + esc(old_path) + "';")
        && run("rename_document", "UPDATE global_marks SET path='" + esc(new_path) + "' WHERE path='" + esc(old_path) + "';");
}

bool DatabaseManager::set_layout_size(const std::wstring& path, float width, float height) {
    std::stringstream ss;
    ss << "INSERT INTO documents (path, layout_width, layout_height) VALUES ('" << esc(path) << "', " << width << ", " << height << ")"
        << " ON CONFLICT(path) DO UPDATE SET layout_width=excluded.layout_width, layout_height=excluded.layout_height;";
    return run("set_layout_size", ss.str());
}

std::optional<std::pair<float, float>> DatabaseManager::select_layout_size(const std::wstring& path) {
    std::vector<std::pair<float, float>> sizes;
    run("select_layout_size", "SELECT layout_width, layout_height FROM documents WHERE path='" + esc(path) + "';", layout_size_callback, &sizes);
    if (sizes.empty()) return {};
    return sizes[0];
}

bool DatabaseManager::set_global_mark(const GlobalMark& mark) {
    std::stringstream ss;
    ss << "INSERT OR REPLACE INTO global_marks (symbol, path, offset_y, offset_x, zoom_level) VALUES ('" << mark.symbol << "', '"
        << esc(mark.document_path) << "', " << mark.offset_y << ", " << number(mark.offset_x) << ", " << number(mark.zoom_level) << ");";
    return run("set_global_mark", ss.str());
}

std::optional<GlobalMark> DatabaseManager::select_global_mark(char symbol) {
    std::vector<GlobalMark> marks;
    run("select_global_mark", std::string("SELECT symbol, path, offset_y, offset_x, zoom_level FROM global_marks WHERE symbol='") + symbol + "';", global_mark_callback, &marks);
    if (marks.empty()) return {};
    return marks[0];
}

bool DatabaseManager::select_global_marks(std::vector<GlobalMark>& out_marks) {
    return run("select_global_marks", "SELECT symbol, path, offset_y, offset_x, zoom_level FROM global_marks ORDER BY symbol;", global_mark_callback, &out_marks);
}

bool DatabaseManager::delete_global_mark(char symbol) {
    return run("delete_global_mark", std::string("DELETE FROM global_marks WHERE symbol='") + symbol + "';");
}

bool DatabaseManager::delete_global_marks_of_document(const std::wstring& path) {
    return run("delete_global_marks_of_document", "DELETE FROM global_marks WHERE path='" + esc(path) + "';");
}

bool DatabaseManager::delete_all_global_marks() {
    return run("delete_all_global_marks", "DELETE FROM global_marks;");
}
