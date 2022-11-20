#pragma once


#include "veldrid/common/Macros.h"
#include "veldrid/DeviceResource.hpp"

#include <cstdint>
#include <limits>

namespace Veldrid{

    class Fence : public DeviceResource{

    protected:

        Fence(sp<GraphicsDevice>&& dev) 
            : DeviceResource(std::move(dev)) {}

    public:
        virtual ~Fence() = default;

        virtual bool WaitForSignal(std::uint64_t timeoutNs) const = 0;
        bool WaitForSignal() const {
            return WaitForSignal(std::numeric_limits<std::uint64_t>::max());
        }
        virtual bool IsSignaled() const = 0;

        virtual void Reset() = 0;

    };

}
