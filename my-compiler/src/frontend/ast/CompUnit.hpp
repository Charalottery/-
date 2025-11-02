#pragma once

#include "Node.hpp"

class CompUnit : public Node {
public:
    CompUnit() = default;
    void Parse() override;
};
