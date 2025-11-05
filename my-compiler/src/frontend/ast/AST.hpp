#pragma once
#include "../lexer/Token.hpp"
#include "../lexer/TokenPrinter.hpp"
#include <string>
#include <vector>
#include <ostream>
#include <memory>

// Minimal AST node representation used by the Parser.
struct ASTNode {
    std::string name; // nonterminal name (like "CompUnit" or token type marker "TOKEN")
    std::vector<std::unique_ptr<ASTNode>> children;
    // if leaf
    bool isToken = false;
    Token token;

    explicit ASTNode(std::string n) : name(std::move(n)), isToken(false), token() {}
    ASTNode(const Token &t) : name("TOKEN"), isToken(true), token(t) {}

    void AddChild(std::unique_ptr<ASTNode> c) { children.push_back(std::move(c)); }

    // post-order traversal: print leaves (tokens) in lexical order, then for non-excluded
    // nonterminals print the node name in angle brackets.
    void PostOrderPrint(std::ostream &os) const {
        for (const auto &c : children) c->PostOrderPrint(os);
        if (isToken) {
            os << TokenTypeToString(token.type) << " " << token.value << "\n";
        } else {
            // do not print these node names
            if (name == "BlockItem" || name == "Decl" || name == "BType") return;
            os << "<" << name << ">" << "\n";
        }
    }
};
