#include "PulseBoostAI/ui/chatbot_window.hpp"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

namespace pulseboost {

ChatbotWindow::ChatbotWindow(ChatbotInterface &chatbot, QWidget *parent) : QWidget(parent), chatbot_(chatbot) {
    auto *layout = new QVBoxLayout(this);
    transcript_ = new QTextEdit(this);
    transcript_->setReadOnly(true);
    transcript_->setPlaceholderText("Ask PulseBoost AI about performance, storage, startup, or cleanup.");

    auto *composerLayout = new QHBoxLayout();
    input_ = new QLineEdit(this);
    input_->setPlaceholderText("Example: Why is my PC slow?");
    sendButton_ = new QPushButton("Send", this);

    composerLayout->addWidget(input_, 1);
    composerLayout->addWidget(sendButton_);

    layout->addWidget(transcript_, 1);
    layout->addLayout(composerLayout);

    connect(sendButton_, &QPushButton::clicked, this, &ChatbotWindow::submitMessage);
    connect(input_, &QLineEdit::returnPressed, this, &ChatbotWindow::submitMessage);
}

void ChatbotWindow::sendPresetCommand(const QString &command) {
    input_->setText(command);
    submitMessage();
}

void ChatbotWindow::submitMessage() {
    const QString text = input_->text().trimmed();
    if (text.isEmpty()) {
        return;
    }

    transcript_->append("> " + text);
    const auto response = chatbot_.handleMessage(text.toStdString());
    transcript_->append(QString::fromStdString(response.message));
    transcript_->append(QString());
    input_->clear();
}

}  // namespace pulseboost
