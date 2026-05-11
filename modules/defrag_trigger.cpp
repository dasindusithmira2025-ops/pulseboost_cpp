#include "PulseBoostAI/modules/defrag_trigger.hpp"

#include <Windows.h>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

bool DefragTrigger::optimizeSystemDrive() const {
    DWORD exitCode = 1;
    // /O performs the right optimization for HDD/SSD/NVMe.
    return runProcessHidden(L"defrag.exe C: /O", &exitCode) && exitCode == 0;
}

}  // namespace pulseboost

