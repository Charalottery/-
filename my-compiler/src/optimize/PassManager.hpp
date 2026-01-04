#pragma once

#include "Pass.hpp"
#include <memory>
#include <vector>

class IrModule;

namespace optimize
{

    class PassManager
    {
    public:
        void addPass(std::unique_ptr<Pass> pass) { passes.emplace_back(std::move(pass)); }

        void run(IrModule *module)
        {
            for (auto &pass : passes)
            {
                pass->run(module);
            }
        }

    private:
        std::vector<std::unique_ptr<Pass>> passes;
    };

} // namespace optimize
