#ifndef INTERPRETER_H
#define INTERPRETER_H
#include "value.h"
#include <iostream>
#include <ctime>
#include <algorithm>
#include <random>
#include <chrono>
#include <regex>
#include <set>
#include <climits>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif

// Control flow signals
struct ReturnSignal { ValPtr value; };
struct BreakSignal {};
struct ContinueSignal {};
struct ThrowSignal { ValPtr value; };

class Interpreter {
public:
    EnvPtr globalEnv;

    Interpreter() {
        globalEnv = std::make_shared<Environment>();
        setupGlobals();
    }

    void run(NodePtr node) {
        try {
            eval(node, globalEnv);
        } catch (ThrowSignal& t) {
            std::cerr << "Uncaught: " << valueToString(t.value) << std::endl;
        }
    }

private:
    // ---- Globals Setup ----
    void setupGlobals() {
        auto env = globalEnv;

        // console.log
        auto consoleObj = std::make_shared<JSObject>();
        consoleObj->props["log"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            for (int i = 0; i < (int)args.size(); i++) {
                if (i > 0) std::cout << " ";
                std::cout << printValue(args[i]);
            }
            std::cout << "\n";
            return Value::makeUndefined();
        });
        consoleObj->props["error"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            for (int i = 0; i < (int)args.size(); i++) {
                if (i > 0) std::cerr << " ";
                std::cerr << printValue(args[i]);
            }
            std::cerr << "\n";
            return Value::makeUndefined();
        });
        env->setLocal("console", Value::makeObject(consoleObj));

        // Math object
        auto mathObj = std::make_shared<JSObject>();
        mathObj->props["PI"] = Value::makeNumber(M_PI);
        mathObj->props["E"] = Value::makeNumber(M_E);
        mathObj->props["abs"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            return Value::makeNumber(std::abs(valueToNumber(args.empty() ? Value::makeUndefined() : args[0])));
        });
        mathObj->props["floor"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            return Value::makeNumber(std::floor(valueToNumber(args.empty() ? Value::makeUndefined() : args[0])));
        });
        mathObj->props["ceil"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            return Value::makeNumber(std::ceil(valueToNumber(args.empty() ? Value::makeUndefined() : args[0])));
        });
        mathObj->props["round"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            return Value::makeNumber(std::round(valueToNumber(args.empty() ? Value::makeUndefined() : args[0])));
        });
        mathObj->props["sqrt"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            return Value::makeNumber(std::sqrt(valueToNumber(args.empty() ? Value::makeUndefined() : args[0])));
        });
        mathObj->props["pow"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            double a = args.size()>0 ? valueToNumber(args[0]) : 0;
            double b = args.size()>1 ? valueToNumber(args[1]) : 0;
            return Value::makeNumber(std::pow(a, b));
        });
        mathObj->props["max"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            if (args.empty()) return Value::makeNumber(-std::numeric_limits<double>::infinity());
            double m = valueToNumber(args[0]);
            for (int i = 1; i < (int)args.size(); i++) m = std::max(m, valueToNumber(args[i]));
            return Value::makeNumber(m);
        });
        mathObj->props["min"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            if (args.empty()) return Value::makeNumber(std::numeric_limits<double>::infinity());
            double m = valueToNumber(args[0]);
            for (int i = 1; i < (int)args.size(); i++) m = std::min(m, valueToNumber(args[i]));
            return Value::makeNumber(m);
        });
        mathObj->props["random"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            return Value::makeNumber(dist(rng));
        });
        mathObj->props["log"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            return Value::makeNumber(std::log(valueToNumber(args.empty() ? Value::makeUndefined() : args[0])));
        });
        mathObj->props["log2"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            return Value::makeNumber(std::log2(valueToNumber(args.empty() ? Value::makeUndefined() : args[0])));
        });
        mathObj->props["log10"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            return Value::makeNumber(std::log10(valueToNumber(args.empty() ? Value::makeUndefined() : args[0])));
        });
        mathObj->props["trunc"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            return Value::makeNumber(std::trunc(valueToNumber(args.empty() ? Value::makeUndefined() : args[0])));
        });
        mathObj->props["sign"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            double n = valueToNumber(args.empty() ? Value::makeUndefined() : args[0]);
            return Value::makeNumber(n > 0 ? 1 : (n < 0 ? -1 : 0));
        });
        mathObj->props["sin"] = Value::makeNative([](std::vector<ValPtr> args)->ValPtr{ return Value::makeNumber(std::sin(valueToNumber(args.empty()?Value::makeUndefined():args[0]))); });
        mathObj->props["cos"] = Value::makeNative([](std::vector<ValPtr> args)->ValPtr{ return Value::makeNumber(std::cos(valueToNumber(args.empty()?Value::makeUndefined():args[0]))); });
        mathObj->props["tan"] = Value::makeNative([](std::vector<ValPtr> args)->ValPtr{ return Value::makeNumber(std::tan(valueToNumber(args.empty()?Value::makeUndefined():args[0]))); });
        env->setLocal("Math", Value::makeObject(mathObj));

        // parseInt, parseFloat
        env->setLocal("parseInt", Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            if (args.empty()) return Value::makeNumber(std::numeric_limits<double>::quiet_NaN());
            std::string s = valueToString(args[0]);
            int base = args.size() > 1 ? (int)valueToNumber(args[1]) : 10;
            size_t start = s.find_first_not_of(" \t\n\r");
            if (start == std::string::npos) return Value::makeNumber(std::numeric_limits<double>::quiet_NaN());
            s = s.substr(start);
            try {
                size_t idx;
                long long v = std::stoll(s, &idx, base);
                return Value::makeNumber((double)v);
            } catch(...) { return Value::makeNumber(std::numeric_limits<double>::quiet_NaN()); }
        }));
        env->setLocal("parseFloat", Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            if (args.empty()) return Value::makeNumber(std::numeric_limits<double>::quiet_NaN());
            std::string s = valueToString(args[0]);
            try { return Value::makeNumber(std::stod(s)); } catch(...) { return Value::makeNumber(std::numeric_limits<double>::quiet_NaN()); }
        }));
        env->setLocal("isNaN", Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            return Value::makeBool(std::isnan(valueToNumber(args.empty() ? Value::makeUndefined() : args[0])));
        }));
        env->setLocal("isFinite", Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            return Value::makeBool(std::isfinite(valueToNumber(args.empty() ? Value::makeUndefined() : args[0])));
        }));
        env->setLocal("Number", Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            return Value::makeNumber(valueToNumber(args.empty() ? Value::makeUndefined() : args[0]));
        }));
        env->setLocal("String", Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            return Value::makeString(valueToString(args.empty() ? Value::makeUndefined() : args[0]));
        }));
        env->setLocal("Boolean", Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            return Value::makeBool((args.empty() ? Value::makeUndefined() : args[0])->isTruthy());
        }));
        env->setLocal("undefined", Value::makeUndefined());
        env->setLocal("NaN", Value::makeNumber(std::numeric_limits<double>::quiet_NaN()));
        env->setLocal("Infinity", Value::makeNumber(std::numeric_limits<double>::infinity()));

        // Array constructor
        env->setLocal("Array", Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            auto obj = std::make_shared<JSObject>(true);
            if (args.size() == 1 && args[0]->type == ValType::Number) {
                obj->elements.resize((int)args[0]->num, Value::makeUndefined());
            } else {
                for (auto& a : args) obj->elements.push_back(a);
            }
            return Value::makeArray(obj);
        }));

        // Object constructor
        env->setLocal("Object", Value::makeNative([this](std::vector<ValPtr> args) -> ValPtr {
            if (!args.empty() && args[0]->type == ValType::Object) return args[0];
            auto obj = std::make_shared<JSObject>();
            return Value::makeObject(obj);
        }));

        // JSON
        auto jsonObj = std::make_shared<JSObject>();
        jsonObj->props["stringify"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            if (args.empty()) return Value::makeUndefined();
            return Value::makeString(jsonStringify(args[0]));
        });
        jsonObj->props["parse"] = Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            // Basic JSON parse (not fully implemented)
            return Value::makeUndefined();
        });
        env->setLocal("JSON", Value::makeObject(jsonObj));

        // Date
        env->setLocal("Date", Value::makeNative([](std::vector<ValPtr> args) -> ValPtr {
            auto obj = std::make_shared<JSObject>();
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            obj->props["__timestamp__"] = Value::makeNumber((double)ms);
            obj->props["getTime"] = Value::makeNative([ms](std::vector<ValPtr>) -> ValPtr {
                return Value::makeNumber((double)ms);
            });
            // Add basic date methods
            time_t t = ms / 1000;
            struct tm* tm_info = localtime(&t);
            int year = tm_info->tm_year + 1900;
            int month = tm_info->tm_mon;
            int day = tm_info->tm_mday;
            int hours = tm_info->tm_hour;
            int mins = tm_info->tm_min;
            int secs = tm_info->tm_sec;
            obj->props["getFullYear"] = Value::makeNative([year](std::vector<ValPtr>)->ValPtr{ return Value::makeNumber(year); });
            obj->props["getMonth"] = Value::makeNative([month](std::vector<ValPtr>)->ValPtr{ return Value::makeNumber(month); });
            obj->props["getDate"] = Value::makeNative([day](std::vector<ValPtr>)->ValPtr{ return Value::makeNumber(day); });
            obj->props["getHours"] = Value::makeNative([hours](std::vector<ValPtr>)->ValPtr{ return Value::makeNumber(hours); });
            obj->props["getMinutes"] = Value::makeNative([mins](std::vector<ValPtr>)->ValPtr{ return Value::makeNumber(mins); });
            obj->props["getSeconds"] = Value::makeNative([secs](std::vector<ValPtr>)->ValPtr{ return Value::makeNumber(secs); });
            obj->props["getDay"] = Value::makeNative([tm_info](std::vector<ValPtr>)->ValPtr{ return Value::makeNumber(tm_info->tm_wday); });
            return Value::makeObject(obj);
        }));

        // typeof already handled in eval
    }

    // ---- Print helper ----
    static std::string printValue(ValPtr v) {
        if (!v) return "undefined";
        switch(v->type) {
            case ValType::Undefined: return "undefined";
            case ValType::Null: return "null";
            case ValType::Boolean: return v->boolean ? "true" : "false";
            case ValType::Number: return numToString(v->num);
            case ValType::String: return v->str;
            case ValType::Function: return "[Function: " + (v->str.empty() ? "anonymous" : v->str) + "]";
            case ValType::NativeFunction: return "[Function (native)]";
            case ValType::Array: {
                if (!v->obj) return "[]";
                std::string s = "[ ";
                for (int i = 0; i < (int)v->obj->elements.size(); i++) {
                    if (i > 0) s += ", ";
                    s += printValue(v->obj->elements[i]);
                }
                s += " ]";
                return s;
            }
            case ValType::Object: {
                if (!v->obj) return "{}";
                std::string s = "{ ";
                bool first = true;
                for (auto it = v->obj->props.begin(); it != v->obj->props.end(); ++it) { const std::string& k = it->first; ValPtr val = it->second;
                    if (!first) s += ", ";
                    s += k + ": " + printValue(val);
                    first = false;
                }
                s += " }";
                return s;
            }
        }
        return "undefined";
    }

    static std::string jsonStringify(ValPtr v, int indent=0) {
        if (!v) return "null";
        switch(v->type) {
            case ValType::Null: return "null";
            case ValType::Undefined: return "undefined";
            case ValType::Boolean: return v->boolean ? "true" : "false";
            case ValType::Number: {
                if (std::isnan(v->num) || std::isinf(v->num)) return "null";
                return numToString(v->num);
            }
            case ValType::String: {
                std::string s = "\"";
                for (char c : v->str) {
                    if (c == '"') s += "\\\"";
                    else if (c == '\\') s += "\\\\";
                    else if (c == '\n') s += "\\n";
                    else if (c == '\t') s += "\\t";
                    else s += c;
                }
                s += "\"";
                return s;
            }
            case ValType::Array: {
                std::string s = "[";
                if (v->obj) {
                    for (int i = 0; i < (int)v->obj->elements.size(); i++) {
                        if (i > 0) s += ",";
                        s += jsonStringify(v->obj->elements[i], indent+1);
                    }
                }
                return s + "]";
            }
            case ValType::Object: {
                std::string s = "{";
                bool first = true;
                if (v->obj) {
                    for (auto it = v->obj->props.begin(); it != v->obj->props.end(); ++it) { const std::string& k = it->first; ValPtr val = it->second;
                        if (!first) s += ",";
                        s += "\"" + k + "\":" + jsonStringify(val, indent+1);
                        first = false;
                    }
                }
                return s + "}";
            }
            default: return "null";
        }
    }

    // ---- Main eval ----
    ValPtr eval(NodePtr node, EnvPtr env) {
        if (!node) return Value::makeUndefined();
        switch(node->type) {
            case NodeType::Program: return evalProgram(node, env);
            case NodeType::Block: return evalBlock(node, env);
            case NodeType::VarDecl: return evalVarDecl(node, env);
            case NodeType::FuncDecl: return evalFuncDecl(node, env);
            case NodeType::ExprStmt: return eval(node->children[0], env);
            case NodeType::Return: throw ReturnSignal{node->children.empty() ? Value::makeUndefined() : eval(node->children[0], env)};
            case NodeType::Break: throw BreakSignal{};
            case NodeType::Continue: throw ContinueSignal{};
            case NodeType::Throw: throw ThrowSignal{eval(node->children[0], env)};
            case NodeType::If: return evalIf(node, env);
            case NodeType::While: return evalWhile(node, env);
            case NodeType::For: return evalFor(node, env);
            case NodeType::ForIn: return evalForIn(node, env);
            case NodeType::ForOf: return evalForOf(node, env);
            case NodeType::DoWhile: return evalDoWhile(node, env);
            case NodeType::Switch: return evalSwitch(node, env);
            case NodeType::Try: return evalTry(node, env);
            case NodeType::ClassDecl: return evalClassDecl(node, env);
            case NodeType::Literal: return evalLiteral(node);
            case NodeType::Identifier: return env->get(node->sval);
            case NodeType::BinOp: return evalBinOp(node, env);
            case NodeType::UnaryOp: return evalUnaryOp(node, env);
            case NodeType::PostfixOp: return evalPostfixOp(node, env);
            case NodeType::Assignment: return evalAssignment(node, env);
            case NodeType::Call: return evalCall(node, env);
            case NodeType::MemberAccess: return evalMemberAccess(node, env);
            case NodeType::Index: return evalIndex(node, env);
            case NodeType::Ternary: return eval(node->children[0], env)->isTruthy() ? eval(node->children[1], env) : eval(node->children[2], env);
            case NodeType::ArrayLiteral: return evalArrayLiteral(node, env);
            case NodeType::ObjectLiteral: return evalObjectLiteral(node, env);
            case NodeType::ArrowFunc:
            case NodeType::FuncExpr: return evalFuncExpr(node, env);
            case NodeType::NewExpr: return evalNew(node, env);
            case NodeType::TypeOf: return evalTypeof(node, env);
            case NodeType::Sequence: { eval(node->children[0], env); return eval(node->children[1], env); }
            default: return Value::makeUndefined();
        }
    }

    ValPtr evalProgram(NodePtr node, EnvPtr env) {
        // First pass: hoist function declarations
        for (auto& child : node->children) {
            if (child->type == NodeType::FuncDecl) {
                evalFuncDecl(child, env);
            }
        }
        ValPtr last = Value::makeUndefined();
        for (auto& child : node->children) {
            if (child->type != NodeType::FuncDecl)
                last = eval(child, env);
        }
        return last;
    }

    ValPtr evalBlock(NodePtr node, EnvPtr env) {
        // Hoist func decls
        for (auto& child : node->children) {
            if (child->type == NodeType::FuncDecl) evalFuncDecl(child, env);
        }
        ValPtr last = Value::makeUndefined();
        for (auto& child : node->children) {
            if (child->type != NodeType::FuncDecl)
                last = eval(child, env);
        }
        return last;
    }

    ValPtr evalVarDecl(NodePtr node, EnvPtr env) {
        // Check for destructuring pattern
        for (auto& child : node->children) {
            if (child->type == NodeType::ArrayLiteral && child->sval == "__destruct_array__") {
                // Next child is the init value... but we need to handle this differently
                // Actually the init is stored as the second child
                continue;
            }
            if (child->type == NodeType::Identifier) {
                ValPtr initVal = Value::makeUndefined();
                if (!child->children.empty()) {
                    initVal = eval(child->children[0], env);
                }
                env->setLocal(child->sval, initVal);
            }
        }
        // Handle destructuring: children[0] is pattern, children[1] is init
        if (!node->children.empty()) {
            auto& first = node->children[0];
            if (first->type == NodeType::ArrayLiteral && first->sval == "__destruct_array__") {
                ValPtr initVal = Value::makeUndefined();
                if (node->children.size() > 1) initVal = eval(node->children[1], env);
                destructureArray(first, initVal, env);
            } else if (first->type == NodeType::ObjectLiteral && first->sval == "__destruct_obj__") {
                ValPtr initVal = Value::makeUndefined();
                if (node->children.size() > 1) initVal = eval(node->children[1], env);
                destructureObject(first, initVal, env);
            }
        }
        return Value::makeUndefined();
    }

    void destructureArray(NodePtr pattern, ValPtr val, EnvPtr env) {
        int i = 0;
        for (auto& elem : pattern->children) {
            if (elem->type == NodeType::SpreadElement) {
                auto arr = std::make_shared<JSObject>(true);
                if (val->type == ValType::Array && val->obj) {
                    for (int j = i; j < (int)val->obj->elements.size(); j++)
                        arr->elements.push_back(val->obj->elements[j]);
                }
                env->setLocal(elem->sval, Value::makeArray(arr));
                break;
            }
            if (elem->sval == "__hole__") { i++; continue; }
            ValPtr elemVal = Value::makeUndefined();
            if (val->type == ValType::Array && val->obj && i < (int)val->obj->elements.size())
                elemVal = val->obj->elements[i];
            // Default value
            if ((elemVal->type == ValType::Undefined) && !elem->children.empty())
                elemVal = eval(elem->children[0], env);
            env->setLocal(elem->sval, elemVal);
            i++;
        }
    }

    void destructureObject(NodePtr pattern, ValPtr val, EnvPtr env) {
        for (auto& prop : pattern->children) {
            if (prop->type == NodeType::SpreadElement) {
                auto rest = std::make_shared<JSObject>();
                // Collect remaining props
                if (val->type == ValType::Object && val->obj) {
                    std::set<std::string> used;
                    for (auto& p2 : pattern->children) {
                        if (p2->type != NodeType::SpreadElement) used.insert(p2->sval);
                    }
                    for (auto it2 = val->obj->props.begin(); it2 != val->obj->props.end(); ++it2) { const std::string& k = it2->first; ValPtr v = it2->second;
                        if (!used.count(k)) rest->props[k] = v;
                    }
                }
                env->setLocal(prop->sval, Value::makeObject(rest));
                continue;
            }
            auto& id = prop->children[0];
            ValPtr v = val->type==ValType::Object && val->obj ? val->obj->get(prop->sval) : Value::makeUndefined();
            if (v->type == ValType::Undefined && !id->children.empty()) v = eval(id->children[0], env);
            env->setLocal(id->sval, v);
        }
    }

    ValPtr evalFuncDecl(NodePtr node, EnvPtr env) {
        auto fn = Value::makeFunction(node->children.back(), node->params, env, node->sval);
        // Store default params and destructs in function
        fn->obj = std::make_shared<JSObject>();
        for (auto& c : node->children) {
            if (c->sval.find("__default__") == 0 || c->sval.find("__param_pattern__") == 0)
                fn->obj->props[c->sval] = eval(c, env); // for defaults, we need to eval lazily...
        }
        env->setLocal(node->sval, fn);
        return fn;
    }

    ValPtr evalFuncExpr(NodePtr node, EnvPtr env) {
        auto fn = Value::makeFunction(node->children.back(), node->params, env, node->sval);
        if (!node->sval.empty()) env->setLocal(node->sval, fn);
        return fn;
    }

    ValPtr evalLiteral(NodePtr node) {
        if (node->sval == "__number__") return Value::makeNumber(node->nval);
        if (node->sval == "__bool__") return Value::makeBool(node->bval);
        if (node->sval == "__null__") return Value::makeNull();
        if (node->sval == "__undefined__") return Value::makeUndefined();
        // String literal (bval == true means string)
        if (node->bval) return Value::makeString(node->sval);
        return Value::makeString(node->sval);
    }

    ValPtr evalIf(NodePtr node, EnvPtr env) {
        auto cond = eval(node->children[0], env);
        if (cond->isTruthy()) {
            auto childEnv = std::make_shared<Environment>(env);
            return eval(node->children[1], childEnv);
        } else if (node->children.size() > 2) {
            auto childEnv = std::make_shared<Environment>(env);
            return eval(node->children[2], childEnv);
        }
        return Value::makeUndefined();
    }

    ValPtr evalWhile(NodePtr node, EnvPtr env) {
        while (eval(node->children[0], env)->isTruthy()) {
            auto loopEnv = std::make_shared<Environment>(env);
            try { eval(node->children[1], loopEnv); }
            catch(BreakSignal&) { break; }
            catch(ContinueSignal&) { continue; }
        }
        return Value::makeUndefined();
    }

    ValPtr evalDoWhile(NodePtr node, EnvPtr env) {
        do {
            auto loopEnv = std::make_shared<Environment>(env);
            try { eval(node->children[1], loopEnv); }
            catch(BreakSignal&) { break; }
            catch(ContinueSignal&) { continue; }
        } while (eval(node->children[0], env)->isTruthy());
        return Value::makeUndefined();
    }

    ValPtr evalFor(NodePtr node, EnvPtr env) {
        auto forEnv = std::make_shared<Environment>(env);
        // children: [init, cond, update, body]
        if (node->children[0]->type != NodeType::Literal) eval(node->children[0], forEnv);
        while (true) {
            if (node->children[1]->type != NodeType::Literal) {
                auto cv = eval(node->children[1], forEnv);
                if (!cv->isTruthy()) break;
            }
            auto loopEnv = std::make_shared<Environment>(forEnv);
            bool broke = false;
            try { eval(node->children[3], loopEnv); }
            catch(BreakSignal&) { broke = true; }
            catch(ContinueSignal&) {}
            if (broke) break;
            if (node->children[2]->type != NodeType::Literal) eval(node->children[2], forEnv);
        }
        return Value::makeUndefined();
    }

    ValPtr evalForOf(NodePtr node, EnvPtr env) {
        auto iterVal = eval(node->children[0], env);
        auto forEnv = std::make_shared<Environment>(env);
        std::vector<ValPtr> items;
        if (iterVal->type == ValType::Array && iterVal->obj) {
            items = iterVal->obj->elements;
        } else if (iterVal->type == ValType::String) {
            for (char c : iterVal->str) items.push_back(Value::makeString(std::string(1, c)));
        }
        for (auto& item : items) {
            forEnv->setLocal(node->sval, item);
            auto loopEnv = std::make_shared<Environment>(forEnv);
            try { eval(node->children[1], loopEnv); }
            catch(BreakSignal&) { break; }
            catch(ContinueSignal&) { continue; }
        }
        return Value::makeUndefined();
    }

    ValPtr evalForIn(NodePtr node, EnvPtr env) {
        auto iterVal = eval(node->children[0], env);
        auto forEnv = std::make_shared<Environment>(env);
        std::vector<std::string> keys;
        if (iterVal->type == ValType::Object && iterVal->obj) {
            for (auto it3 = iterVal->obj->props.begin(); it3 != iterVal->obj->props.end(); ++it3) keys.push_back(it3->first);
        } else if (iterVal->type == ValType::Array && iterVal->obj) {
            for (int i = 0; i < (int)iterVal->obj->elements.size(); i++) keys.push_back(std::to_string(i));
        }
        for (auto& key : keys) {
            forEnv->setLocal(node->sval, Value::makeString(key));
            auto loopEnv = std::make_shared<Environment>(forEnv);
            try { eval(node->children[1], loopEnv); }
            catch(BreakSignal&) { break; }
            catch(ContinueSignal&) { continue; }
        }
        return Value::makeUndefined();
    }

    ValPtr evalSwitch(NodePtr node, EnvPtr env) {
        auto disc = eval(node->children[0], env);
        bool matched = false;
        auto switchEnv = std::make_shared<Environment>(env);
        for (int i = 1; i < (int)node->children.size(); i++) {
            auto& sc = node->children[i];
            if (!matched) {
                if (sc->sval == "default") { matched = true; }
                else if (!sc->children.empty()) {
                    auto caseVal = eval(sc->children[0], env);
                    if (strictEqual(disc, caseVal)) matched = true;
                }
            }
            if (matched) {
                int start = (sc->sval == "default") ? 0 : 1;
                try {
                    for (int j = start; j < (int)sc->children.size(); j++)
                        eval(sc->children[j], switchEnv);
                } catch(BreakSignal&) { return Value::makeUndefined(); }
            }
        }
        // Try default if nothing matched
        if (!matched) {
            for (int i = 1; i < (int)node->children.size(); i++) {
                if (node->children[i]->sval == "default") {
                    try {
                        for (int j = 0; j < (int)node->children[i]->children.size(); j++)
                            eval(node->children[i]->children[j], switchEnv);
                    } catch(BreakSignal&) {}
                    break;
                }
            }
        }
        return Value::makeUndefined();
    }

    ValPtr evalTry(NodePtr node, EnvPtr env) {
        // children: [try_block, catch_block, (optional) finally_block]
        ValPtr result = Value::makeUndefined();
        bool threw = false;
        ValPtr thrownVal;
        try {
            auto tryEnv = std::make_shared<Environment>(env);
            result = eval(node->children[0], tryEnv);
        } catch(ThrowSignal& t) {
            threw = true;
            thrownVal = t.value;
        } catch(ReturnSignal&) { throw; }
        catch(BreakSignal&) { throw; }
        catch(ContinueSignal&) { throw; }

        if (threw && node->children.size() > 1) {
            auto catchEnv = std::make_shared<Environment>(env);
            if (!node->sval.empty()) catchEnv->setLocal(node->sval, thrownVal);
            try { result = eval(node->children[1], catchEnv); }
            catch(...) {
                if (node->children.size() > 2) eval(node->children[2], env);
                throw;
            }
        }
        if (node->children.size() > 2) eval(node->children[2], env);
        return result;
    }

    ValPtr evalClassDecl(NodePtr node, EnvPtr env) {
        // Create a constructor function
        std::string className = node->sval;
        std::string parentName = node->params.empty() ? "" : node->params[0];

        // Find constructor
        NodePtr constructorNode;
        std::vector<NodePtr> methods;
        for (auto& child : node->children) {
            if (child->sval == "constructor") constructorNode = child;
            else methods.push_back(child);
        }

        auto capturedMethods = methods;
        auto capturedEnv = env;
        auto capturedConstructor = constructorNode;
        auto capturedParent = parentName;
        auto capturedClassName = className;

        auto ctorFn = Value::makeNative([this, capturedConstructor, capturedMethods, capturedEnv, capturedParent, capturedClassName](std::vector<ValPtr> args) -> ValPtr {
            auto obj = std::make_shared<JSObject>();
            auto thisVal = Value::makeObject(obj);
            obj->props["__class__"] = Value::makeString(capturedClassName);

            // Add methods
            EnvPtr methodEnv = std::make_shared<Environment>(capturedEnv);
            for (auto& m : capturedMethods) {
                if (m->sval.find("static_") == 0) continue;
                auto mfn = Value::makeFunction(m->children.back(), m->params, methodEnv, m->sval);
                obj->props[m->sval] = mfn;
            }

            // Call constructor
            if (capturedConstructor) {
                auto ctorEnv = std::make_shared<Environment>(capturedEnv);
                ctorEnv->setLocal("this", thisVal);
                // Bind args to params
                auto& params = capturedConstructor->params;
                for (int i = 0; i < (int)params.size(); i++) {
                    ctorEnv->setLocal(params[i], i < (int)args.size() ? args[i] : Value::makeUndefined());
                }
                try { eval(capturedConstructor->children.back(), ctorEnv); }
                catch(ReturnSignal&) {}
            }
            return thisVal;
        });

        env->setLocal(className, ctorFn);
        return ctorFn;
    }

    ValPtr evalBinOp(NodePtr node, EnvPtr env) {
        std::string op = node->sval;

        // Short-circuit
        if (op == "&&") {
            auto left = eval(node->children[0], env);
            return left->isTruthy() ? eval(node->children[1], env) : left;
        }
        if (op == "||") {
            auto left = eval(node->children[0], env);
            return left->isTruthy() ? left : eval(node->children[1], env);
        }
        if (op == "??") {
            auto left = eval(node->children[0], env);
            return left->isNullish() ? eval(node->children[1], env) : left;
        }

        auto left = eval(node->children[0], env);
        auto right = eval(node->children[1], env);

        if (op == "+") {
            if (left->type == ValType::String || right->type == ValType::String)
                return Value::makeString(valueToString(left) + valueToString(right));
            return Value::makeNumber(valueToNumber(left) + valueToNumber(right));
        }
        if (op == "-") return Value::makeNumber(valueToNumber(left) - valueToNumber(right));
        if (op == "*") return Value::makeNumber(valueToNumber(left) * valueToNumber(right));
        if (op == "/") return Value::makeNumber(valueToNumber(left) / valueToNumber(right));
        if (op == "%") {
            double a = valueToNumber(left), b = valueToNumber(right);
            return Value::makeNumber(std::fmod(a, b));
        }
        if (op == "**") return Value::makeNumber(std::pow(valueToNumber(left), valueToNumber(right)));
        if (op == "===") return Value::makeBool(strictEqual(left, right));
        if (op == "!==") return Value::makeBool(!strictEqual(left, right));
        if (op == "==") return Value::makeBool(looseEqual(left, right));
        if (op == "!=") return Value::makeBool(!looseEqual(left, right));
        if (op == "<") {
            if (left->type==ValType::String && right->type==ValType::String) return Value::makeBool(left->str < right->str);
            return Value::makeBool(valueToNumber(left) < valueToNumber(right));
        }
        if (op == ">") {
            if (left->type==ValType::String && right->type==ValType::String) return Value::makeBool(left->str > right->str);
            return Value::makeBool(valueToNumber(left) > valueToNumber(right));
        }
        if (op == "<=") {
            if (left->type==ValType::String && right->type==ValType::String) return Value::makeBool(left->str <= right->str);
            return Value::makeBool(valueToNumber(left) <= valueToNumber(right));
        }
        if (op == ">=") {
            if (left->type==ValType::String && right->type==ValType::String) return Value::makeBool(left->str >= right->str);
            return Value::makeBool(valueToNumber(left) >= valueToNumber(right));
        }
        if (op == "&") return Value::makeNumber((double)((int)valueToNumber(left) & (int)valueToNumber(right)));
        if (op == "|") return Value::makeNumber((double)((int)valueToNumber(left) | (int)valueToNumber(right)));
        if (op == "^") return Value::makeNumber((double)((int)valueToNumber(left) ^ (int)valueToNumber(right)));
        if (op == "<<") return Value::makeNumber((double)((int)valueToNumber(left) << (int)valueToNumber(right)));
        if (op == ">>") return Value::makeNumber((double)((int)valueToNumber(left) >> (int)valueToNumber(right)));
        if (op == ">>>") return Value::makeNumber((double)((unsigned int)valueToNumber(left) >> (int)valueToNumber(right)));
        if (op == "instanceof") return Value::makeBool(false); // simplified
        if (op == "in") {
            std::string key = valueToString(left);
            if (right->type == ValType::Object && right->obj) return Value::makeBool(right->obj->has(key));
            if (right->type == ValType::Array && right->obj) {
                try { int idx = std::stoi(key); return Value::makeBool(idx >= 0 && idx < (int)right->obj->elements.size()); } catch(...) {}
                return Value::makeBool(right->obj->has(key));
            }
            return Value::makeBool(false);
        }
        return Value::makeUndefined();
    }

    ValPtr evalUnaryOp(NodePtr node, EnvPtr env) {
        std::string op = node->sval;
        if (op == "++pre") {
            auto val = eval(node->children[0], env);
            auto newVal = Value::makeNumber(valueToNumber(val) + 1);
            assignToNode(node->children[0], newVal, env);
            return newVal;
        }
        if (op == "--pre") {
            auto val = eval(node->children[0], env);
            auto newVal = Value::makeNumber(valueToNumber(val) - 1);
            assignToNode(node->children[0], newVal, env);
            return newVal;
        }
        auto val = eval(node->children[0], env);
        if (op == "-") {
            if (val->type == ValType::Number) return Value::makeNumber(-val->num);
            return Value::makeNumber(-valueToNumber(val));
        }
        if (op == "+") return Value::makeNumber(valueToNumber(val));
        if (op == "!") return Value::makeBool(!val->isTruthy());
        if (op == "~") return Value::makeNumber((double)(~(int)valueToNumber(val)));
        return Value::makeUndefined();
    }

    ValPtr evalPostfixOp(NodePtr node, EnvPtr env) {
        auto val = eval(node->children[0], env);
        double old = valueToNumber(val);
        double newNum = node->sval == "++" ? old + 1 : old - 1;
        assignToNode(node->children[0], Value::makeNumber(newNum), env);
        return Value::makeNumber(old);
    }

    void assignToNode(NodePtr lhs, ValPtr val, EnvPtr env) {
        if (lhs->type == NodeType::Identifier) {
            env->set(lhs->sval, val);
        } else if (lhs->type == NodeType::MemberAccess) {
            auto obj = eval(lhs->children[0], env);
            if (obj->type == ValType::Object || obj->type == ValType::Array) {
                if (obj->obj) obj->obj->set(lhs->sval, val);
            }
        } else if (lhs->type == NodeType::Index) {
            auto obj = eval(lhs->children[0], env);
            auto key = eval(lhs->children[1], env);
            std::string k = (key->type == ValType::Number) ? numToString(key->num) : valueToString(key);
            if ((obj->type == ValType::Object || obj->type == ValType::Array) && obj->obj)
                obj->obj->set(k, val);
            else if (obj->type == ValType::String) {
                // Can't assign to string index
            }
        }
    }

    ValPtr evalAssignment(NodePtr node, EnvPtr env) {
        std::string op = node->sval;
        ValPtr rhs = eval(node->children[1], env);

        if (op != "=") {
            auto lhs = eval(node->children[0], env);
            if (op == "+=") {
                if (lhs->type == ValType::String || rhs->type == ValType::String)
                    rhs = Value::makeString(valueToString(lhs) + valueToString(rhs));
                else rhs = Value::makeNumber(valueToNumber(lhs) + valueToNumber(rhs));
            } else if (op == "-=") rhs = Value::makeNumber(valueToNumber(lhs) - valueToNumber(rhs));
            else if (op == "*=") rhs = Value::makeNumber(valueToNumber(lhs) * valueToNumber(rhs));
            else if (op == "/=") rhs = Value::makeNumber(valueToNumber(lhs) / valueToNumber(rhs));
            else if (op == "%=") rhs = Value::makeNumber(std::fmod(valueToNumber(lhs), valueToNumber(rhs)));
        }

        // Handle destructuring assignment
        auto& lhsNode = node->children[0];
        if (lhsNode->type == NodeType::ArrayLiteral) {
            destructureArray(lhsNode, rhs, env);
            return rhs;
        }

        assignToNode(lhsNode, rhs, env);
        return rhs;
    }

    ValPtr evalMemberAccess(NodePtr node, EnvPtr env) {
        auto obj = eval(node->children[0], env);
        std::string prop = node->sval;
        bool optional = node->bval;
        if (optional && obj->isNullish()) return Value::makeUndefined();
        return getMember(obj, prop, env);
    }

    ValPtr evalIndex(NodePtr node, EnvPtr env) {
        auto obj = eval(node->children[0], env);
        auto key = eval(node->children[1], env);
        std::string k;
        if (key->type == ValType::Number) {
            double n = key->num;
            if (n == std::floor(n)) k = std::to_string((long long)n);
            else k = numToString(n);
        } else k = valueToString(key);
        return getMember(obj, k, env);
    }

    ValPtr getMember(ValPtr obj, const std::string& prop, EnvPtr env) {
        if (obj->type == ValType::String) {
            return getStringProp(obj, prop, env);
        }
        if (obj->type == ValType::Array) {
            return getArrayProp(obj, prop, env);
        }
        if (obj->type == ValType::Number) {
            return getNumberProp(obj, prop);
        }
        if (obj->type == ValType::Object && obj->obj) {
            auto v = obj->obj->get(prop);
            if (v && v->type != ValType::Undefined) return v;
            return Value::makeUndefined();
        }
        if (obj->type == ValType::Function || obj->type == ValType::NativeFunction) {
            if (prop == "call" || prop == "apply" || prop == "bind") return Value::makeNative([](std::vector<ValPtr> args)->ValPtr{ return Value::makeUndefined(); });
            if (prop == "length") return Value::makeNumber(0);
            if (prop == "name") return Value::makeString(obj->str);
        }
        return Value::makeUndefined();
    }

    ValPtr getNumberProp(ValPtr num, const std::string& prop) {
        if (prop == "toFixed") {
            double n = num->num;
            return Value::makeNative([n](std::vector<ValPtr> args)->ValPtr{
                int digits = args.empty() ? 0 : (int)valueToNumber(args[0]);
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(digits) << n;
                return Value::makeString(oss.str());
            });
        }
        if (prop == "toString") {
            double n = num->num;
            return Value::makeNative([n](std::vector<ValPtr> args)->ValPtr{
                int base = args.empty() ? 10 : (int)valueToNumber(args[0]);
                if (base == 10) return Value::makeString(numToString(n));
                // Convert to base
                if (base == 16) {
                    std::ostringstream oss; oss << std::hex << (long long)n;
                    return Value::makeString(oss.str());
                }
                return Value::makeString(numToString(n));
            });
        }
        return Value::makeUndefined();
    }

    ValPtr getStringProp(ValPtr strVal, const std::string& prop, EnvPtr env) {
        std::string s = strVal->str;
        if (prop == "length") return Value::makeNumber((double)s.size());
        if (prop == "toUpperCase") return Value::makeNative([s](std::vector<ValPtr>)->ValPtr{
            std::string r = s; for (auto& c : r) c = std::toupper(c); return Value::makeString(r);
        });
        if (prop == "toLowerCase") return Value::makeNative([s](std::vector<ValPtr>)->ValPtr{
            std::string r = s; for (auto& c : r) c = std::tolower(c); return Value::makeString(r);
        });
        if (prop == "trim") return Value::makeNative([s](std::vector<ValPtr>)->ValPtr{
            size_t start = s.find_first_not_of(" \t\n\r");
            if (start == std::string::npos) return Value::makeString("");
            size_t end = s.find_last_not_of(" \t\n\r");
            return Value::makeString(s.substr(start, end-start+1));
        });
        if (prop == "trimStart" || prop == "trimLeft") return Value::makeNative([s](std::vector<ValPtr>)->ValPtr{
            size_t start = s.find_first_not_of(" \t\n\r");
            return Value::makeString(start == std::string::npos ? "" : s.substr(start));
        });
        if (prop == "trimEnd" || prop == "trimRight") return Value::makeNative([s](std::vector<ValPtr>)->ValPtr{
            size_t end = s.find_last_not_of(" \t\n\r");
            return Value::makeString(end == std::string::npos ? "" : s.substr(0, end+1));
        });
        if (prop == "split") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            auto arr = std::make_shared<JSObject>(true);
            std::string delim = args.empty() || args[0]->type==ValType::Undefined ? "" : valueToString(args[0]);
            int limit = args.size()>1 && args[1]->type!=ValType::Undefined ? (int)valueToNumber(args[1]) : INT_MAX;
            if (delim.empty()) {
                for (char c : s) {
                    if ((int)arr->elements.size() >= limit) break;
                    arr->elements.push_back(Value::makeString(std::string(1,c)));
                }
            } else {
                size_t pos = 0, found;
                while ((int)arr->elements.size() < limit && (found = s.find(delim, pos)) != std::string::npos) {
                    arr->elements.push_back(Value::makeString(s.substr(pos, found-pos)));
                    pos = found + delim.size();
                }
                if ((int)arr->elements.size() < limit) arr->elements.push_back(Value::makeString(s.substr(pos)));
            }
            return Value::makeArray(arr);
        });
        if (prop == "indexOf") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            if (args.empty()) return Value::makeNumber(-1);
            std::string sub = valueToString(args[0]);
            int from = args.size()>1 ? (int)valueToNumber(args[1]) : 0;
            size_t pos = s.find(sub, from);
            return Value::makeNumber(pos == std::string::npos ? -1 : (double)pos);
        });
        if (prop == "lastIndexOf") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            if (args.empty()) return Value::makeNumber(-1);
            std::string sub = valueToString(args[0]);
            size_t pos = s.rfind(sub);
            return Value::makeNumber(pos == std::string::npos ? -1 : (double)pos);
        });
        if (prop == "includes") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            if (args.empty()) return Value::makeBool(false);
            return Value::makeBool(s.find(valueToString(args[0])) != std::string::npos);
        });
        if (prop == "startsWith") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            if (args.empty()) return Value::makeBool(false);
            std::string pre = valueToString(args[0]);
            return Value::makeBool(s.size() >= pre.size() && s.substr(0, pre.size()) == pre);
        });
        if (prop == "endsWith") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            if (args.empty()) return Value::makeBool(false);
            std::string suf = valueToString(args[0]);
            return Value::makeBool(s.size() >= suf.size() && s.substr(s.size()-suf.size()) == suf);
        });
        if (prop == "slice") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            int len = (int)s.size();
            int start = args.size()>0 ? (int)valueToNumber(args[0]) : 0;
            if (start < 0) start = std::max(0, len + start);
            else start = std::min(start, len);
            int end = args.size()>1 && args[1]->type!=ValType::Undefined ? (int)valueToNumber(args[1]) : len;
            if (end < 0) end = std::max(0, len + end);
            else end = std::min(end, len);
            if (start >= end) return Value::makeString("");
            return Value::makeString(s.substr(start, end-start));
        });
        if (prop == "substring") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            int len = (int)s.size();
            int start = args.size()>0 ? (int)valueToNumber(args[0]) : 0;
            int end = args.size()>1 && args[1]->type!=ValType::Undefined ? (int)valueToNumber(args[1]) : len;
            start = std::max(0, std::min(start, len));
            end = std::max(0, std::min(end, len));
            if (start > end) std::swap(start, end);
            return Value::makeString(s.substr(start, end-start));
        });
        if (prop == "charAt") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            int idx = args.empty() ? 0 : (int)valueToNumber(args[0]);
            if (idx < 0 || idx >= (int)s.size()) return Value::makeString("");
            return Value::makeString(std::string(1, s[idx]));
        });
        if (prop == "charCodeAt") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            int idx = args.empty() ? 0 : (int)valueToNumber(args[0]);
            if (idx < 0 || idx >= (int)s.size()) return Value::makeNumber(std::numeric_limits<double>::quiet_NaN());
            return Value::makeNumber((double)(unsigned char)s[idx]);
        });
        if (prop == "replace") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            if (args.size() < 2) return Value::makeString(s);
            std::string from = valueToString(args[0]);
            std::string to = valueToString(args[1]);
            std::string result = s;
            size_t pos = result.find(from);
            if (pos != std::string::npos) result.replace(pos, from.size(), to);
            return Value::makeString(result);
        });
        if (prop == "replaceAll") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            if (args.size() < 2) return Value::makeString(s);
            std::string from = valueToString(args[0]);
            std::string to = valueToString(args[1]);
            std::string result = s;
            size_t pos = 0;
            while ((pos = result.find(from, pos)) != std::string::npos) {
                result.replace(pos, from.size(), to);
                pos += to.size();
            }
            return Value::makeString(result);
        });
        if (prop == "repeat") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            int n = args.empty() ? 0 : (int)valueToNumber(args[0]);
            std::string result;
            for (int i = 0; i < n; i++) result += s;
            return Value::makeString(result);
        });
        if (prop == "padStart") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            int targetLen = args.empty() ? 0 : (int)valueToNumber(args[0]);
            std::string pad = args.size()>1 ? valueToString(args[1]) : " ";
            std::string result = s;
            while ((int)result.size() < targetLen) result = pad + result;
            return Value::makeString(result.substr(result.size() - std::max((int)s.size(), targetLen)));
        });
        if (prop == "padEnd") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            int targetLen = args.empty() ? 0 : (int)valueToNumber(args[0]);
            std::string pad = args.size()>1 ? valueToString(args[1]) : " ";
            std::string result = s;
            while ((int)result.size() < targetLen) result += pad;
            return Value::makeString(result.substr(0, targetLen));
        });
        if (prop == "concat") return Value::makeNative([s](std::vector<ValPtr> args)->ValPtr{
            std::string result = s;
            for (auto& a : args) result += valueToString(a);
            return Value::makeString(result);
        });
        if (prop == "toString" || prop == "valueOf") return Value::makeNative([s](std::vector<ValPtr>)->ValPtr{ return Value::makeString(s); });
        // Index access for strings
        try {
            int idx = std::stoi(prop);
            if (idx >= 0 && idx < (int)s.size()) return Value::makeString(std::string(1, s[idx]));
        } catch(...) {}
        return Value::makeUndefined();
    }

    ValPtr getArrayProp(ValPtr arrVal, const std::string& prop, EnvPtr env) {
        auto& obj = arrVal->obj;
        if (!obj) return Value::makeUndefined();

        if (prop == "length") return Value::makeNumber((double)obj->elements.size());
        if (prop == "push") return Value::makeNative([obj](std::vector<ValPtr> args)->ValPtr{
            for (auto& a : args) obj->elements.push_back(a);
            return Value::makeNumber((double)obj->elements.size());
        });
        if (prop == "pop") return Value::makeNative([obj](std::vector<ValPtr>)->ValPtr{
            if (obj->elements.empty()) return Value::makeUndefined();
            auto v = obj->elements.back(); obj->elements.pop_back(); return v;
        });
        if (prop == "shift") return Value::makeNative([obj](std::vector<ValPtr>)->ValPtr{
            if (obj->elements.empty()) return Value::makeUndefined();
            auto v = obj->elements.front(); obj->elements.erase(obj->elements.begin()); return v;
        });
        if (prop == "unshift") return Value::makeNative([obj](std::vector<ValPtr> args)->ValPtr{
            for (int i = (int)args.size()-1; i >= 0; i--) obj->elements.insert(obj->elements.begin(), args[i]);
            return Value::makeNumber((double)obj->elements.size());
        });
        if (prop == "reverse") return Value::makeNative([obj, arrVal](std::vector<ValPtr>)->ValPtr{
            std::reverse(obj->elements.begin(), obj->elements.end());
            return arrVal;
        });
        if (prop == "sort") return Value::makeNative([this, obj, arrVal](std::vector<ValPtr> args)->ValPtr{
            if (!args.empty() && (args[0]->type==ValType::Function || args[0]->type==ValType::NativeFunction)) {
                auto fn = args[0];
                std::stable_sort(obj->elements.begin(), obj->elements.end(), [this, &fn](ValPtr a, ValPtr b)->bool{
                    auto result = callFunction(fn, {a, b}, Value::makeUndefined());
                    return valueToNumber(result) < 0;
                });
            } else {
                std::stable_sort(obj->elements.begin(), obj->elements.end(), [](ValPtr a, ValPtr b)->bool{
                    return valueToString(a) < valueToString(b);
                });
            }
            return arrVal;
        });
        if (prop == "join") return Value::makeNative([obj](std::vector<ValPtr> args)->ValPtr{
            std::string sep = args.empty() || args[0]->type==ValType::Undefined ? "," : valueToString(args[0]);
            std::string result;
            for (int i = 0; i < (int)obj->elements.size(); i++) {
                if (i > 0) result += sep;
                auto& e = obj->elements[i];
                if (e->type != ValType::Null && e->type != ValType::Undefined)
                    result += valueToString(e);
            }
            return Value::makeString(result);
        });
        if (prop == "slice") return Value::makeNative([obj](std::vector<ValPtr> args)->ValPtr{
            int len = (int)obj->elements.size();
            int start = args.size()>0 && args[0]->type!=ValType::Undefined ? (int)valueToNumber(args[0]) : 0;
            if (start < 0) start = std::max(0, len+start);
            int end = args.size()>1 && args[1]->type!=ValType::Undefined ? (int)valueToNumber(args[1]) : len;
            if (end < 0) end = std::max(0, len+end);
            end = std::min(end, len); start = std::min(start, len);
            auto res = std::make_shared<JSObject>(true);
            for (int i = start; i < end; i++) res->elements.push_back(obj->elements[i]);
            return Value::makeArray(res);
        });
        if (prop == "splice") return Value::makeNative([obj](std::vector<ValPtr> args)->ValPtr{
            int len = (int)obj->elements.size();
            int start = args.size()>0 ? (int)valueToNumber(args[0]) : 0;
            if (start < 0) start = std::max(0, len+start);
            start = std::min(start, len);
            int deleteCount = args.size()>1 ? (int)valueToNumber(args[1]) : len-start;
            deleteCount = std::max(0, std::min(deleteCount, len-start));
            auto removed = std::make_shared<JSObject>(true);
            for (int i = 0; i < deleteCount; i++) removed->elements.push_back(obj->elements[start+i]);
            obj->elements.erase(obj->elements.begin()+start, obj->elements.begin()+start+deleteCount);
            for (int i = 2; i < (int)args.size(); i++)
                obj->elements.insert(obj->elements.begin()+start+(i-2), args[i]);
            return Value::makeArray(removed);
        });
        if (prop == "concat") return Value::makeNative([obj](std::vector<ValPtr> args)->ValPtr{
            auto res = std::make_shared<JSObject>(true);
            for (auto& e : obj->elements) res->elements.push_back(e);
            for (auto& a : args) {
                if (a->type == ValType::Array && a->obj) for (auto& e : a->obj->elements) res->elements.push_back(e);
                else res->elements.push_back(a);
            }
            return Value::makeArray(res);
        });
        if (prop == "includes") return Value::makeNative([obj](std::vector<ValPtr> args)->ValPtr{
            if (args.empty()) return Value::makeBool(false);
            for (auto& e : obj->elements) if (strictEqual(e, args[0])) return Value::makeBool(true);
            return Value::makeBool(false);
        });
        if (prop == "indexOf") return Value::makeNative([obj](std::vector<ValPtr> args)->ValPtr{
            if (args.empty()) return Value::makeNumber(-1);
            for (int i = 0; i < (int)obj->elements.size(); i++)
                if (strictEqual(obj->elements[i], args[0])) return Value::makeNumber(i);
            return Value::makeNumber(-1);
        });
        if (prop == "lastIndexOf") return Value::makeNative([obj](std::vector<ValPtr> args)->ValPtr{
            if (args.empty()) return Value::makeNumber(-1);
            for (int i = (int)obj->elements.size()-1; i >= 0; i--)
                if (strictEqual(obj->elements[i], args[0])) return Value::makeNumber(i);
            return Value::makeNumber(-1);
        });
        if (prop == "find") return Value::makeNative([this, obj](std::vector<ValPtr> args)->ValPtr{
            if (args.empty()) return Value::makeUndefined();
            for (int i = 0; i < (int)obj->elements.size(); i++) {
                auto r = callFunction(args[0], {obj->elements[i], Value::makeNumber(i)}, Value::makeUndefined());
                if (r->isTruthy()) return obj->elements[i];
            }
            return Value::makeUndefined();
        });
        if (prop == "findIndex") return Value::makeNative([this, obj](std::vector<ValPtr> args)->ValPtr{
            if (args.empty()) return Value::makeNumber(-1);
            for (int i = 0; i < (int)obj->elements.size(); i++) {
                auto r = callFunction(args[0], {obj->elements[i], Value::makeNumber(i)}, Value::makeUndefined());
                if (r->isTruthy()) return Value::makeNumber(i);
            }
            return Value::makeNumber(-1);
        });
        if (prop == "filter") return Value::makeNative([this, obj](std::vector<ValPtr> args)->ValPtr{
            auto res = std::make_shared<JSObject>(true);
            if (args.empty()) return Value::makeArray(res);
            for (int i = 0; i < (int)obj->elements.size(); i++) {
                auto r = callFunction(args[0], {obj->elements[i], Value::makeNumber(i)}, Value::makeUndefined());
                if (r->isTruthy()) res->elements.push_back(obj->elements[i]);
            }
            return Value::makeArray(res);
        });
        if (prop == "map") return Value::makeNative([this, obj](std::vector<ValPtr> args)->ValPtr{
            auto res = std::make_shared<JSObject>(true);
            if (args.empty()) return Value::makeArray(res);
            for (int i = 0; i < (int)obj->elements.size(); i++) {
                res->elements.push_back(callFunction(args[0], {obj->elements[i], Value::makeNumber(i)}, Value::makeUndefined()));
            }
            return Value::makeArray(res);
        });
        if (prop == "reduce") return Value::makeNative([this, obj](std::vector<ValPtr> args)->ValPtr{
            if (args.empty()) return Value::makeUndefined();
            auto fn = args[0];
            ValPtr acc;
            int start = 0;
            if (args.size() > 1) { acc = args[1]; }
            else if (!obj->elements.empty()) { acc = obj->elements[0]; start = 1; }
            else return Value::makeUndefined();
            for (int i = start; i < (int)obj->elements.size(); i++) {
                acc = callFunction(fn, {acc, obj->elements[i], Value::makeNumber(i)}, Value::makeUndefined());
            }
            return acc;
        });
        if (prop == "reduceRight") return Value::makeNative([this, obj](std::vector<ValPtr> args)->ValPtr{
            if (args.empty()) return Value::makeUndefined();
            auto fn = args[0];
            ValPtr acc;
            int start = (int)obj->elements.size()-1;
            if (args.size() > 1) { acc = args[1]; }
            else if (!obj->elements.empty()) { acc = obj->elements.back(); start--; }
            else return Value::makeUndefined();
            for (int i = start; i >= 0; i--) {
                acc = callFunction(fn, {acc, obj->elements[i], Value::makeNumber(i)}, Value::makeUndefined());
            }
            return acc;
        });
        if (prop == "some") return Value::makeNative([this, obj](std::vector<ValPtr> args)->ValPtr{
            if (args.empty()) return Value::makeBool(false);
            for (int i = 0; i < (int)obj->elements.size(); i++) {
                auto r = callFunction(args[0], {obj->elements[i], Value::makeNumber(i)}, Value::makeUndefined());
                if (r->isTruthy()) return Value::makeBool(true);
            }
            return Value::makeBool(false);
        });
        if (prop == "every") return Value::makeNative([this, obj](std::vector<ValPtr> args)->ValPtr{
            if (args.empty()) return Value::makeBool(true);
            for (int i = 0; i < (int)obj->elements.size(); i++) {
                auto r = callFunction(args[0], {obj->elements[i], Value::makeNumber(i)}, Value::makeUndefined());
                if (!r->isTruthy()) return Value::makeBool(false);
            }
            return Value::makeBool(true);
        });
        if (prop == "flat") return Value::makeNative([obj](std::vector<ValPtr> args)->ValPtr{
            int depth = args.empty() || args[0]->type==ValType::Undefined ? 1 : (int)valueToNumber(args[0]);
            auto res = std::make_shared<JSObject>(true);
            std::function<void(std::vector<ValPtr>&, int)> flatten = [&](std::vector<ValPtr>& elems, int d) {
                for (auto& e : elems) {
                    if (d > 0 && e->type == ValType::Array && e->obj) flatten(e->obj->elements, d-1);
                    else res->elements.push_back(e);
                }
            };
            flatten(obj->elements, depth);
            return Value::makeArray(res);
        });
        if (prop == "flatMap") return Value::makeNative([this, obj](std::vector<ValPtr> args)->ValPtr{
            auto res = std::make_shared<JSObject>(true);
            if (args.empty()) return Value::makeArray(res);
            for (int i = 0; i < (int)obj->elements.size(); i++) {
                auto r = callFunction(args[0], {obj->elements[i], Value::makeNumber(i)}, Value::makeUndefined());
                if (r->type == ValType::Array && r->obj) for (auto& e : r->obj->elements) res->elements.push_back(e);
                else res->elements.push_back(r);
            }
            return Value::makeArray(res);
        });
        if (prop == "forEach") return Value::makeNative([this, obj](std::vector<ValPtr> args)->ValPtr{
            if (!args.empty()) {
                for (int i = 0; i < (int)obj->elements.size(); i++)
                    callFunction(args[0], {obj->elements[i], Value::makeNumber(i)}, Value::makeUndefined());
            }
            return Value::makeUndefined();
        });
        if (prop == "fill") return Value::makeNative([obj, arrVal](std::vector<ValPtr> args)->ValPtr{
            ValPtr fillVal = args.empty() ? Value::makeUndefined() : args[0];
            int len = (int)obj->elements.size();
            int start = args.size()>1 && args[1]->type!=ValType::Undefined ? (int)valueToNumber(args[1]) : 0;
            int end = args.size()>2 && args[2]->type!=ValType::Undefined ? (int)valueToNumber(args[2]) : len;
            if (start < 0) start = std::max(0, len+start);
            if (end < 0) end = std::max(0, len+end);
            for (int i = start; i < end && i < len; i++) obj->elements[i] = fillVal;
            return arrVal;
        });
        if (prop == "toString") return Value::makeNative([obj](std::vector<ValPtr>)->ValPtr{
            std::string s;
            for (int i = 0; i < (int)obj->elements.size(); i++) {
                if (i > 0) s += ",";
                s += valueToString(obj->elements[i]);
            }
            return Value::makeString(s);
        });
        // Index
        try {
            int idx = std::stoi(prop);
            if (idx >= 0 && idx < (int)obj->elements.size()) return obj->elements[idx];
            return Value::makeUndefined();
        } catch(...) {}
        auto it = obj->props.find(prop);
        if (it != obj->props.end()) return it->second;
        return Value::makeUndefined();
    }

    // ---- Call ----
    ValPtr evalCall(NodePtr node, EnvPtr env) {
        ValPtr thisVal = Value::makeUndefined();
        ValPtr fn;

        auto& calleeNode = node->children[0];

        // Determine 'this'
        if (calleeNode->type == NodeType::MemberAccess) {
            thisVal = eval(calleeNode->children[0], env);
            fn = getMember(thisVal, calleeNode->sval, env);
        } else if (calleeNode->type == NodeType::Index) {
            thisVal = eval(calleeNode->children[0], env);
            auto key = eval(calleeNode->children[1], env);
            fn = getMember(thisVal, valueToString(key), env);
        } else {
            fn = eval(calleeNode, env);
        }

        // Collect args
        std::vector<ValPtr> args;
        for (int i = 1; i < (int)node->children.size(); i++) {
            auto& arg = node->children[i];
            if (arg->type == NodeType::SpreadElement) {
                auto spread = eval(arg->children[0], env);
                if (spread->type == ValType::Array && spread->obj)
                    for (auto& e : spread->obj->elements) args.push_back(e);
                else args.push_back(spread);
            } else {
                args.push_back(eval(arg, env));
            }
        }

        return callFunction(fn, args, thisVal);
    }

    ValPtr callFunction(ValPtr fn, std::vector<ValPtr> args, ValPtr thisVal) {
        if (!fn) return Value::makeUndefined();

        if (fn->type == ValType::NativeFunction) {
            return fn->native(args);
        }

        if (fn->type == ValType::Function) {
            auto fnEnv = std::make_shared<Environment>(fn->closure);
            fnEnv->setLocal("this", thisVal ? thisVal : Value::makeUndefined());

            // Bind params
            auto& params = fn->params;
            for (int i = 0; i < (int)params.size(); i++) {
                std::string pname = params[i];
                if (pname.find("...") == 0) {
                    // Rest param
                    std::string restName = pname.substr(3);
                    auto restArr = std::make_shared<JSObject>(true);
                    for (int j = i; j < (int)args.size(); j++) restArr->elements.push_back(args[j]);
                    fnEnv->setLocal(restName, Value::makeArray(restArr));
                    break;
                }
                ValPtr argVal = i < (int)args.size() ? args[i] : Value::makeUndefined();
                // Check default value
                if ((argVal->type == ValType::Undefined) && fn->obj) {
                    std::string defKey = "__default__" + pname;
                    auto it = fn->obj->props.find(defKey);
                    if (it != fn->obj->props.end()) argVal = it->second;
                }
                fnEnv->setLocal(pname, argVal);
            }

            // arguments object
            auto argsArr = std::make_shared<JSObject>(true);
            for (auto& a : args) argsArr->elements.push_back(a);
            fnEnv->setLocal("arguments", Value::makeArray(argsArr));

            try {
                eval(fn->funcBody, fnEnv);
            } catch(ReturnSignal& r) {
                return r.value;
            }
            return Value::makeUndefined();
        }

        if (fn->type == ValType::Object && fn->obj) {
            // Callable object (shouldn't happen normally)
        }

        return Value::makeUndefined();
    }

    ValPtr evalNew(NodePtr node, EnvPtr env) {
        auto callee = eval(node->children[0], env);
        std::vector<ValPtr> args;
        for (int i = 1; i < (int)node->children.size(); i++) {
            auto& arg = node->children[i];
            if (arg->type == NodeType::SpreadElement) {
                auto spread = eval(arg->children[0], env);
                if (spread->type == ValType::Array && spread->obj)
                    for (auto& e : spread->obj->elements) args.push_back(e);
            } else {
                args.push_back(eval(arg, env));
            }
        }

        if (callee->type == ValType::NativeFunction) {
            return callee->native(args);
        }

        if (callee->type == ValType::Function) {
            auto obj = std::make_shared<JSObject>();
            auto thisVal = Value::makeObject(obj);
            auto fnEnv = std::make_shared<Environment>(callee->closure);
            fnEnv->setLocal("this", thisVal);
            auto& params = callee->params;
            for (int i = 0; i < (int)params.size(); i++) {
                std::string pname = params[i];
                if (pname.find("...") == 0) {
                    auto restArr = std::make_shared<JSObject>(true);
                    for (int j = i; j < (int)args.size(); j++) restArr->elements.push_back(args[j]);
                    fnEnv->setLocal(pname.substr(3), Value::makeArray(restArr));
                    break;
                }
                fnEnv->setLocal(pname, i < (int)args.size() ? args[i] : Value::makeUndefined());
            }
            try { eval(callee->funcBody, fnEnv); }
            catch(ReturnSignal& r) { if (r.value->type != ValType::Undefined) return r.value; }
            return thisVal;
        }

        return Value::makeUndefined();
    }

    ValPtr evalTypeof(NodePtr node, EnvPtr env) {
        ValPtr val;
        // typeof on undefined variable should not throw
        if (node->children[0]->type == NodeType::Identifier) {
            std::string name = node->children[0]->sval;
            val = env->get(name); // returns undefined if not found
        } else {
            val = eval(node->children[0], env);
        }
        if (!val) return Value::makeString("undefined");
        switch(val->type) {
            case ValType::Undefined: return Value::makeString("undefined");
            case ValType::Null: return Value::makeString("object");
            case ValType::Boolean: return Value::makeString("boolean");
            case ValType::Number: return Value::makeString("number");
            case ValType::String: return Value::makeString("string");
            case ValType::Function:
            case ValType::NativeFunction: return Value::makeString("function");
            case ValType::Object:
            case ValType::Array: return Value::makeString("object");
        }
        return Value::makeString("undefined");
    }

    ValPtr evalArrayLiteral(NodePtr node, EnvPtr env) {
        auto obj = std::make_shared<JSObject>(true);
        for (auto& child : node->children) {
            if (child->type == NodeType::SpreadElement) {
                auto spread = eval(child->children[0], env);
                if (spread->type == ValType::Array && spread->obj)
                    for (auto& e : spread->obj->elements) obj->elements.push_back(e);
                else obj->elements.push_back(spread);
            } else {
                obj->elements.push_back(eval(child, env));
            }
        }
        return Value::makeArray(obj);
    }

    ValPtr evalObjectLiteral(NodePtr node, EnvPtr env) {
        auto obj = std::make_shared<JSObject>();
        for (auto& prop : node->children) {
            if (prop->type == NodeType::SpreadElement) {
                auto spread = eval(prop->children[0], env);
                if ((spread->type == ValType::Object) && spread->obj) {
                    for (auto it4 = spread->obj->props.begin(); it4 != spread->obj->props.end(); ++it4) obj->props[it4->first] = it4->second;
                }
                continue;
            }
            std::string key;
            if (prop->computed && !prop->children.empty()) {
                key = valueToString(eval(prop->children[0], env));
                auto val = eval(prop->children[1], env);
                obj->props[key] = val;
            } else {
                key = prop->sval;
                if (!prop->children.empty()) {
                    auto val = eval(prop->children[0], env);
                    // If it's a function, set its name
                    if (val->type == ValType::Function && val->str.empty()) val->str = key;
                    obj->props[key] = val;
                }
            }
        }
        return Value::makeObject(obj);
    }
};

#endif // INTERPRETER_H