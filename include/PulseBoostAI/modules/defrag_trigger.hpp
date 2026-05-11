#pragma once

namespace pulseboost {

/// Triggers Windows storage optimization for the system drive.
class DefragTrigger {
public:
    bool optimizeSystemDrive() const;
};

}  // namespace pulseboost
