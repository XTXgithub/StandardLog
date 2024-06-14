#ifndef XIO_CLIENT_H
#define XIO_CLIENT_H

#include <QObject>
#include <QTcpSocket>

class XIOClient : public QObject {
    Q_OBJECT
public:
    explicit XIOClient(QObject *parent = nullptr);
    ~XIOClient();

    void connectToServer(const QString &host, quint16 port);
    void sendData(const QString &data);
    void stop();
    bool isConnected;

private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket *socket;
};

#endif // XIO_CLIENT_H


//#ifndef XIO_CLIENT_H
//#define XIO_CLIENT_H

//#include <QObject>
//#include <QTcpSocket>

//class XIOClient : public QObject {
//    Q_OBJECT
//public:
//    explicit XIOClient(QObject *parent = nullptr);
//    void connectToServer(const QString &host, quint16 port);
//    void sendData(const QString &data);

//private slots:
//    void onConnected();
//    void onDisconnected();
//    void onReadyRead();
//    void onErrorOccurred(QAbstractSocket::SocketError socketError);

//private:
//    QTcpSocket *socket;
//};

//#endif // XIO_CLIENT_H
