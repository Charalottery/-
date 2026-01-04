#pragma once

#include <string>

class IrModule;

namespace optimize
{

    class Pass
    {
    public:
        virtual ~Pass() = default;
        virtual std::string name() const = 0;
        virtual void run(IrModule *module) = 0;
    };

} // namespace optimize
