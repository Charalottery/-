#pragma once

#include "Pass.hpp"

namespace optimize
{

    class Mem2RegPass final : public Pass
    {
    public:
        std::string name() const override { return "mem2reg"; }
        void run(IrModule *module) override;
    };

} // namespace optimize
