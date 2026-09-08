#pragma once

#include "ir/design.hpp"

namespace minipd {

class IRouter {
public:
    virtual ~IRouter() = default;
    virtual void route(Design& design) = 0;
    virtual const char* name() const = 0;
};

}  // namespace minipd
