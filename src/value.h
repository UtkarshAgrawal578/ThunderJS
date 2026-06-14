#ifndef VALUE_H
#define VALUE_H
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <limits>
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include "ast.h"

struct Value;
using ValPtr = std::shared_ptr<Value>;
using NativeFunc = std::function<ValPtr(std::vector<ValPtr>)>;

enum class ValType { Undefined, Null, Boolean, Number, String, Object, Array, Function, NativeFunction };

struct JSObject;
using ObjPtr = std::shared_ptr<JSObject>;

struct Value {
    ValType type = ValType::Undefined;
    double num = 0;
    std::string str;
    bool boolean = false;
    ObjPtr obj; // for Object, Array, Function
    NativeFunc native;
    NodePtr funcBody;
    std::vector<std::string> params;
    std::shared_ptr<struct Environment> closure;

    Value() : type(ValType::Undefined) {}
    static ValPtr makeUndefined() { return std::make_shared<Value>(); }
    static ValPtr makeNull() { auto v = std::make_shared<Value>(); v->type = ValType::Null; return v; }
    static ValPtr makeNumber(double n) { auto v = std::make_shared<Value>(); v->type = ValType::Number; v->num = n; return v; }
    static ValPtr makeString(const std::string& s) { auto v = std::make_shared<Value>(); v->type = ValType::String; v->str = s; return v; }
    static ValPtr makeBool(bool b) { auto v = std::make_shared<Value>(); v->type = ValType::Boolean; v->boolean = b; return v; }
    static ValPtr makeObject(ObjPtr o) { auto v = std::make_shared<Value>(); v->type = ValType::Object; v->obj = o; return v; }
    static ValPtr makeArray(ObjPtr o) { auto v = std::make_shared<Value>(); v->type = ValType::Array; v->obj = o; return v; }
    static ValPtr makeFunction(NodePtr body, std::vector<std::string> ps, std::shared_ptr<struct Environment> env, std::string name="");
    static ValPtr makeNative(NativeFunc f) { auto v = std::make_shared<Value>(); v->type = ValType::NativeFunction; v->native = f; return v; }

    bool isTruthy() const {
        switch(type) {
            case ValType::Undefined: return false;
            case ValType::Null: return false;
            case ValType::Boolean: return boolean;
            case ValType::Number: return num != 0 && !std::isnan(num);
            case ValType::String: return !str.empty();
            default: return true;
        }
    }

    bool isNullish() const { return type == ValType::Undefined || type == ValType::Null; }
};

struct JSObject {
    std::map<std::string, ValPtr> props;
    std::vector<ValPtr> elements; // for arrays
    bool isArray = false;
    std::string name; // function name
    NodePtr funcBody;
    std::vector<std::string> params;
    std::shared_ptr<struct Environment> closure;
    // For prototype chain
    ObjPtr proto;

    JSObject() {}
    JSObject(bool arr): isArray(arr) {}

    ValPtr get(const std::string& key) const {
        if (isArray) {
            if (key == "length") return Value::makeNumber((double)elements.size());
            try { int idx = std::stoi(key); if (idx >= 0 && idx < (int)elements.size()) return elements[idx]; }
            catch(...) {}
        }
        auto it = props.find(key);
        if (it != props.end()) return it->second;
        if (proto) return proto->get(key);
        return Value::makeUndefined();
    }

    void set(const std::string& key, ValPtr val) {
        if (isArray) {
            try {
                int idx = std::stoi(key);
                if (idx >= 0) {
                    if (idx >= (int)elements.size()) elements.resize(idx+1, Value::makeUndefined());
                    elements[idx] = val;
                    return;
                }
            } catch(...) {}
            if (key == "length") {
                int newLen = (int)val->num;
                elements.resize(newLen, Value::makeUndefined());
                return;
            }
        }
        props[key] = val;
    }

    bool has(const std::string& key) const {
        if (isArray) {
            if (key == "length") return true;
            try { int idx = std::stoi(key); if (idx >= 0 && idx < (int)elements.size()) return true; } catch(...) {}
        }
        return props.count(key) > 0;
    }
};

// Forward declare Environment
struct Environment {
    std::map<std::string, ValPtr> vars;
    std::shared_ptr<Environment> parent;

    Environment(std::shared_ptr<Environment> p = nullptr): parent(p) {}

    ValPtr get(const std::string& name) const {
        auto it = vars.find(name);
        if (it != vars.end()) return it->second;
        if (parent) return parent->get(name);
        return Value::makeUndefined();
    }

    bool has(const std::string& name) const {
        if (vars.count(name)) return true;
        if (parent) return parent->has(name);
        return false;
    }

    void set(const std::string& name, ValPtr val) {
        auto it = vars.find(name);
        if (it != vars.end()) { it->second = val; return; }
        if (parent && parent->has(name)) { parent->set(name, val); return; }
        vars[name] = val; // create in current scope
    }

    void setLocal(const std::string& name, ValPtr val) {
        vars[name] = val;
    }
};

using EnvPtr = std::shared_ptr<Environment>;

ValPtr Value::makeFunction(NodePtr body, std::vector<std::string> ps, EnvPtr env, std::string name) {
    auto v = std::make_shared<Value>();
    v->type = ValType::Function;
    v->funcBody = body;
    v->params = ps;
    v->closure = env;
    v->str = name;
    return v;
}

// ---- Number to string conversion ----
inline std::string numToString(double n) {
    if (std::isnan(n)) return "NaN";
    if (std::isinf(n)) return n > 0 ? "Infinity" : "-Infinity";
    if (n == 0) return "0";
    // Check if integer
    if (n == std::floor(n) && std::abs(n) < 1e15) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << n;
        return oss.str();
    }
    std::ostringstream oss;
    oss << std::setprecision(17) << n;
    std::string s = oss.str();
    // Remove trailing zeros after decimal
    if (s.find('.') != std::string::npos) {
        size_t last = s.find_last_not_of('0');
        if (last != std::string::npos && s[last] == '.') last--;
        s = s.substr(0, last+1);
    }
    return s;
}

inline std::string valueToString(ValPtr v);
inline std::string valueToString(const Value& v) {
    switch(v.type) {
        case ValType::Undefined: return "undefined";
        case ValType::Null: return "null";
        case ValType::Boolean: return v.boolean ? "true" : "false";
        case ValType::Number: return numToString(v.num);
        case ValType::String: return v.str;
        case ValType::Function: return "[Function: " + (v.str.empty() ? "anonymous" : v.str) + "]";
        case ValType::NativeFunction: return "[Function (native)]";
        case ValType::Array: {
            if (!v.obj) return "";
            std::string s;
            for (int i = 0; i < (int)v.obj->elements.size(); i++) {
                if (i > 0) s += ",";
                auto el = v.obj->elements[i];
                if (el && el->type != ValType::Undefined && el->type != ValType::Null)
                    s += valueToString(*el);
            }
            return s;
        }
        case ValType::Object: {
            if (!v.obj) return "[object Object]";
            return "[object Object]";
        }
    }
    return "undefined";
}
inline std::string valueToString(ValPtr v) { return v ? valueToString(*v) : "undefined"; }

inline double valueToNumber(ValPtr v) {
    if (!v) return std::numeric_limits<double>::quiet_NaN();
    switch(v->type) {
        case ValType::Number: return v->num;
        case ValType::Boolean: return v->boolean ? 1 : 0;
        case ValType::String: {
            std::string s = v->str;
            // trim
            size_t start = s.find_first_not_of(" \t\n\r");
            if (start == std::string::npos) return 0;
            s = s.substr(start);
            size_t end = s.find_last_not_of(" \t\n\r");
            s = s.substr(0, end+1);
            if (s.empty()) return 0;
            if (s == "Infinity" || s == "+Infinity") return std::numeric_limits<double>::infinity();
            if (s == "-Infinity") return -std::numeric_limits<double>::infinity();
            try { return std::stod(s); } catch(...) { return std::numeric_limits<double>::quiet_NaN(); }
        }
        case ValType::Null: return 0;
        case ValType::Undefined: return std::numeric_limits<double>::quiet_NaN();
        case ValType::Array: {
            if (!v->obj) return 0;
            if (v->obj->elements.empty()) return 0;
            if (v->obj->elements.size() == 1) return valueToNumber(v->obj->elements[0]);
            return std::numeric_limits<double>::quiet_NaN();
        }
        default: return std::numeric_limits<double>::quiet_NaN();
    }
}

inline bool strictEqual(ValPtr a, ValPtr b) {
    if (a->type != b->type) return false;
    switch(a->type) {
        case ValType::Undefined: return true;
        case ValType::Null: return true;
        case ValType::Boolean: return a->boolean == b->boolean;
        case ValType::Number: return a->num == b->num;
        case ValType::String: return a->str == b->str;
        default: return a->obj == b->obj;
    }
}

inline bool looseEqual(ValPtr a, ValPtr b) {
    if (a->type == b->type) return strictEqual(a, b);
    if ((a->type==ValType::Null && b->type==ValType::Undefined) ||
        (a->type==ValType::Undefined && b->type==ValType::Null)) return true;
    if (a->type==ValType::Number && b->type==ValType::String) return a->num == valueToNumber(b);
    if (a->type==ValType::String && b->type==ValType::Number) return valueToNumber(a) == b->num;
    if (a->type==ValType::Boolean) { auto na = Value::makeNumber(valueToNumber(a)); return looseEqual(na, b); }
    if (b->type==ValType::Boolean) { auto nb = Value::makeNumber(valueToNumber(b)); return looseEqual(a, nb); }
    return false;
}

#endif // VALUE_H