#pragma once

#include <QString>
#include <QList>

namespace pulseboost {

struct ChatMessage {
    QString role; // "user", "model", "system"
    QString content;
};

class SessionMemory {
public:
    explicit SessionMemory(int maxMessages = 20);

    /// Append a new message to the rolling conversation history.
    void addMessage(const QString &role, const QString &content);
    
    /// Clear the conversation history.
    void clear();
    
    /// Retrieve the current message history.
    QList<ChatMessage> getHistory() const;

    /// Get history formatted as a single string (useful for basic LLMs or context dumps).
    QString getFormattedHistory() const;

private:
    QList<ChatMessage> messages_;
    int maxMessages_;
};

} // namespace pulseboost
