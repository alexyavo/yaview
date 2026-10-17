#pragma once

#include <optional>
#include <vector>
#include <string>
#include <mutex>
#include "sqlite3.h"
#include "book.h"

/*
    The database holds the reader's state about documents, keyed by their path: the reading position and
    zoom of every document opened so far (which is also the list of previously opened documents) and the
    layout size of reflowable documents. Annotations, marks included, are in the documents themselves.
*/
class DatabaseManager {
private:
    std::recursive_mutex db_mutex;
    sqlite3* db = nullptr;
    bool run(const char* function_name, const std::string& sql, int (*callback)(void*, int, char**, char**) = nullptr, void* callback_data = nullptr);
    bool create_tables();
public:
    bool open(const std::wstring& database_file_path);

    bool update_document(const std::wstring& path, float zoom_level, float offset_x, float offset_y, const std::wstring& name);
    std::optional<OpenedBookState> select_document(const std::wstring& path);
    // the paths and detected names of the opened documents, most recently opened first
    bool select_documents(std::vector<std::pair<std::wstring, std::wstring>>& out_paths_and_names);
    bool delete_document(const std::wstring& path);
    bool rename_document(const std::wstring& old_path, const std::wstring& new_path);

    bool set_layout_size(const std::wstring& path, float width, float height);
    std::optional<std::pair<float, float>> select_layout_size(const std::wstring& path);
};
