#include "CompUnit.hpp"
#include "../parser/Parser.hpp"

// CompUnit::Parse becomes a thin wrapper that delegates to the current Parser
// instance. The Parser implementation provides all recursive-descent member
// functions and produces/writes the AST output.
void CompUnit::Parse() {
    Parser *p = Parser::GetCurrent();
    if (p) {
        p->ParseCompUnit();
    } else {
        // fallback: nothing to do if no parser context
    }
}
