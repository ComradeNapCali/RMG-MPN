#ifndef WAITROOM_H
#define WAITROOM_H

#include <QDialog>
#include <QJsonObject>
#include <QJsonDocument>
#include <QWebSocket>
#include <QLabel>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QCheckBox>

#include <UserInterface/MainWindow.hpp>

class WaitRoom : public QDialog
{
    Q_OBJECT
public:
    WaitRoom(QString filename, QJsonObject room, QWebSocket *socket, QWidget *parent = nullptr);
private slots:
    void processTextMessage(QString message);
    void onFinished(int);
    void sendChat();
    void startGame();
    void updatePing(quint64 elapsedTime, const QByteArray &payload);
    void sendPing();
private:
    QWebSocket *webSocket = nullptr;
    QLabel *pName[4];
    QPlainTextEdit *chatWindow;
    QLineEdit *chatEdit;
    QString player_name;
    int player_number;
    QString file_name;
    int room_port;
    QString room_name;
    QPushButton *startGameButton;
    QLabel *pingValue;
    QLabel *motd;
    QTimer *timer;
    int started;
    UserInterface::MainWindow* w;
};

#endif // WAITROOM_H
