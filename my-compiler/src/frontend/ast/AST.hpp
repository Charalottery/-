#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../lexer/Token.hpp"

struct ASTNode {
    std::string name; // e.g., "VarDecl" or "IDENFR" for token nodes
    std::shared_ptr<Token> token; // non-null for token leaves
    std::vector<std::shared_ptr<ASTNode>> children;

    ASTNode(const std::string &n) : name(n), token(nullptr) {}
    ASTNode(const Token &t) : name("TOKEN"), token(std::make_shared<Token>(t)) {}

    static std::shared_ptr<ASTNode> Make(const std::string &n) { return std::make_shared<ASTNode>(n); }
    static std::shared_ptr<ASTNode> MakeToken(const Token &t) { return std::make_shared<ASTNode>(t); }
};

// traversal function
void TraversePreOrder(const std::shared_ptr<ASTNode> &root);
