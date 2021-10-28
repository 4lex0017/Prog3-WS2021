#define RAPIDJSON_ASSERT(x)

#include "JsonParser.hpp"
#include "Core/Exception/NotImplementedException.hpp"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace Prog3::Api::Parser;
using namespace Prog3::Core::Model;
using namespace Prog3::Core::Exception;
using namespace rapidjson;
using namespace std;

string JsonParser::convertToApiString(Board &board) {
    throw NotImplementedException();
}

string JsonParser::convertToApiString(Column &column) {
    throw NotImplementedException();
}

string JsonParser::convertToApiString(std::vector<Column> &columns) {
    throw NotImplementedException();
}

string JsonParser::convertToApiString(Item &item) {
    throw NotImplementedException();
}

string JsonParser::convertToApiString(std::vector<Item> &items) {
    throw NotImplementedException();
}

std::optional<Column> JsonParser::convertColumnToModel(int columnId, std::string &request) {
    std::string itemstring;
    std::string name;
    int pos;

    std::size_t temp = request.find_first_of("\"name\"; ");
    name = request.substr(temp);
    std::size_t temp = request.find_first_of(",");
    name.erase(temp);

    std::size_t temp = request.find_first_of("\"position\"; ");
    std::string posstr = request.substr(temp);
    std::size_t temp = request.find_first_of(",");
    posstr.erase(temp);
    pos = std::stoi(posstr);

    Column col = Column(columnId, name, pos);
    return col;
}

std::optional<Item> JsonParser::convertItemToModel(int itemId, std::string &request) {
    std::string itemtitle;
    std::string itemtime;
    int itempos;
    int itemid;
}
