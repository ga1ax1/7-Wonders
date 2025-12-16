//
// Created by choyichi on 2025/12/16.
//

#ifndef SEVEN_WONDERS_DUEL_TINYJSON_H
#define SEVEN_WONDERS_DUEL_TINYJSON_H

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <variant>

// 一个极简的 JSON 解析器，仅用于本项目的数据读取
namespace TinyJson {

    struct Value;

    using List = std::vector<Value>;
    using Object = std::map<std::string, Value>;

    struct Value {
        std::variant<std::monostate, int, double, std::string, bool, List, Object> data;

        // 构造函数
        Value() : data(std::monostate{}) {}
        Value(int v) : data(v) {}
        Value(double v) : data(v) {}
        Value(std::string v) : data(v) {}
        Value(const char* v) : data(std::string(v)) {}
        Value(bool v) : data(v) {}
        Value(List v) : data(v) {}
        Value(Object v) : data(v) {}

        // 类型检测与获取
        bool isString() const { return std::holds_alternative<std::string>(data); }
        bool isInt() const { return std::holds_alternative<int>(data); }
        bool isList() const { return std::holds_alternative<List>(data); }
        bool isObject() const { return std::holds_alternative<Object>(data); }

        std::string asString() const {
            if (isString()) return std::get<std::string>(data);
            return "";
        }
        int asInt() const {
            if (isInt()) return std::get<int>(data);
            if (std::holds_alternative<double>(data)) return (int)std::get<double>(data);
            return 0;
        }
        bool asBool() const {
            if (std::holds_alternative<bool>(data)) return std::get<bool>(data);
            return false;
        }
        const List& asList() const {
            static List empty;
            if (isList()) return std::get<List>(data);
            return empty;
        }
        const Object& asObject() const {
            static Object empty;
            if (isObject()) return std::get<Object>(data);
            return empty;
        }

        // 简易下标访问
        const Value& operator[](const std::string& key) const {
            const auto& obj = asObject();
            auto it = obj.find(key);
            if (it != obj.end()) return it->second;
            static Value nullVal;
            return nullVal;
        }

        const Value& operator[](size_t index) const {
            const auto& list = asList();
            if (index < list.size()) return list[index];
            static Value nullVal;
            return nullVal;
        }
    };

    class Parser {
    public:
        static Value parse(const std::string& json) {
            size_t pos = 0;
            return parseValue(json, pos);
        }

    private:
        static void skipWhitespace(const std::string& json, size_t& pos) {
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) {
                pos++;
            }
        }

        static Value parseValue(const std::string& json, size_t& pos) {
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

        static Value parseString(const std::string& json, size_t& pos) {
            pos++; // skip "
            std::string str;
            while (pos < json.size() && json[pos] != '"') {
                str += json[pos++];
            }
            pos++; // skip "
            return Value(str);
        }

        static Value parseNumber(const std::string& json, size_t& pos) {
            size_t start = pos;
            while (pos < json.size() && (json[pos] == '-' || (json[pos] >= '0' && json[pos] <= '9') || json[pos] == '.')) {
                pos++;
            }
            std::string numStr = json.substr(start, pos - start);
            if (numStr.find('.') != std::string::npos) return Value(std::stod(numStr));
            return Value(std::stoi(numStr));
        }

        static Value parseBool(const std::string& json, size_t& pos) {
            if (json.substr(pos, 4) == "true") { pos += 4; return Value(true); }
            if (json.substr(pos, 5) == "false") { pos += 5; return Value(false); }
            return Value(false);
        }

        static Value parseList(const std::string& json, size_t& pos) {
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

        static Value parseObject(const std::string& json, size_t& pos) {
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
    };
}

#endif // SEVEN_WONDERS_DUEL_TINYJSON_H