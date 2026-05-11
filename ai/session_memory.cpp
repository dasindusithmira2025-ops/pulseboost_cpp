#include "PulseBoostAI/ai/session_memory.hpp"

namespace pulseboost {

SessionMemory::SessionMemory(int maxMessages)
    : maxMessages_(maxMessages)
{
}

void SessionMemory::addMessage(const QString &role, const QString &content) {
    messages_.append({role, content});
    while (messages_.size() > maxMessages_) {
        // Keep history bounded, removing oldest message
        messages_.removeFirst();
    }
}

void SessionMemory::clear() {
    messages_.clear();
}

QList<ChatMessage> SessionMemory::getHistory() const {
    return messages_;
}

QString SessionMemory::getFormattedHistory() const {
    QString history;
    for (const auto &msg : messages_) {
        history += QString("[%1]: %2\n").arg(msg.role.toUpper(), msg.content);
    }
    return history;
}

} // namespace pulseboost
