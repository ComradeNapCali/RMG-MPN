#include "waitroom.h"
#include <QGridLayout>
#include <QMessageBox>
#include <QJsonArray>

WaitRoom::WaitRoom(QString filename, QJsonObject room, QWebSocket *socket, QWidget *parent)
    : QDialog(parent)
{


    setWindowTitle("RMG NetPlay");
    w = (UserInterface::MainWindow*)parent;

    this->resize(640,480);

    player_name = room.value("player_name").toString();
    room_port = room.value("port").toInt();
    room_name = room.value("room_name").toString();
    file_name = filename;
    started = 0;

    webSocket = socket;
    connect(webSocket, &QWebSocket::textMessageReceived,
            this, &WaitRoom::processTextMessage);

    connect(webSocket, &QWebSocket::pong, this, &WaitRoom::updatePing);

    QGridLayout *layout = new QGridLayout(this);

    QLabel *gameLabel = new QLabel("Game Name:", this);
    layout->addWidget(gameLabel, 0, 0);
    QLabel *gameName = new QLabel(room.value("game_name").toString(), this);
    layout->addWidget(gameName, 0, 1);

    QLabel *pingLabel = new QLabel("Your Ping:", this);
    layout->addWidget(pingLabel, 1, 0);
    pingValue = new QLabel(this);
    layout->addWidget(pingValue, 1, 1);

    QLabel *p1Label = new QLabel("Player 1:", this);
    layout->addWidget(p1Label, 3, 0);

    QLabel *p2Label = new QLabel("Player 2:", this);
    layout->addWidget(p2Label, 4, 0);

    QLabel *p3Label = new QLabel("Player 3:", this);
    layout->addWidget(p3Label, 5, 0);

    QLabel *p4Label = new QLabel("Player 4:", this);
    layout->addWidget(p4Label, 6, 0);

    for (int i = 0; i < 4; ++i)
    {
        pName[i] = new QLabel(this);
        layout->addWidget(pName[i], i+3, 1);
    }

    chatWindow = new QPlainTextEdit(this);
    chatWindow->setReadOnly(1);
    layout->addWidget(chatWindow, 7, 0, 3, 2);

    chatEdit = new QLineEdit(this);
    chatEdit->setPlaceholderText("Enter chat message here");
    connect(chatEdit, &QLineEdit::returnPressed, this, &WaitRoom::sendChat);
    layout->addWidget(chatEdit, 10, 0, 1, 2);

    startGameButton = new QPushButton(this);
    startGameButton->setText("Start Game");
    startGameButton->setAutoDefault(0);
    connect(startGameButton, &QPushButton::released, this, &WaitRoom::startGame);
    layout->addWidget(startGameButton, 11, 0, 1, 2);

    motd = new QLabel(this);
    motd->setTextFormat(Qt::RichText);
    motd->setTextInteractionFlags(Qt::TextBrowserInteraction);
    motd->setOpenExternalLinks(true);
    motd->setAlignment(Qt::AlignCenter);
    layout->addWidget(motd, 12, 0, 1, 2);

    setLayout(layout);

    connect(this, SIGNAL (finished(int)), this, SLOT (onFinished(int)));

    QJsonObject json;
    json.insert("type", "request_players");
    json.insert("port", room_port);
    QJsonDocument json_doc = QJsonDocument(json);
    webSocket->sendTextMessage(json_doc.toJson());

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &WaitRoom::sendPing);
    timer->start(5000);
}

void WaitRoom::sendPing()
{
    if (motd->text().isEmpty())
    {
        QJsonObject json;
        json.insert("type", "request_motd");
        json.insert("room_name", room_name);
        QJsonDocument json_doc = QJsonDocument(json);
        webSocket->sendTextMessage(json_doc.toJson());
    }
    webSocket->ping();
}

void WaitRoom::updatePing(quint64 elapsedTime, const QByteArray&)
{
    pingValue->setText(QString::number(elapsedTime) + " ms");
}

void WaitRoom::startGame()
{
    if (player_name == pName[0]->text())
    {
        startGameButton->setEnabled(false);
        QJsonObject json;
        json.insert("type", "request_begin_game");
        json.insert("port", room_port);
        QJsonDocument json_doc = QJsonDocument(json);
        webSocket->sendTextMessage(json_doc.toJson());
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText("Only Player 1 can start the game");
        msgBox.exec();
    }
}

void WaitRoom::sendChat()
{
    if (!chatEdit->text().isEmpty())
    {
        QJsonObject json;
        json.insert("type", "request_chat_message");
        json.insert("port", room_port);
        json.insert("player_name", player_name);
        json.insert("message", chatEdit->text());
        QJsonDocument json_doc = QJsonDocument(json);
        webSocket->sendTextMessage(json_doc.toJson());
        chatEdit->clear();
    }
}

void WaitRoom::onFinished(int)
{
    timer->stop();
    webSocket->close();
    webSocket->deleteLater();
}

void WaitRoom::processTextMessage(QString message)
{
    QJsonDocument json_doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject json = json_doc.object();
    if (json.value("type").toString() == "reply_players")
    {
        if (json.contains("player_names"))
        {
            for (int i = 0; i < 4; ++i)
            {
                pName[i]->setText(json.value("player_names").toArray().at(i).toString());
                if (pName[i]->text() == player_name)
                    player_number = i + 1;
            }
        }
    }
    else if (json.value("type").toString() == "reply_chat_message")
    {
        chatWindow->appendPlainText(json.value("message").toString());
    }
    else if (json.value("type").toString() == "reply_begin_game")
    {
        started = 1;
        w->OpenROMNetplay(file_name, webSocket->peerAddress().toString(), room_port, player_number);
        accept();
    }
    else if (json.value("type").toString() == "reply_motd")
    {
        QString message = json.value("message").toString();
        motd->setText(message);
    }
}
