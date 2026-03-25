#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cctype>
#include <stdexcept>

class Json {
public:
    enum Type { NULL_VAL, BOOL, NUM, STR, ARR, OBJ };
    Type type;
    bool bVal = false;
    double nVal = 0.0;
    std::string sVal;
    std::vector<Json> aVal;
    std::map<std::string, Json> oVal;

    Json() : type(NULL_VAL) {}
    Json(Type t) : type(t) {}
    Json(bool b) : type(BOOL), bVal(b) {}
    Json(double n) : type(NUM), nVal(n) {}
    Json(int n) : type(NUM), nVal(n) {}
    Json(unsigned int n) : type(NUM), nVal(n) {}
    Json(const std::string& s) : type(STR), sVal(s) {}
    Json(const char* s) : type(STR), sVal(s) {}

    Json& operator[](const std::string& key) {
        if (type != OBJ) type = OBJ;
        return oVal[key];
    }
    const Json& operator[](const std::string& key) const {
        static Json null_json;
        if (type != OBJ) return null_json;
        auto it = oVal.find(key);
        if (it != oVal.end()) return it->second;
        return null_json;
    }

    void push_back(const Json& v) {
        if (type != ARR) type = ARR;
        aVal.push_back(v);
    }

    bool is_null() const { return type == NULL_VAL; }
    bool is_bool() const { return type == BOOL; }
    bool is_number() const { return type == NUM; }
    bool is_string() const { return type == STR; }
    bool is_array() const { return type == ARR; }
    bool is_object() const { return type == OBJ; }
    bool contains(const std::string& key) const { return type == OBJ && oVal.find(key) != oVal.end(); }

    std::string stringify() const {
        switch (type) {
            case NULL_VAL: return "null";
            case BOOL: return bVal ? "true" : "false";
            case NUM: {
                if (nVal == (int)nVal) return std::to_string((int)nVal);
                return std::to_string(nVal);
            }
            case STR: {
                std::string res = "\"";
                for (char c : sVal) {
                    if (c == '"') res += "\\\"";
                    else if (c == '\\') res += "\\\\";
                    else if (c == '\n') res += "\\n";
                    else if (c == '\r') res += "\\r";
                    else if (c == '\t') res += "\\t";
                    else res += c;
                }
                res += "\"";
                return res;
            }
            case ARR: {
                std::string res = "[";
                for (size_t i = 0; i < aVal.size(); ++i) {
                    res += aVal[i].stringify();
                    if (i < aVal.size() - 1) res += ",";
                }
                res += "]";
                return res;
            }
            case OBJ: {
                std::string res = "{";
                bool first = true;
                for (const auto& kv : oVal) {
                    if (!first) res += ",";
                    Json k(kv.first);
                    res += k.stringify() + ":" + kv.second.stringify();
                    first = false;
                }
                res += "}";
                return res;
            }
        }
        return "null";
    }

    static Json parse(const std::string& s, size_t& i) {
        auto skip_whitespace = [&]() {
            while (i < s.length() && std::isspace(s[i])) i++;
        };
        skip_whitespace();
        if (i >= s.length()) return Json();

        char c = s[i];
        if (c == '{') {
            Json obj(OBJ);
            i++; skip_whitespace();
            if (i < s.length() && s[i] == '}') { i++; return obj; }
            while (i < s.length()) {
                skip_whitespace();
                Json key = parse(s, i);
                skip_whitespace();
                if (i < s.length() && s[i] == ':') i++;
                skip_whitespace();
                Json val = parse(s, i);
                if (key.type == STR) obj.oVal[key.sVal] = val;
                skip_whitespace();
                if (i < s.length() && s[i] == ',') { i++; }
                else if (i < s.length() && s[i] == '}') { i++; break; }
            }
            return obj;
        } else if (c == '[') {
            Json arr(ARR);
            i++; skip_whitespace();
            if (i < s.length() && s[i] == ']') { i++; return arr; }
            while (i < s.length()) {
                skip_whitespace();
                arr.push_back(parse(s, i));
                skip_whitespace();
                if (i < s.length() && s[i] == ',') { i++; }
                else if (i < s.length() && s[i] == ']') { i++; break; }
            }
            return arr;
        } else if (c == '"') {
            std::string str;
            i++; // skip '"'
            while (i < s.length() && s[i] != '"') {
                if (s[i] == '\\' && i + 1 < s.length()) {
                    i++;
                    if (s[i] == 'n') str += '\n';
                    else if (s[i] == 't') str += '\t';
                    else if (s[i] == 'r') str += '\r';
                    else str += s[i];
                } else {
                    str += s[i];
                }
                i++;
            }
            if (i < s.length()) i++; // skip ending '"'
            return Json(str);
        } else if (std::isdigit(c) || c == '-') {
            size_t start = i;
            while (i < s.length() && (std::isdigit(s[i]) || s[i] == '.' || s[i] == 'e' || s[i] == 'E' || s[i] == '-' || s[i] == '+')) i++;
            std::string num_str = s.substr(start, i - start);
            try {
                return Json(std::stod(num_str));
            } catch (...) {
                return Json(0.0);
            }
        } else if (s.compare(i, 4, "true") == 0) {
            i += 4; return Json(true);
        } else if (s.compare(i, 5, "false") == 0) {
            i += 5; return Json(false);
        } else if (s.compare(i, 4, "null") == 0) {
            i += 4; return Json();
        }
        i++; // fallback to prevent infinite loop
        return Json();
    }

    static Json parse(const std::string& s) {
        size_t i = 0;
        return parse(s, i);
    }
};
