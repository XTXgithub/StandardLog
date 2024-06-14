#include "xio_client.h"
#include <QDebug>

XIOClient::XIOClient(QObject *parent) : QObject(parent), socket(new QTcpSocket(this)) {
    connect(socket, &QTcpSocket::connected, this, &XIOClient::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &XIOClient::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &XIOClient::onDisconnected);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &XIOClient::onError);
}

XIOClient::~XIOClient() {
    stop();
}

void XIOClient::connectToServer(const QString &host, quint16 port) {
    if (socket->state() == QAbstractSocket::UnconnectedState) {
        socket->connectToHost(host, port);
    } else {
        qDebug() << "XIOClient: Already connected or connecting.";
    }
}

void XIOClient::sendData(const QString &data) {
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(data.toUtf8());
    } else {
        qDebug() << "XIOClient: Not connected to server.";
    }
}

void XIOClient::stop() {
    if (socket->state() == QAbstractSocket::ConnectedState || socket->state() == QAbstractSocket::ConnectingState) {
        socket->disconnectFromHost();
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            socket->waitForDisconnected(3000);
        }
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            qDebug() << "XIOClient: Socket disconnection timed out, aborting.";
            socket->abort();
        } else {
            qDebug() << "XIOClient: Socket disconnected successfully.";
        }
    }
}

void XIOClient::onConnected() {
    isConnected=true;
    qDebug() << "XIOClient: Connected to server.";
}

void XIOClient::onReadyRead() {
    QByteArray data = socket->readAll();
    qDebug() << "XIOClient: Data received:" << data;
}

void XIOClient::onDisconnected() {
    isConnected=false;
    qDebug() << "XIOClient: Disconnected from server.";
}

void XIOClient::onError(QAbstractSocket::SocketError socketError) {
    qDebug() << "XIOClient: Socket error occurred:" << socketError;
}


//#include "xio_client.h"
//#include <QDebug>

//XIOClient::XIOClient(QObject *parent) : QObject(parent), socket(new QTcpSocket(this)) {
//    connect(socket, &QTcpSocket::connected, this, &XIOClient::onConnected);
//    connect(socket, &QTcpSocket::disconnected, this, &XIOClient::onDisconnected);
//    connect(socket, &QTcpSocket::readyRead, this, &XIOClient::onReadyRead);
//    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
//            this, &XIOClient::onErrorOccurred);
//}

//void XIOClient::connectToServer(const QString &host, quint16 port) {
//    qDebug() << "Connecting to server...";
//    socket->connectToHost(host, port);
//}

//void XIOClient::sendData(const QString &data) {
//    if (socket->state() == QAbstractSocket::ConnectedState) {
//        socket->write(data.toLatin1());
//    } else {
//        qDebug() << "Not connected to server!";
//    }
//}

//void XIOClient::onConnected() {
//    qDebug() << "Connected to server!";
//}

//void XIOClient::onDisconnected() {
//    qDebug() << "Disconnected from server!";
//}

//void XIOClient::onReadyRead() {
//    QByteArray data = socket->readAll();
//    qDebug() << "Data received from server:" << data;
//}

//void XIOClient::onErrorOccurred(QAbstractSocket::SocketError socketError) {
//    qDebug() << "Socket error occurred:" << socketError << socket->errorString();
//}
