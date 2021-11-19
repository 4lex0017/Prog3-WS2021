#include "BoardRepository.hpp"
#include "Core/Exception/NotImplementedException.hpp"
#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include <filesystem>
#include <string.h>

using namespace Prog3::Repository::SQLite;
using namespace Prog3::Core::Model;
using namespace Prog3::Core::Exception;
using namespace std;
using namespace rapidjson;

#ifdef RELEASE_SERVICE
string const BoardRepository::databaseFile = "./data/kanban-board.db";
#else
string const BoardRepository::databaseFile = "../data/kanban-board.db";
#endif

BoardRepository::BoardRepository() : database(nullptr) {

    string databaseDirectory = filesystem::path(databaseFile).parent_path().string();

    if (filesystem::is_directory(databaseDirectory) == false) {
        filesystem::create_directory(databaseDirectory);
    }

    int result = sqlite3_open(databaseFile.c_str(), &database);

    if (SQLITE_OK != result) {
        cout << "Cannot open database: " << sqlite3_errmsg(database) << endl;
    }

    initialize();
}

BoardRepository::~BoardRepository() {
    sqlite3_close(database);
}

void BoardRepository::initialize() {
    int result = 0;
    char *errorMessage = nullptr;

    string sqlCreateTableColumn =
        "create table if not exists column("
        "id integer not null primary key autoincrement,"
        "name text not null,"
        "position integer not null UNIQUE);";

    string sqlCreateTableItem =
        "create table if not exists item("
        "id integer not null primary key autoincrement,"
        "title text not null,"
        "date text not null,"
        "position integer not null,"
        "column_id integer not null,"
        "unique (position, column_id),"
        "foreign key (column_id) references column (id));";

    result = sqlite3_exec(database, sqlCreateTableColumn.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
    result = sqlite3_exec(database, sqlCreateTableItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    // only if dummy data is needed ;)
    //createDummyData();
}

Board BoardRepository::getBoard() {
    throw NotImplementedException();
}

std::vector<Column> BoardRepository::getColumns() {
    throw NotImplementedException();
}

std::optional<Column> BoardRepository::getColumn(int id) {
    int result = 0;
    char *errorMessage = nullptr;

    string sqlSelectColumn = "select * from column where id = " + to_string(id);
    string columns;

    result = sqlite3_exec(database, sqlSelectColumn.c_str(), queryCallback, &columns, &errorMessage);
    handleSQLError(result, errorMessage);

    // we didn't get a response, the column doesn't exist.
    if (columns.empty()) {
        return {};
    };
    columns.pop_back();

    Document document;
    document.Parse(columns.c_str()); // don't think I need error handling here, since the data from the db should be fine
    auto test1 = document.HasParseError();
    auto test = document.HasMember("name");
    auto title = document["name"].GetString();
    auto position = document["position"].GetString();
    Column a = Column(id, title, stoi(position));
    std::vector<Item> b = getItems(id);
    if (!b.empty()) {
        for (auto &item : b) {
            a.addItem(item);
        }
    }

    return a;
}

std::optional<Column> BoardRepository::postColumn(std::string name, int position) {
    string sqlPostItem =
        "INSERT INTO column('name', 'position') "
        "VALUES('" +
        name + "', '" + to_string(position) + "')";

    int result = 0;
    char *errorMessage = nullptr;

    result = sqlite3_exec(database, sqlPostItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    if (SQLITE_OK == result) {
        int columnId = sqlite3_last_insert_rowid(database);
        return Column(columnId, name, position);
    }

    return std::nullopt;
    ;
}

std::optional<Prog3::Core::Model::Column> BoardRepository::putColumn(int id, std::string name, int position) {
    int result = 0;
    char *errorMessage = nullptr;

    getColumn(id);

    std::vector<Item> tempItems = getItems(id);

    string sqlUpdateColumn = "update column "
                             "set name = '" +
                             name + "', position = " + std::to_string(position) +
                             " where id = " + std::to_string(id);

    // time to update the column
    result = sqlite3_exec(database, sqlUpdateColumn.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    // if everything went well we add our items to the column and return that.
    if (result == SQLITE_OK) {
        auto c = Column(id, name, position);
        for (auto &item : tempItems)
            c.addItem(item);

        return c;
    }

    return {};
}

void BoardRepository::deleteColumn(int id) {
    string sqlDeleteItem =
        "DELETE FROM column WHERE id = " +
        to_string(id);

    int result = 0;
    char *errorMessage = nullptr;
    result = sqlite3_exec(database, sqlDeleteItem.c_str(), NULL, 0, &errorMessage);

    handleSQLError(result, errorMessage);
}

std::vector<Item> BoardRepository::getItems(int columnId) {
    int result = 0;
    char *errorMessage = nullptr;

    string sqlSelectItems = "select * from item "
                            "where id = " +
                            std::to_string(columnId); // get all items of the column
    string items = "[";

    result = sqlite3_exec(database, sqlSelectItems.c_str(), queryCallback, &items, &errorMessage);
    handleSQLError(result, errorMessage);

    if (items.size() > 1) // the string is > 1 when the column has items/the callback returned the json string
        items.pop_back(); // if that happens, remove the last ','
    items += "]";         // we now have a valid json string, with items if available...

    Document document;
    document.Parse(items.c_str()); // don't think I need error handling here, since the data from the db should be fine

    std::vector<Item> tempItems;
    // iterate the json object and save the items in a vector.
    for (auto i = 0; i < document.Size(); i++) {
        // they're all saved as strings
        auto id = document[i]["id"].GetString();
        auto title = document[i]["title"].GetString();
        auto date = document[i]["date"].GetString();
        auto position = document[i]["position"].GetString();

        // so we convert them here
        tempItems.push_back(Item(stoi(id), title, stoi(position), date));
    };

    return tempItems;
}

std::optional<Item> BoardRepository::getItem(int columnId, int itemId) {
    int result = 0;
    char *errorMessage = nullptr;

    string sqlSelectItem = "select * from item where id = " + std::to_string(itemId) + " and column_id = " + std::to_string(columnId); // get all items of the column
    string items = "";

    result = sqlite3_exec(database, sqlSelectItem.c_str(), queryCallback, &items, &errorMessage);
    handleSQLError(result, errorMessage);

    if (items.empty()) {
        return {};
    }

    if (items.size() > 1) // the string is > 1 when the column has items/the callback returned the json string
        items.pop_back(); // if that happens, remove the last ','

    Document document;
    document.Parse(items.c_str()); // don't think I need error handling here, since the data from the db should be fine

    auto title = document["title"].GetString();
    auto date = document["date"].GetString();
    auto position = document["position"].GetString();

    return Item(itemId, title, stoi(position), date);
}

std::optional<Item> BoardRepository::postItem(int columnId, std::string title, int position) {
    time_t now = time(0);
    char *datetime = ctime(&now);

    string sqlPostItem =
        "INSERT INTO item ('title', 'date', 'position', 'column_id') "
        "VALUES ('" +
        title + "', '" + datetime + "', " + to_string(position) + ", " + to_string(columnId) + ");";

    int result = 0;
    char *errorMessage = nullptr;

    result = sqlite3_exec(database, sqlPostItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    int itemId = INVALID_ID;
    if (SQLITE_OK == result) {
        itemId = sqlite3_last_insert_rowid(database);
        return Item(itemId, title, position, datetime);
    }
    return std::nullopt;
}

std::optional<Prog3::Core::Model::Item> BoardRepository::putItem(int columnId, int itemId, std::string title, int position) {
    time_t now = time(0);
    char *datetime = ctime(&now);
    char *errorMessage = nullptr;
    int result = 0;

    string sqlSelectItem = "select * from item where id = " + std::to_string(itemId) + " and column_id = " + std::to_string(columnId);
    std::string callback;

    result = sqlite3_exec(database, sqlSelectItem.c_str(), queryCallback, &callback, &errorMessage);
    handleSQLError(result, errorMessage);

    // the item doesn't exist
    if (callback.empty()) {
        return {};
    }

    string sqlDeleteItem = "UPDATE item SET ";

    sqlDeleteItem = sqlDeleteItem + "title = '" + title + "', ";

    sqlDeleteItem = sqlDeleteItem + "positon = " + std::to_string(position) + " ";

    sqlDeleteItem = sqlDeleteItem + "WHERE column_id = " + std::to_string(columnId) + " AND id = " + std::to_string(itemId) + ";";

    result = 0;

    result = sqlite3_exec(database, sqlDeleteItem.c_str(), NULL, 0, &errorMessage);
    printf(errorMessage);
    handleSQLError(result, errorMessage);
    return Item(itemId, title, position, datetime);
}

void BoardRepository::deleteItem(int columnId, int itemId) {
    string sqlDeleteItem =
        "DELETE FROM item WHERE column_id = " +
        to_string(columnId) + " AND id =" + to_string(itemId);

    int result = 0;
    char *errorMessage = nullptr;
    result = sqlite3_exec(database, sqlDeleteItem.c_str(), NULL, 0, &errorMessage);

    handleSQLError(result, errorMessage);
}

void BoardRepository::handleSQLError(int statementResult, char *errorMessage) {

    if (statementResult != SQLITE_OK) {
        cout << "SQL error: " << errorMessage << endl;
        sqlite3_free(errorMessage);
    }
}

void BoardRepository::createDummyData() {

    cout << "creatingDummyData ..." << endl;

    int result = 0;
    char *errorMessage;
    string sqlInsertDummyColumns =
        "insert into column (name, position)"
        "VALUES"
        "(\"prepare\", 1),"
        "(\"running\", 2),"
        "(\"finished\", 3);";

    result = sqlite3_exec(database, sqlInsertDummyColumns.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    string sqlInserDummyItems =
        "insert into item (title, date, position, column_id)"
        "VALUES"
        "(\"in plan\", date('now'), 1, 1),"
        "(\"some running task\", date('now'), 1, 2),"
        "(\"finished task 1\", date('now'), 1, 3),"
        "(\"finished task 2\", date('now'), 2, 3);";

    result = sqlite3_exec(database, sqlInserDummyItems.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
}

/*
  I know source code comments are bad, but this one is to guide you through the use of sqlite3_exec() in case you want to use it.
  sqlite3_exec takes a "Callback function" as one of its arguments, and since there are many crazy approaches in the wild internet,
  I want to show you how the signature of this "callback function" may look like in order to work with sqlite3_exec()
*/
int BoardRepository::queryCallback(void *data, int numberOfColumns, char **fieldValues, char **columnNames) {
    if (data) {
        auto &return_value = *static_cast<std::string *>(data);

        return_value += "{";

        for (int i = 0; i < numberOfColumns; ++i) {
            return_value += "\"" + std::string(columnNames[i]) + "\": ";
            return_value += "\"" + std::string(fieldValues[i]) + "\"";

            if (i != numberOfColumns - 1)
                return_value += ",";
        }

        return_value += "},";
    }

    return 0;
}
