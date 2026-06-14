#ifndef LEXER_H
#define LEXER_H
#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>
#include <unordered_map>

enum class TokenType {
    // Literals
    NUMBER, STRING, BOOLEAN, NULL_TOK, UNDEFINED,
    IDENTIFIER,
    // Keywords
    LET, CONST, VAR, FUNCTION, RETURN, IF, ELSE, WHILE, FOR, DO,
    NEW, THIS, TYPEOF, INSTANCEOF, IN, OF, SWITCH, CASE, BREAK, DEFAULT,
    CONTINUE, THROW, TRY, CATCH, FINALLY, CLASS, EXTENDS, IMPORT, EXPORT,
    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT, STARSTAR,
    EQ, NEQ, STRICT_EQ, STRICT_NEQ,
    LT, GT, LTE, GTE,
    AND, OR, NOT, NULLISH,
    ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN, STAR_ASSIGN, SLASH_ASSIGN, PERCENT_ASSIGN,
    INCREMENT, DECREMENT,
    ARROW,
    BITAND, BITOR, BITXOR, BITNOT, LSHIFT, RSHIFT, URSHIFT,
    QUESTION, COLON,
    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    SEMICOLON, COMMA, DOT, DOTDOTDOT, OPTIONAL_CHAIN,
    // Special
    EOF_TOK, NEWLINE
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    Token(TokenType t, std::string v, int l=0): type(t), value(std::move(v)), line(l) {}
};

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"let", TokenType::LET}, {"const", TokenType::CONST}, {"var", TokenType::VAR},
    {"function", TokenType::FUNCTION}, {"return", TokenType::RETURN},
    {"if", TokenType::IF}, {"else", TokenType::ELSE},
    {"while", TokenType::WHILE}, {"for", TokenType::FOR}, {"do", TokenType::DO},
    {"new", TokenType::NEW}, {"this", TokenType::THIS},
    {"typeof", TokenType::TYPEOF}, {"instanceof", TokenType::INSTANCEOF},
    {"in", TokenType::IN}, {"of", TokenType::OF},
    {"switch", TokenType::SWITCH}, {"case", TokenType::CASE},
    {"break", TokenType::BREAK}, {"default", TokenType::DEFAULT},
    {"continue", TokenType::CONTINUE}, {"throw", TokenType::THROW},
    {"try", TokenType::TRY}, {"catch", TokenType::CATCH}, {"finally", TokenType::FINALLY},
    {"true", TokenType::BOOLEAN}, {"false", TokenType::BOOLEAN},
    {"null", TokenType::NULL_TOK}, {"undefined", TokenType::UNDEFINED},
    {"class", TokenType::CLASS}, {"extends", TokenType::EXTENDS},
};

class Lexer {
public:
    std::string src;
    int pos = 0, line = 1;

    Lexer(const std::string& s): src(s) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (pos < (int)src.size()) {
            skipWhitespaceAndComments();
            if (pos >= (int)src.size()) break;
            char c = src[pos];
            int ln = line;

            if (c == '\n') { line++; pos++; continue; }
            if (c == '\r') { pos++; continue; }

            if (std::isdigit(c) || (c == '.' && pos+1 < (int)src.size() && std::isdigit(src[pos+1]))) {
                tokens.push_back(readNumber(ln)); continue;
            }
            if (c == '"' || c == '\'' || c == '`') {
                tokens.push_back(readString(ln)); continue;
            }
            if (std::isalpha(c) || c == '_' || c == '$') {
                tokens.push_back(readIdentOrKeyword(ln)); continue;
            }

            // Multi-char operators
            if (pos+2 < (int)src.size()) {
                std::string three = src.substr(pos, 3);
                if (three == "===") { tokens.emplace_back(TokenType::STRICT_EQ, three, ln); pos+=3; continue; }
                if (three == "!==") { tokens.emplace_back(TokenType::STRICT_NEQ, three, ln); pos+=3; continue; }
                if (three == ">>>") { tokens.emplace_back(TokenType::URSHIFT, three, ln); pos+=3; continue; }
                if (three == "**=") { tokens.emplace_back(TokenType::STAR_ASSIGN, three, ln); pos+=3; continue; }
                if (three == "...") { tokens.emplace_back(TokenType::DOTDOTDOT, three, ln); pos+=3; continue; }
                if (three == "??=") { tokens.emplace_back(TokenType::NULLISH, three, ln); pos+=3; continue; }
            }
            if (pos+1 < (int)src.size()) {
                std::string two = src.substr(pos, 2);
                if (two == "==") { tokens.emplace_back(TokenType::EQ, two, ln); pos+=2; continue; }
                if (two == "!=") { tokens.emplace_back(TokenType::NEQ, two, ln); pos+=2; continue; }
                if (two == "<=") { tokens.emplace_back(TokenType::LTE, two, ln); pos+=2; continue; }
                if (two == ">=") { tokens.emplace_back(TokenType::GTE, two, ln); pos+=2; continue; }
                if (two == "&&") { tokens.emplace_back(TokenType::AND, two, ln); pos+=2; continue; }
                if (two == "||") { tokens.emplace_back(TokenType::OR, two, ln); pos+=2; continue; }
                if (two == "??") { tokens.emplace_back(TokenType::NULLISH, two, ln); pos+=2; continue; }
                if (two == "**") { tokens.emplace_back(TokenType::STARSTAR, two, ln); pos+=2; continue; }
                if (two == "++") { tokens.emplace_back(TokenType::INCREMENT, two, ln); pos+=2; continue; }
                if (two == "--") { tokens.emplace_back(TokenType::DECREMENT, two, ln); pos+=2; continue; }
                if (two == "+=") { tokens.emplace_back(TokenType::PLUS_ASSIGN, two, ln); pos+=2; continue; }
                if (two == "-=") { tokens.emplace_back(TokenType::MINUS_ASSIGN, two, ln); pos+=2; continue; }
                if (two == "*=") { tokens.emplace_back(TokenType::STAR_ASSIGN, two, ln); pos+=2; continue; }
                if (two == "/=") { tokens.emplace_back(TokenType::SLASH_ASSIGN, two, ln); pos+=2; continue; }
                if (two == "%=") { tokens.emplace_back(TokenType::PERCENT_ASSIGN, two, ln); pos+=2; continue; }
                if (two == "=>") { tokens.emplace_back(TokenType::ARROW, two, ln); pos+=2; continue; }
                if (two == "<<") { tokens.emplace_back(TokenType::LSHIFT, two, ln); pos+=2; continue; }
                if (two == ">>") { tokens.emplace_back(TokenType::RSHIFT, two, ln); pos+=2; continue; }
                if (two == "?.") { tokens.emplace_back(TokenType::OPTIONAL_CHAIN, two, ln); pos+=2; continue; }
            }

            // Single char
            switch(c) {
                case '+': tokens.emplace_back(TokenType::PLUS, "+", ln); break;
                case '-': tokens.emplace_back(TokenType::MINUS, "-", ln); break;
                case '*': tokens.emplace_back(TokenType::STAR, "*", ln); break;
                case '/': tokens.emplace_back(TokenType::SLASH, "/", ln); break;
                case '%': tokens.emplace_back(TokenType::PERCENT, "%", ln); break;
                case '<': tokens.emplace_back(TokenType::LT, "<", ln); break;
                case '>': tokens.emplace_back(TokenType::GT, ">", ln); break;
                case '!': tokens.emplace_back(TokenType::NOT, "!", ln); break;
                case '=': tokens.emplace_back(TokenType::ASSIGN, "=", ln); break;
                case '(': tokens.emplace_back(TokenType::LPAREN, "(", ln); break;
                case ')': tokens.emplace_back(TokenType::RPAREN, ")", ln); break;
                case '{': tokens.emplace_back(TokenType::LBRACE, "{", ln); break;
                case '}': tokens.emplace_back(TokenType::RBRACE, "}", ln); break;
                case '[': tokens.emplace_back(TokenType::LBRACKET, "[", ln); break;
                case ']': tokens.emplace_back(TokenType::RBRACKET, "]", ln); break;
                case ';': tokens.emplace_back(TokenType::SEMICOLON, ";", ln); break;
                case ',': tokens.emplace_back(TokenType::COMMA, ",", ln); break;
                case '.': tokens.emplace_back(TokenType::DOT, ".", ln); break;
                case '&': tokens.emplace_back(TokenType::BITAND, "&", ln); break;
                case '|': tokens.emplace_back(TokenType::BITOR, "|", ln); break;
                case '^': tokens.emplace_back(TokenType::BITXOR, "^", ln); break;
                case '~': tokens.emplace_back(TokenType::BITNOT, "~", ln); break;
                case '?': tokens.emplace_back(TokenType::QUESTION, "?", ln); break;
                case ':': tokens.emplace_back(TokenType::COLON, ":", ln); break;
                default: pos++; continue;
            }
            pos++;
        }
        tokens.emplace_back(TokenType::EOF_TOK, "", line);
        return tokens;
    }

private:
    void skipWhitespaceAndComments() {
        while (pos < (int)src.size()) {
            char c = src[pos];
            if (c == ' ' || c == '\t' || c == '\r') { pos++; continue; }
            if (c == '\n') break; // preserve newlines for ASI
            // Single line comment
            if (c == '/' && pos+1 < (int)src.size() && src[pos+1] == '/') {
                while (pos < (int)src.size() && src[pos] != '\n') pos++;
                continue;
            }
            // Multi-line comment
            if (c == '/' && pos+1 < (int)src.size() && src[pos+1] == '*') {
                pos += 2;
                while (pos+1 < (int)src.size() && !(src[pos]=='*' && src[pos+1]=='/')) {
                    if (src[pos] == '\n') line++;
                    pos++;
                }
                pos += 2;
                continue;
            }
            break;
        }
    }

    Token readNumber(int ln) {
        std::string num;
        bool isHex = false;
        if (pos+1 < (int)src.size() && src[pos]=='0' && (src[pos+1]=='x'||src[pos+1]=='X')) {
            num += src[pos++]; num += src[pos++];
            while (pos < (int)src.size() && std::isxdigit(src[pos])) num += src[pos++];
            isHex = true;
        } else {
            while (pos < (int)src.size() && (std::isdigit(src[pos]) || src[pos]=='.')) {
                if (src[pos]=='.' && num.find('.')!=std::string::npos) break;
                num += src[pos++];
            }
            if (pos < (int)src.size() && (src[pos]=='e'||src[pos]=='E')) {
                num += src[pos++];
                if (pos < (int)src.size() && (src[pos]=='+'||src[pos]=='-')) num += src[pos++];
                while (pos < (int)src.size() && std::isdigit(src[pos])) num += src[pos++];
            }
        }
        return Token(TokenType::NUMBER, num, ln);
    }

    Token readString(int ln) {
        char quote = src[pos++];
        std::string val;
        bool isTemplate = (quote == '`');
        while (pos < (int)src.size()) {
            char c = src[pos];
            if (c == quote) { pos++; break; }
            if (c == '\n') line++;
            if (c == '\\' && pos+1 < (int)src.size()) {
                pos++;
                char esc = src[pos++];
                switch(esc) {
                    case 'n': val += '\n'; break;
                    case 't': val += '\t'; break;
                    case 'r': val += '\r'; break;
                    case '\\': val += '\\'; break;
                    case '\'': val += '\''; break;
                    case '"': val += '"'; break;
                    case '`': val += '`'; break;
                    default: val += '\\'; val += esc; break;
                }
                continue;
            }
            val += src[pos++];
        }
        return Token(TokenType::STRING, val, ln);
    }

    Token readIdentOrKeyword(int ln) {
        std::string ident;
        while (pos < (int)src.size() && (std::isalnum(src[pos]) || src[pos]=='_' || src[pos]=='$'))
            ident += src[pos++];
        auto it = KEYWORDS.find(ident);
        if (it != KEYWORDS.end()) return Token(it->second, ident, ln);
        return Token(TokenType::IDENTIFIER, ident, ln);
    }
};

#endif // LEXER_H