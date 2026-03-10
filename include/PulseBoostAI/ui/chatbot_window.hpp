#pragma once

#include <QWidget>

#include "PulseBoostAI/ai/chatbot_interface.hpp"

class QLineEdit;
class QPushButton;
class QTextEdit;

namespace pulseboost {

class ChatbotWindow : public QWidget {
public:
    explicit ChatbotWindow(ChatbotInterface &chatbot, QWidget *parent = nullptr);
    void sendPresetCommand(const QString &command);

private:
    void submitMessage();

    ChatbotInterface &chatbot_;
    QTextEdit *transcript_ = nullptr;
    QLineEdit *input_ = nullptr;
    QPushButton *sendButton_ = nullptr;
};

}  // namespace pulseboost
