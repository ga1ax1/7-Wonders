//
// Created by choyichi on 2025/12/16.
//

#ifndef SEVEN_WONDERS_DUEL_TINYJSON_H
#define SEVEN_WONDERS_DUEL_TINYJSON_H

#include <string>
#include <vector>
#include <map>
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

        std::string asString() const;
        int asInt() const;
        bool asBool() const;
        const List& asList() const;
        const Object& asObject() const;

        // 简易下标访问
        const Value& operator[](const std::string& key) const;
        const Value& operator[](size_t index) const;
    };

    class Parser {
    public:
        static Value parse(const std::string& json);

    private:
        static void skipWhitespace(const std::string& json, size_t& pos);
        static Value parseValue(const std::string& json, size_t& pos);
        static Value parseString(const std::string& json, size_t& pos);
        static Value parseNumber(const std::string& json, size_t& pos);
        static Value parseBool(const std::string& json, size_t& pos);
        static Value parseList(const std::string& json, size_t& pos);
        static Value parseObject(const std::string& json, size_t& pos);
    };
}

#endif // SEVEN_WONDERS_DUEL_TINYJSON_H
