#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"
#include "ast.h"
#include <stdexcept>

class Parser {
    std::vector<Token> tokens;
    int pos = 0;

    Token& cur() { return tokens[pos]; }
    Token& peek(int offset=1) { 
        int p = pos + offset;
        if (p >= (int)tokens.size()) return tokens.back();
        return tokens[p];
    }

    bool check(TokenType t) { return cur().type == t; }

    Token consume() { return tokens[pos++]; }

    Token expect(TokenType t, const std::string& msg="") {
        if (!check(t)) {
            throw std::runtime_error("Parse error at '" + cur().value + "' (line " + std::to_string(cur().line) + "): " + msg);
        }
        return consume();
    }

    bool match(TokenType t) {
        if (check(t)) { consume(); return true; }
        return false;
    }

    void skipSemis() {
        while (check(TokenType::SEMICOLON)) consume();
    }

public:
    Parser(std::vector<Token> toks): tokens(std::move(toks)) {}

    NodePtr parse() {
        auto prog = makeNode(NodeType::Program);
        while (!check(TokenType::EOF_TOK)) {
            skipSemis();
            if (check(TokenType::EOF_TOK)) break;
            prog->children.push_back(parseStatement());
        }
        return prog;
    }

private:
    NodePtr parseStatement() {
        skipSemis();
        auto t = cur().type;

        if (t == TokenType::LET || t == TokenType::CONST || t == TokenType::VAR)
            return parseVarDecl();
        if (t == TokenType::FUNCTION)
            return parseFuncDecl();
        if (t == TokenType::IF)
            return parseIf();
        if (t == TokenType::WHILE)
            return parseWhile();
        if (t == TokenType::FOR)
            return parseFor();
        if (t == TokenType::DO)
            return parseDoWhile();
        if (t == TokenType::RETURN)
            return parseReturn();
        if (t == TokenType::SWITCH)
            return parseSwitch();
        if (t == TokenType::BREAK) {
            consume();
            match(TokenType::SEMICOLON);
            return makeNode(NodeType::Break);
        }
        if (t == TokenType::CONTINUE) {
            consume();
            match(TokenType::SEMICOLON);
            return makeNode(NodeType::Continue);
        }
        if (t == TokenType::THROW)
            return parseThrow();
        if (t == TokenType::TRY)
            return parseTry();
        if (t == TokenType::CLASS)
            return parseClassDecl();
        if (t == TokenType::LBRACE)
            return parseBlock();

        // Expression statement
        auto expr = parseExpr();
        match(TokenType::SEMICOLON);
        auto stmt = makeNode(NodeType::ExprStmt);
        stmt->children.push_back(expr);
        return stmt;
    }

    NodePtr parseBlock() {
        expect(TokenType::LBRACE);
        auto block = makeNode(NodeType::Block);
        while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOK)) {
            skipSemis();
            if (check(TokenType::RBRACE)) break;
            block->children.push_back(parseStatement());
        }
        expect(TokenType::RBRACE);
        return block;
    }

    NodePtr parseVarDecl() {
        bool isConst = cur().type == TokenType::CONST;
        consume(); // let/const/var
        auto decl = makeNode(NodeType::VarDecl);
        decl->is_const = isConst;

        // Support destructuring: let [a, b] = ...
        if (check(TokenType::LBRACKET) || check(TokenType::LBRACE)) {
            // Simple: treat as single ident for now with pattern stored in sval
            // For now parse as a pattern node
            auto pat = parseDestructPattern();
            decl->children.push_back(pat);
            if (match(TokenType::ASSIGN)) {
                decl->children.push_back(parseExpr());
            }
            match(TokenType::SEMICOLON);
            return decl;
        }

        // Parse multiple declarators: let a = 1, b = 2;
        while (true) {
            auto id = expect(TokenType::IDENTIFIER, "Expected identifier in variable declaration");
            auto varNode = makeNode(NodeType::Identifier);
            varNode->sval = id.value;
            if (match(TokenType::ASSIGN)) {
                varNode->children.push_back(parseAssignment());
            }
            decl->children.push_back(varNode);
            if (!match(TokenType::COMMA)) break;
        }
        match(TokenType::SEMICOLON);
        return decl;
    }

    NodePtr parseDestructPattern() {
        // Returns an ArrayLiteral or ObjectLiteral node representing the pattern
        if (check(TokenType::LBRACKET)) {
            consume();
            auto arr = makeNode(NodeType::ArrayLiteral);
            arr->sval = "__destruct_array__";
            while (!check(TokenType::RBRACKET) && !check(TokenType::EOF_TOK)) {
                if (check(TokenType::COMMA)) { 
                    consume(); 
                    auto hole = makeNode(NodeType::Identifier);
                    hole->sval = "__hole__";
                    arr->children.push_back(hole);
                    continue; 
                }
                if (check(TokenType::DOTDOTDOT)) {
                    consume();
                    auto rest = makeNode(NodeType::SpreadElement);
                    rest->sval = expect(TokenType::IDENTIFIER).value;
                    arr->children.push_back(rest);
                    break;
                }
                auto id = makeNode(NodeType::Identifier);
                id->sval = expect(TokenType::IDENTIFIER).value;
                // Default value
                if (match(TokenType::ASSIGN)) {
                    id->children.push_back(parseAssignment());
                }
                arr->children.push_back(id);
                if (!match(TokenType::COMMA)) break;
            }
            expect(TokenType::RBRACKET);
            return arr;
        } else {
            consume(); // {
            auto obj = makeNode(NodeType::ObjectLiteral);
            obj->sval = "__destruct_obj__";
            while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOK)) {
                if (check(TokenType::DOTDOTDOT)) {
                    consume();
                    auto rest = makeNode(NodeType::SpreadElement);
                    rest->sval = expect(TokenType::IDENTIFIER).value;
                    obj->children.push_back(rest);
                    break;
                }
                auto key = expect(TokenType::IDENTIFIER).value;
                std::string alias = key;
                if (match(TokenType::COLON)) alias = expect(TokenType::IDENTIFIER).value;
                auto prop = makeNode(NodeType::ObjectProp);
                prop->sval = key;
                auto id = makeNode(NodeType::Identifier);
                id->sval = alias;
                if (match(TokenType::ASSIGN)) id->children.push_back(parseAssignment());
                prop->children.push_back(id);
                obj->children.push_back(prop);
                if (!match(TokenType::COMMA)) break;
            }
            expect(TokenType::RBRACE);
            return obj;
        }
    }

    NodePtr parseFuncDecl() {
        expect(TokenType::FUNCTION);
        auto node = makeNode(NodeType::FuncDecl);
        if (check(TokenType::IDENTIFIER)) node->sval = consume().value;
        parseFuncParams(node);
        node->children.push_back(parseBlock());
        return node;
    }

    void parseFuncParams(NodePtr& node) {
        expect(TokenType::LPAREN);
        while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOK)) {
            bool rest = false;
            if (check(TokenType::DOTDOTDOT)) { consume(); rest = true; }
            if (check(TokenType::LBRACKET) || check(TokenType::LBRACE)) {
                // Destructuring param - store as __destruct_X__
                auto pat = parseDestructPattern();
                std::string pname = "__destruct_" + std::to_string(node->params.size()) + "__";
                node->params.push_back(pname);
                // Store pattern in children (before body)
                pat->sval = "__param_pattern__" + pname;
                node->children.push_back(pat);
            } else {
                std::string pname = expect(TokenType::IDENTIFIER).value;
                if (rest) pname = "..." + pname;
                node->params.push_back(pname);
                // Default value
                if (match(TokenType::ASSIGN)) {
                    auto defNode = parseAssignment();
                    defNode->sval = "__default__" + pname;
                    node->children.push_back(defNode);
                }
            }
            if (!match(TokenType::COMMA)) break;
        }
        expect(TokenType::RPAREN);
    }

    NodePtr parseIf() {
        expect(TokenType::IF);
        expect(TokenType::LPAREN);
        auto cond = parseExpr();
        expect(TokenType::RPAREN);
        auto node = makeNode(NodeType::If);
        node->children.push_back(cond);
        node->children.push_back(parseStatement());
        if (match(TokenType::ELSE)) {
            node->children.push_back(parseStatement());
        }
        return node;
    }

    NodePtr parseWhile() {
        expect(TokenType::WHILE);
        expect(TokenType::LPAREN);
        auto cond = parseExpr();
        expect(TokenType::RPAREN);
        auto node = makeNode(NodeType::While);
        node->children.push_back(cond);
        node->children.push_back(parseStatement());
        return node;
    }

    NodePtr parseFor() {
        expect(TokenType::FOR);
        expect(TokenType::LPAREN);

        // Check for for...of or for...in
        // Peek: for (let x of/in ...)
        int savedPos = pos;
        bool isForInOf = false;
        NodeType forKind = NodeType::For;
        
        // Try to detect for..in / for..of
        if (cur().type == TokenType::LET || cur().type == TokenType::CONST || cur().type == TokenType::VAR) {
            int p2 = pos + 1;
            if (p2 < (int)tokens.size() && tokens[p2].type == TokenType::IDENTIFIER) {
                int p3 = p2 + 1;
                if (p3 < (int)tokens.size()) {
                    if (tokens[p3].type == TokenType::OF) forKind = NodeType::ForOf, isForInOf = true;
                    else if (tokens[p3].type == TokenType::IN) forKind = NodeType::ForIn, isForInOf = true;
                }
            }
        }

        if (isForInOf) {
            consume(); // let/const/var
            auto varName = expect(TokenType::IDENTIFIER).value;
            consume(); // in/of
            auto iterNode = parseExpr();
            expect(TokenType::RPAREN);
            auto node = makeNode(forKind);
            node->sval = varName;
            node->children.push_back(iterNode);
            node->children.push_back(parseStatement());
            return node;
        }

        // Regular for loop
        NodePtr init;
        if (!check(TokenType::SEMICOLON)) {
            if (cur().type == TokenType::LET || cur().type == TokenType::CONST || cur().type == TokenType::VAR)
                init = parseVarDecl();
            else {
                auto e = parseExpr(); match(TokenType::SEMICOLON);
                init = makeNode(NodeType::ExprStmt);
                init->children.push_back(e);
            }
        } else { consume(); }

        NodePtr cond;
        if (!check(TokenType::SEMICOLON)) cond = parseExpr();
        expect(TokenType::SEMICOLON);

        NodePtr update;
        if (!check(TokenType::RPAREN)) update = parseExpr();
        expect(TokenType::RPAREN);

        auto node = makeNode(NodeType::For);
        node->children.push_back(init ? init : makeNode(NodeType::Literal));
        node->children.push_back(cond ? cond : makeNode(NodeType::Literal));
        node->children.push_back(update ? update : makeNode(NodeType::Literal));
        node->children.push_back(parseStatement());
        return node;
    }

    NodePtr parseDoWhile() {
        expect(TokenType::DO);
        auto body = parseStatement();
        expect(TokenType::WHILE);
        expect(TokenType::LPAREN);
        auto cond = parseExpr();
        expect(TokenType::RPAREN);
        match(TokenType::SEMICOLON);
        auto node = makeNode(NodeType::DoWhile);
        node->children.push_back(cond);
        node->children.push_back(body);
        return node;
    }

    NodePtr parseReturn() {
        expect(TokenType::RETURN);
        auto node = makeNode(NodeType::Return);
        if (!check(TokenType::SEMICOLON) && !check(TokenType::RBRACE) && !check(TokenType::EOF_TOK)) {
            node->children.push_back(parseExpr());
        }
        match(TokenType::SEMICOLON);
        return node;
    }

    NodePtr parseSwitch() {
        expect(TokenType::SWITCH);
        expect(TokenType::LPAREN);
        auto disc = parseExpr();
        expect(TokenType::RPAREN);
        expect(TokenType::LBRACE);
        auto node = makeNode(NodeType::Switch);
        node->children.push_back(disc);
        while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOK)) {
            auto sc = makeNode(NodeType::SwitchCase);
            if (match(TokenType::CASE)) {
                sc->children.push_back(parseExpr());
                expect(TokenType::COLON);
            } else {
                expect(TokenType::DEFAULT);
                expect(TokenType::COLON);
                sc->sval = "default";
            }
            while (!check(TokenType::CASE) && !check(TokenType::DEFAULT) &&
                   !check(TokenType::RBRACE) && !check(TokenType::EOF_TOK)) {
                skipSemis();
                if (check(TokenType::CASE)||check(TokenType::DEFAULT)||check(TokenType::RBRACE)) break;
                sc->children.push_back(parseStatement());
            }
            node->children.push_back(sc);
        }
        expect(TokenType::RBRACE);
        return node;
    }

    NodePtr parseThrow() {
        expect(TokenType::THROW);
        auto node = makeNode(NodeType::Throw);
        node->children.push_back(parseExpr());
        match(TokenType::SEMICOLON);
        return node;
    }

    NodePtr parseTry() {
        expect(TokenType::TRY);
        auto node = makeNode(NodeType::Try);
        node->children.push_back(parseBlock());
        if (match(TokenType::CATCH)) {
            if (match(TokenType::LPAREN)) {
                node->sval = expect(TokenType::IDENTIFIER).value;
                expect(TokenType::RPAREN);
            }
            node->children.push_back(parseBlock());
        } else {
            node->children.push_back(makeNode(NodeType::Block));
        }
        if (match(TokenType::FINALLY)) {
            node->children.push_back(parseBlock());
        }
        return node;
    }

    NodePtr parseClassDecl() {
        expect(TokenType::CLASS);
        auto node = makeNode(NodeType::ClassDecl);
        if (check(TokenType::IDENTIFIER)) node->sval = consume().value;
        if (match(TokenType::EXTENDS)) {
            node->params.push_back(expect(TokenType::IDENTIFIER).value);
        }
        expect(TokenType::LBRACE);
        while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOK)) {
            skipSemis();
            if (check(TokenType::RBRACE)) break;
            bool isStatic = false;
            if (check(TokenType::IDENTIFIER) && cur().value == "static") {
                isStatic = true; consume();
            }
            std::string name;
            if (check(TokenType::IDENTIFIER)) name = consume().value;
            else if (check(TokenType::STRING)) name = consume().value;
            else if (check(TokenType::LBRACKET)) {
                consume(); parseExpr(); expect(TokenType::RBRACKET);
                name = "__computed__";
            }
            // Constructor or method
            auto method = makeNode(NodeType::FuncDecl);
            method->sval = isStatic ? ("static_" + name) : name;
            parseFuncParams(method);
            method->children.push_back(parseBlock());
            node->children.push_back(method);
        }
        expect(TokenType::RBRACE);
        return node;
    }

    // Expression parsing with precedence climbing
    NodePtr parseExpr() { return parseSequence(); }

    NodePtr parseSequence() {
        auto left = parseAssignment();
        while (check(TokenType::COMMA)) {
            // Don't consume commas that are part of argument lists — context decides
            // But in expression context comma = sequence
            // We'll let callers handle this carefully
            break; // disable sequence for safety; callers handle commas
        }
        return left;
    }

    NodePtr parseAssignment() {
        auto left = parseTernary();
        auto t = cur().type;
        if (t == TokenType::ASSIGN || t == TokenType::PLUS_ASSIGN ||
            t == TokenType::MINUS_ASSIGN || t == TokenType::STAR_ASSIGN ||
            t == TokenType::SLASH_ASSIGN || t == TokenType::PERCENT_ASSIGN) {
            std::string op = consume().value;
            auto right = parseAssignment();
            auto node = makeNode(NodeType::Assignment);
            node->sval = op;
            node->children.push_back(left);
            node->children.push_back(right);
            return node;
        }
        return left;
    }

    NodePtr parseTernary() {
        auto cond = parseNullish();
        if (match(TokenType::QUESTION)) {
            auto then = parseAssignment();
            expect(TokenType::COLON);
            auto els = parseAssignment();
            auto node = makeNode(NodeType::Ternary);
            node->children = {cond, then, els};
            return node;
        }
        return cond;
    }

    NodePtr parseNullish() {
        auto left = parseOr();
        while (check(TokenType::NULLISH)) {
            consume();
            auto right = parseOr();
            auto node = makeNode(NodeType::BinOp);
            node->sval = "??";
            node->children = {left, right};
            left = node;
        }
        return left;
    }

    NodePtr parseOr() {
        auto left = parseAnd();
        while (check(TokenType::OR)) {
            consume();
            auto right = parseAnd();
            auto node = makeNode(NodeType::BinOp);
            node->sval = "||";
            node->children = {left, right};
            left = node;
        }
        return left;
    }

    NodePtr parseAnd() {
        auto left = parseBitOr();
        while (check(TokenType::AND)) {
            consume();
            auto right = parseBitOr();
            auto node = makeNode(NodeType::BinOp);
            node->sval = "&&";
            node->children = {left, right};
            left = node;
        }
        return left;
    }

    NodePtr parseBitOr() {
        auto left = parseBitXor();
        while (check(TokenType::BITOR)) {
            consume();
            auto right = parseBitXor();
            auto node = makeNode(NodeType::BinOp); node->sval = "|";
            node->children = {left, right}; left = node;
        }
        return left;
    }

    NodePtr parseBitXor() {
        auto left = parseBitAnd();
        while (check(TokenType::BITXOR)) {
            consume();
            auto right = parseBitAnd();
            auto node = makeNode(NodeType::BinOp); node->sval = "^";
            node->children = {left, right}; left = node;
        }
        return left;
    }

    NodePtr parseBitAnd() {
        auto left = parseEquality();
        while (check(TokenType::BITAND)) {
            consume();
            auto right = parseEquality();
            auto node = makeNode(NodeType::BinOp); node->sval = "&";
            node->children = {left, right}; left = node;
        }
        return left;
    }

    NodePtr parseEquality() {
        auto left = parseRelational();
        while (true) {
            auto t = cur().type;
            if (t!=TokenType::EQ && t!=TokenType::NEQ && t!=TokenType::STRICT_EQ && t!=TokenType::STRICT_NEQ) break;
            std::string op = consume().value;
            auto right = parseRelational();
            auto node = makeNode(NodeType::BinOp);
            node->sval = op;
            node->children = {left, right};
            left = node;
        }
        return left;
    }

    NodePtr parseRelational() {
        auto left = parseShift();
        while (true) {
            auto t = cur().type;
            if (t!=TokenType::LT && t!=TokenType::GT && t!=TokenType::LTE && t!=TokenType::GTE
                && t!=TokenType::INSTANCEOF && t!=TokenType::IN) break;
            std::string op = consume().value;
            auto right = parseShift();
            auto node = makeNode(NodeType::BinOp);
            node->sval = op;
            node->children = {left, right};
            left = node;
        }
        return left;
    }

    NodePtr parseShift() {
        auto left = parseAddSub();
        while (check(TokenType::LSHIFT)||check(TokenType::RSHIFT)||check(TokenType::URSHIFT)) {
            std::string op = consume().value;
            auto right = parseAddSub();
            auto node = makeNode(NodeType::BinOp); node->sval = op;
            node->children = {left, right}; left = node;
        }
        return left;
    }

    NodePtr parseAddSub() {
        auto left = parseMulDiv();
        while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
            std::string op = consume().value;
            auto right = parseMulDiv();
            auto node = makeNode(NodeType::BinOp);
            node->sval = op;
            node->children = {left, right};
            left = node;
        }
        return left;
    }

    NodePtr parseMulDiv() {
        auto left = parseExponent();
        while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
            std::string op = consume().value;
            auto right = parseExponent();
            auto node = makeNode(NodeType::BinOp);
            node->sval = op;
            node->children = {left, right};
            left = node;
        }
        return left;
    }

    NodePtr parseExponent() {
        auto left = parseUnary();
        if (check(TokenType::STARSTAR)) {
            consume();
            auto right = parseExponent(); // right-associative
            auto node = makeNode(NodeType::BinOp);
            node->sval = "**";
            node->children = {left, right};
            return node;
        }
        return left;
    }

    NodePtr parseUnary() {
        auto t = cur().type;
        if (t == TokenType::NOT) {
            consume();
            auto node = makeNode(NodeType::UnaryOp); node->sval = "!";
            node->children.push_back(parseUnary());
            return node;
        }
        if (t == TokenType::MINUS) {
            consume();
            auto node = makeNode(NodeType::UnaryOp); node->sval = "-";
            node->children.push_back(parseUnary());
            return node;
        }
        if (t == TokenType::PLUS) {
            consume();
            auto node = makeNode(NodeType::UnaryOp); node->sval = "+";
            node->children.push_back(parseUnary());
            return node;
        }
        if (t == TokenType::BITNOT) {
            consume();
            auto node = makeNode(NodeType::UnaryOp); node->sval = "~";
            node->children.push_back(parseUnary());
            return node;
        }
        if (t == TokenType::TYPEOF) {
            consume();
            auto node = makeNode(NodeType::TypeOf);
            node->children.push_back(parseUnary());
            return node;
        }
        if (t == TokenType::INCREMENT) {
            consume();
            auto node = makeNode(NodeType::UnaryOp); node->sval = "++pre";
            node->children.push_back(parseUnary());
            return node;
        }
        if (t == TokenType::DECREMENT) {
            consume();
            auto node = makeNode(NodeType::UnaryOp); node->sval = "--pre";
            node->children.push_back(parseUnary());
            return node;
        }
        return parsePostfix();
    }

    NodePtr parsePostfix() {
        auto expr = parseCallMember();
        if (check(TokenType::INCREMENT)) {
            consume();
            auto node = makeNode(NodeType::PostfixOp); node->sval = "++";
            node->children.push_back(expr);
            return node;
        }
        if (check(TokenType::DECREMENT)) {
            consume();
            auto node = makeNode(NodeType::PostfixOp); node->sval = "--";
            node->children.push_back(expr);
            return node;
        }
        return expr;
    }

    NodePtr parseCallMember() {
        auto expr = parsePrimary();
        while (true) {
            if (check(TokenType::DOT) || check(TokenType::OPTIONAL_CHAIN)) {
                bool optional = cur().type == TokenType::OPTIONAL_CHAIN;
                consume();
                std::string prop;
                if (check(TokenType::IDENTIFIER) || isKeywordAsIdent()) {
                    prop = consume().value;
                } else {
                    prop = expect(TokenType::IDENTIFIER).value;
                }
                auto node = makeNode(NodeType::MemberAccess);
                node->sval = prop;
                node->bval = optional;
                node->children.push_back(expr);
                expr = node;
            } else if (check(TokenType::LBRACKET)) {
                consume();
                auto idx = parseExpr();
                expect(TokenType::RBRACKET);
                auto node = makeNode(NodeType::Index);
                node->children = {expr, idx};
                expr = node;
            } else if (check(TokenType::LPAREN)) {
                auto node = makeNode(NodeType::Call);
                node->children.push_back(expr);
                consume();
                while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOK)) {
                    if (check(TokenType::DOTDOTDOT)) {
                        consume();
                        auto spread = makeNode(NodeType::SpreadElement);
                        spread->children.push_back(parseAssignment());
                        node->children.push_back(spread);
                    } else {
                        node->children.push_back(parseAssignment());
                    }
                    if (!match(TokenType::COMMA)) break;
                }
                expect(TokenType::RPAREN);
                expr = node;
            } else break;
        }
        return expr;
    }

    bool isKeywordAsIdent() {
        auto t = cur().type;
        return t == TokenType::IN || t == TokenType::OF || t == TokenType::NEW ||
               t == TokenType::CLASS || t == TokenType::LET || t == TokenType::CONST ||
               t == TokenType::VAR;
    }

    // Hacky token for "from" and "delete" used as identifiers
    enum { FROM = 9999, DELETE = 9998 };

    NodePtr parsePrimary() {
        auto t = cur().type;
        int ln = cur().line;

        if (t == TokenType::NUMBER) {
            std::string v = consume().value;
            auto node = makeNode(NodeType::Literal);
            node->sval = "__number__";
            if (v.size() > 2 && v[0]=='0' && (v[1]=='x'||v[1]=='X'))
                node->nval = (double)std::stoll(v, nullptr, 16);
            else
                node->nval = std::stod(v);
            return node;
        }
        if (t == TokenType::STRING) {
            auto node = makeNode(NodeType::Literal);
            node->sval = consume().value;
            node->bval = true; // marks as string
            return node;
        }
        if (t == TokenType::BOOLEAN) {
            std::string v = consume().value;
            auto node = makeNode(NodeType::Literal);
            node->sval = "__bool__";
            node->bval = (v == "true");
            return node;
        }
        if (t == TokenType::NULL_TOK) {
            consume();
            auto node = makeNode(NodeType::Literal);
            node->sval = "__null__";
            return node;
        }
        if (t == TokenType::UNDEFINED) {
            consume();
            auto node = makeNode(NodeType::Literal);
            node->sval = "__undefined__";
            return node;
        }
        if (t == TokenType::IDENTIFIER) {
            // Check for arrow function: ident => ...
            if (peek().type == TokenType::ARROW) {
                std::string param = consume().value;
                consume(); // =>
                auto node = makeNode(NodeType::ArrowFunc);
                node->params.push_back(param);
                if (check(TokenType::LBRACE)) node->children.push_back(parseBlock());
                else {
                    auto ret = makeNode(NodeType::Return);
                    ret->children.push_back(parseAssignment());
                    node->children.push_back(ret);
                }
                return node;
            }
            auto node = makeNode(NodeType::Identifier);
            node->sval = consume().value;
            return node;
        }
        if (t == TokenType::FUNCTION) {
            consume();
            auto node = makeNode(NodeType::FuncExpr);
            if (check(TokenType::IDENTIFIER)) node->sval = consume().value;
            parseFuncParams(node);
            node->children.push_back(parseBlock());
            return node;
        }
        if (t == TokenType::NEW) {
            consume();
            auto callee = parseCallMember();
            auto node = makeNode(NodeType::NewExpr);
            node->children.push_back(callee);
            if (check(TokenType::LPAREN)) {
                consume();
                while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOK)) {
                    if (check(TokenType::DOTDOTDOT)) { consume(); auto s = makeNode(NodeType::SpreadElement); s->children.push_back(parseAssignment()); node->children.push_back(s); }
                    else node->children.push_back(parseAssignment());
                    if (!match(TokenType::COMMA)) break;
                }
                expect(TokenType::RPAREN);
            }
            return node;
        }
        if (t == TokenType::LPAREN) {
            consume();
            // Arrow function with multiple params: (a, b) => ...
            // Check for arrow func
            int savedPos = pos;
            bool isArrow = false;
            std::vector<std::string> arrowParams;
            // Try to parse params list then =>
            try {
                std::vector<std::string> tryParams;
                bool ok = true;
                int tmpPos = pos;
                while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOK)) {
                    bool rest = false;
                    if (check(TokenType::DOTDOTDOT)) { consume(); rest = true; }
                    if (!check(TokenType::IDENTIFIER) && !check(TokenType::LBRACKET) && !check(TokenType::LBRACE)) { ok = false; break; }
                    std::string pname;
                    if (check(TokenType::IDENTIFIER)) pname = consume().value;
                    else { parseDestructPattern(); pname = "__destruct__"; }
                    if (rest) pname = "..." + pname;
                    tryParams.push_back(pname);
                    // Default value
                    if (match(TokenType::ASSIGN)) parseAssignment();
                    if (!match(TokenType::COMMA)) break;
                }
                if (ok && check(TokenType::RPAREN)) {
                    consume();
                    if (check(TokenType::ARROW)) {
                        consume();
                        isArrow = true;
                        arrowParams = tryParams;
                    } else {
                        // Not arrow, reset
                        pos = savedPos;
                    }
                } else {
                    pos = savedPos;
                }
            } catch(...) {
                pos = savedPos;
            }

            if (isArrow) {
                auto node = makeNode(NodeType::ArrowFunc);
                node->params = arrowParams;
                if (check(TokenType::LBRACE)) node->children.push_back(parseBlock());
                else {
                    auto ret = makeNode(NodeType::Return);
                    ret->children.push_back(parseAssignment());
                    node->children.push_back(ret);
                }
                return node;
            }

            // Regular parens
            auto expr = parseExpr();
            // Handle comma as sequence inside parens
            while (match(TokenType::COMMA)) {
                auto right = parseExpr();
                auto seq = makeNode(NodeType::Sequence);
                seq->children = {expr, right};
                expr = seq;
            }
            expect(TokenType::RPAREN);
            return expr;
        }
        if (t == TokenType::LBRACKET) {
            consume();
            auto node = makeNode(NodeType::ArrayLiteral);
            while (!check(TokenType::RBRACKET) && !check(TokenType::EOF_TOK)) {
                if (check(TokenType::COMMA)) {
                    consume();
                    auto hole = makeNode(NodeType::Literal); hole->sval = "__undefined__";
                    node->children.push_back(hole);
                    continue;
                }
                if (check(TokenType::DOTDOTDOT)) {
                    consume();
                    auto spread = makeNode(NodeType::SpreadElement);
                    spread->children.push_back(parseAssignment());
                    node->children.push_back(spread);
                } else {
                    node->children.push_back(parseAssignment());
                }
                if (!match(TokenType::COMMA)) break;
            }
            expect(TokenType::RBRACKET);
            return node;
        }
        if (t == TokenType::LBRACE) {
            consume();
            auto node = makeNode(NodeType::ObjectLiteral);
            while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOK)) {
                if (check(TokenType::DOTDOTDOT)) {
                    consume();
                    auto spread = makeNode(NodeType::SpreadElement);
                    spread->children.push_back(parseAssignment());
                    node->children.push_back(spread);
                    match(TokenType::COMMA); continue;
                }
                auto prop = makeNode(NodeType::ObjectProp);
                // Computed property [expr]: val
                if (check(TokenType::LBRACKET)) {
                    consume();
                    prop->children.push_back(parseAssignment());
                    expect(TokenType::RBRACKET);
                    prop->computed = true;
                    expect(TokenType::COLON);
                    prop->children.push_back(parseAssignment());
                } else {
                    // Key: string, identifier, or number
                    std::string key;
                    if (check(TokenType::IDENTIFIER) || isKeywordAsIdent() || 
                        cur().type==TokenType::FUNCTION || cur().type==TokenType::IF ||
                        cur().type==TokenType::LET || cur().type==TokenType::CONST) {
                        key = consume().value;
                    } else if (check(TokenType::STRING)) {
                        key = consume().value;
                    } else if (check(TokenType::NUMBER)) {
                        key = consume().value;
                    } else {
                        key = consume().value;
                    }
                    prop->sval = key;

                    // Method shorthand: { foo() {...} }
                    if (check(TokenType::LPAREN)) {
                        auto fn = makeNode(NodeType::FuncExpr);
                        fn->sval = key;
                        parseFuncParams(fn);
                        fn->children.push_back(parseBlock());
                        prop->children.push_back(fn);
                    } else if (match(TokenType::COLON)) {
                        prop->children.push_back(parseAssignment());
                    } else {
                        // Shorthand { x } => { x: x }
                        auto id = makeNode(NodeType::Identifier);
                        id->sval = key;
                        prop->children.push_back(id);
                    }
                }
                node->children.push_back(prop);
                match(TokenType::COMMA);
            }
            expect(TokenType::RBRACE);
            return node;
        }

        // If nothing matched, skip and return undefined
        auto node = makeNode(NodeType::Literal);
        node->sval = "__undefined__";
        if (!check(TokenType::EOF_TOK)) consume(); // skip bad token
        return node;
    }
};

#endif // PARSER_H