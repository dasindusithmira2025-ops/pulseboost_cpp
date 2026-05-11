#pragma once

#include <QString>
#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

class ContextBuilder {
public:
    /// Constructs a highly detailed system instruction prompt describing PulseBoost AI's available rules.
    static QString buildSystemInstruction();

    /// Constructs a formatted string describing the current system state, for injection into prompts.
    static QString buildTelemetryContext(const SystemSnapshot &snapshot);
    
    /// Constructs a complete prompt package combining the user prompt and the live telemetry.
    static QString buildUserPrompt(const QString &userQuery, const SystemSnapshot &snapshot);
};

} // namespace pulseboost
