#include "AST.hpp"
#include "../parser/ParserOutput.hpp"
#include "../lexer/TokenType.hpp"
#include <functional>

using SPNode = std::shared_ptr<ASTNode>;

// 修改说明：原先采用先序（节点先于子节点）输出，导致例如
// <VarDecl>
// INTTK int
// IDENFR i
// 的顺序。为使输出更接近常见的 parser.txt 风格（先列出 token/子节点，
// 再列出对应的结构标签），这里对非 token 节点采用后序输出：
// 先遍历并输出所有子节点，最后再输出节点自身的标签。

static void emitNodeToken(const SPNode &n) {
    if (!n) return;
    if (n->token) {
        // 叶子 token 节点直接输出：类型 + 空格 + 值
        ParserOutput::Get().Write(TokenTypeToString(n->token->type) + std::string(" ") + n->token->value);
    }
}

void TraversePreOrder(const SPNode &root) {
    if (!root) return;
    // 辅助函数：获取子树中最右边的 token 叶子（后序最后出现的 token）
    std::function<SPNode(const SPNode&)> getLastTokenNode = [&](const SPNode &n) -> SPNode {
        if (!n) return nullptr;
        if (n->token) return n;
        for (auto it = n->children.rbegin(); it != n->children.rend(); ++it) {
            SPNode res = getLastTokenNode(*it);
            if (res) return res;
        }
        return nullptr;
    };

    // 辅助遍历：在遍历时跳过特定节点（skip）；跳过时不输出该节点
    std::function<void(const SPNode&, const SPNode&)> traverseWithSkip = [&](const SPNode &n, const SPNode &skip) {
        if (!n) return;
        if (n == skip) return; // 整个子树跳过
        if (n->token) {
            emitNodeToken(n);
            return;
        }
        // 遍历当前节点的直接子节点，但在跳过指定的 skip 节点时仍然保证非 token 子节点完整遍历（以输出它们的结构标签）
        for (auto &c : n->children) {
            if (c == skip) continue;
            if (c->token) emitNodeToken(c);
            else TraversePreOrder(c);
        }
    };

    // 如果当前节点是非 token（结构节点），且其子树最后的 token 是分号，则特殊处理：
    // 先输出该节点子树中除尾分号外的所有内容，再输出结构标签，最后输出分号。
    if (!root->token && !root->children.empty()) {
        SPNode lastTok = getLastTokenNode(root);
        if (lastTok && lastTok->token && lastTok->token->type == TokenType::SEMICN) {
            // 直接对每个子节点递归 TraversePreOrder（但跳过最后的分号节点），
            // 保证非 token 子节点的结构标签被完整输出（例如 VarDef）
            for (auto &c : root->children) {
                if (c == lastTok) continue;
                TraversePreOrder(c);
            }
            // 按评测系统（参考答案）期望的顺序：先输出尾部分号，再输出结构标签
            emitNodeToken(lastTok);
            ParserOutput::Get().Write("<" + root->name + ">");
            return;
        }
    }

    // NOTE: single-chain optimization removed to ensure deterministic
    // post-order output that matches the reference parser.txt formatting.
    // The traversal now always falls through to the default post-order
    // emission below (children first, then the node label).

    // 默认的后序输出：先输出所有子节点，再输出结构标签；token 叶子直接输出
    for (auto &c : root->children) TraversePreOrder(c);
    if (!root->token) {
        ParserOutput::Get().Write("<" + root->name + ">");
    } else {
        emitNodeToken(root);
    }
}
