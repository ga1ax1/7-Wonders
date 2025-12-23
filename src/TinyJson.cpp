#include "TinyJson.h"

namespace TinyJson {

    // ========================================================== 
    //  Value
    // ========================================================== 

    std::string Value::asString() const {
        if (isString()) return std::get<std::string>(data);
        return "";
    }

    int Value::asInt() const {
        if (isInt()) return std::get<int>(data);
        if (std::holds_alternative<double>(data)) return (int)std::get<double>(data);
        return 0;
    }

    bool Value::asBool() const {
        if (std::holds_alternative<bool>(data)) return std::get<bool>(data);
        return false;
    }

    const List& Value::asList() const {
        static List empty;
        if (isList()) return std::get<List>(data);
        return empty;
    }

    const Object& Value::asObject() const {
        static Object empty;
        if (isObject()) return std::get<Object>(data);
        return empty;
    }

    const Value& Value::operator[](const std::string& key) const {
        const auto& obj = asObject();
        auto it = obj.find(key);
        if (it != obj.end()) return it->second;
        static Value nullVal;
        return nullVal;
    }

    const Value& Value::operator[](size_t index) const {
        const auto& list = asList();
        if (index < list.size()) return list[index];
        static Value nullVal;
        return nullVal;
    }

    // ========================================================== 
    //  Parser
    // ========================================================== 

    Value Parser::parse(const std::string& json) {
        size_t pos = 0;
        return parseValue(json, pos);
    }

    void Parser::skipWhitespace(const std::string& json, size_t& pos) {
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) {
            pos++;
        }
    }

    Value Parser::parseValue(const std::string& json, size_t& pos) {
        skipWhitespace(json, pos);
        if (pos >= json.size()) return Value();

        char c = json[pos];
        if (c == '"') return parseString(json, pos);
        if (c == '{') return parseObject(json, pos);
        if (c == '[') return parseList(json, pos);
        if (c == 't' || c == 'f') return parseBool(json, pos);
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(json, pos);

        return Value();
    }

    Value Parser::parseString(const std::string& json, size_t& pos) {
        pos++; // skip "
        std::string str;
        while (pos < json.size() && json[pos] != '"') {
            str += json[pos++];
        }
        pos++; // skip "
        return Value(str);
    }

    Value Parser::parseNumber(const std::string& json, size_t& pos) {
        size_t start = pos;
        while (pos < json.size() && (json[pos] == '-' || (json[pos] >= '0' && json[pos] <= '9') || json[pos] == '.')) {
            pos++;
        }
        std::string numStr = json.substr(start, pos - start);
        if (numStr.find('.') != std::string::npos) return Value(std::stod(numStr));
        return Value(std::stoi(numStr));
    }

    Value Parser::parseBool(const std::string& json, size_t& pos) {
        if (json.substr(pos, 4) == "true") { pos += 4; return Value(true); }
        if (json.substr(pos, 5) == "false") { pos += 5; return Value(false); }
        return Value(false);
    }

    Value Parser::parseList(const std::string& json, size_t& pos) {
        pos++; // skip [
        List list;
        while (pos < json.size()) {
            skipWhitespace(json, pos);
            if (json[pos] == ']') { pos++; break; }
            list.push_back(parseValue(json, pos));
            skipWhitespace(json, pos);
            if (json[pos] == ',') pos++;
        }
        return Value(list);
    }

    Value Parser::parseObject(const std::string& json, size_t& pos) {
        pos++; // skip {
        Object obj;
        while (pos < json.size()) {
            skipWhitespace(json, pos);
            if (json[pos] == '}') { pos++; break; }

            std::string key = parseString(json, pos).asString();
            skipWhitespace(json, pos);
            if (json[pos] == ':') pos++; // skip :

            obj[key] = parseValue(json, pos);

            skipWhitespace(json, pos);
            if (json[pos] == ',') pos++;
        }
        return Value(obj);
    }

}