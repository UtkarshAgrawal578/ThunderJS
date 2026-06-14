#ifndef AST_H
#define AST_H
#include <string>
#include <vector>
#include <memory>

struct ASTNode;
using NodePtr = std::shared_ptr<ASTNode>;
using NodeList = std::vector<NodePtr>;

enum class NodeType {
    // Statements
    Program, Block, VarDecl, FuncDecl, Return, If, While, For, ForIn, ForOf,
    DoWhile, Switch, SwitchCase, Break, Continue, Throw, Try, ExprStmt,
    ClassDecl,
    // Expressions
    Literal, Identifier, BinOp, UnaryOp, PostfixOp, Assignment, Call,
    MemberAccess, Index, Ternary, Sequence,
    ArrayLiteral, ObjectLiteral, ObjectProp, SpreadElement,
    ArrowFunc, FuncExpr, NewExpr, TypeOf, Typeof,
    TemplateLiteral,
};

struct ASTNode {
    NodeType type;
    std::string sval;          // string/identifier/op
    double nval = 0;           // number
    bool bval = false;         // boolean
    NodeList children;         // sub-nodes
    std::vector<std::string> params; // function params
    bool is_rest = false;      // rest param
    bool computed = false;     // computed member access
    bool is_const = false;

    ASTNode(NodeType t): type(t) {}
};

inline NodePtr makeNode(NodeType t) { return std::make_shared<ASTNode>(t); }

#endif // AST_H