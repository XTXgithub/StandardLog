#include "Server.h"
#include <QDataStream>
#include <QTimer>

Frame::Frame(quint8 seq, const QByteArray& data)
    : STX(0xFFFF), VER(0x00), SEQ(seq), LEN(static_cast<quint16>(data.size())), DATA(data), CRC(0) {}

QByteArray FrameOperations::encode(Frame& f) {
    QByteArray buf;
    QDataStream stream(&buf, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << f.STX << f.VER << f.SEQ << f.LEN;
    buf.append(f.DATA);
    f.CRC = calcCRC16(0xFFFF, buf.mid(2, 6 + f.DATA.size()));
    stream.device()->seek(8 + f.DATA.size());
    stream << f.CRC;
    return buf;
}

Frame FrameOperations::decode(const QByteArray& buf) {
    if (buf.size() < 10) throw std::runtime_error("buffer too short");

    Frame frame;
    QDataStream stream(buf);
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> frame.STX >> frame.VER >> frame.SEQ >> frame.LEN;
    frame.DATA = buf.mid(8, static_cast<qint32>(frame.LEN));
    stream.device()->seek(8 + frame.LEN);
    stream >> frame.CRC;

    return frame;
}

quint16 FrameOperations::calcCRC16(quint16 initCRC, const QByteArray& crcData) {
    quint16 crc = initCRC;
    for (int cnt = 0; cnt < crcData.size(); ++cnt) {
        crc ^= static_cast<quint8>(crcData[cnt]);
        for (int k = 0; k < 8; ++k) {
            if (crc & 0x1) {
                crc = (crc >> 1) ^ 0x8408;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc & 0xFFFF;
}

Server::Server() : listener(nullptr), stop(false), switchState(false), clientSocket(nullptr), isClientConnected(false),isHeartbeatOpened(true) {
    lastedFrameCmd="";
}

Server::~Server() {
    stopServer();
}

void Server::startServer(const QString& address, quint16 port) {
    if (listener) {
        qWarning() << "Server is already running.";
        return;
    }

    logFile.setFileName("server.log");
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning() << "Failed to open log file.";
        return;
    }

    listener = new QTcpServer(this);
    if (!listener->listen(QHostAddress(address), port)) {
        qWarning() << "Failed to start server.";
        return;
    }

    connect(listener, &QTcpServer::newConnection, this, [=] {
        if (isClientConnected) {
            QTcpSocket* newConn = listener->nextPendingConnection();
            newConn->disconnectFromHost();
            qInfo() << "Rejected connection from" << newConn->peerAddress().toString() << ": another client is already connected.";
            return;
        }

        QTcpSocket* conn = listener->nextPendingConnection();
        if (conn) {
            isClientConnected = true;
            clientSocket = conn;
            connMap.insert(conn->peerAddress().toString(), conn);
            qInfo() << "Accepted connection from" << conn->peerAddress().toString();
            //QMetaObject::invokeMethod(this, "handleConnection", Qt::QueuedConnection, Q_ARG(QTcpSocket*, conn));
            handleConnection(conn);
        }
    });

    stop = false;
    start();
    qInfo() << "Server started on" << address << port;
}

void Server::stopServer() {
    if (!listener) return;

    stop = true;
    listener->close();
    delete listener;
    listener = nullptr;

    logFile.close();
    quit();
    wait();
    isClientConnected=false;
}

void Server::run() {
    exec();
}

void Server::setSwitch(bool state) {
    QMutexLocker locker(&mutex);
    switchState = state;
}

bool Server::isSwitchOn() {
    QMutexLocker locker(&mutex);
    return switchState;
}

void Server::setB2Data(const QString& data) {
    QMutexLocker locker(&mutex);
    B2Data = data;
}

void Server::setB3Data(const QString& data) {
    QMutexLocker locker(&mutex);
    B3Data = data;
}

void Server::setB4Data(const QString& data) {
    QMutexLocker locker(&mutex);
    B4Data = data;
}

void Server::setB5Data(const QString& data) {
    QMutexLocker locker(&mutex);
    B5Data = data;
}

QMap<QString, QTcpSocket*> Server::GetMap() {
    QMutexLocker locker(&mutex);
    return connMap;
}

void Server::handleConnection(QTcpSocket* conn) {
    qInfo() << "Client connected from" << conn->peerAddress().toString();
    connect(conn, &QTcpSocket::disconnected, this, [=] {
        qInfo() << "Client disconnected from" << conn->peerAddress().toString();
        isClientConnected = false;
        connMap.remove(conn->peerAddress().toString());
        clientSocket = nullptr;
        conn->deleteLater();
    });

    sendB0Frame(conn, 0x09);

    QSharedPointer<quint8> seq(new quint8(0));
    QTimer* heartbeatTimer = new QTimer(conn);
    connect(heartbeatTimer, &QTimer::timeout, this, [=] {
        if (conn == nullptr) return;
        QAbstractSocket::SocketState state = conn->state();
        if (state == QAbstractSocket::ConnectedState) {
            (*seq)++;
            if (*seq > 9) {
                *seq = 1;
            }
            qInfo() << "Send B280 @: " << *seq;
            sendHeartbeat(conn, *seq);
        }
    });
    heartbeatTimer->start(10000);

    connect(conn, &QTcpSocket::readyRead, this, [=] {
        QByteArray buf = conn->readAll();
        Frame frame;
        try {
            frame = FrameOperations::decode(buf);
        } catch (const std::exception& e) {
            qWarning() << "Error decoding frame:" << e.what();
            return;
        }
        quint8 tempFrameType= static_cast<quint8>(frame.DATA[0]);
        switch (tempFrameType) {
        case 0xC0:
            qInfo() << "Received C0 frame, initializing...";
            sendB0Frame(conn, 0x01);
            break;
        case 0xC1:
            qInfo() << "Received C1 frame, continuing interaction...";
            //            if (isSwitchOn()) {
            //                sendFrameBasedOnNum(conn, frameNum++);
            //            }
            sendFrameBasedOnLastCmd(conn);
            break;
        case 0xC6:
            qInfo() << "Received C6 frame, continuing interaction...";
            sendB5Frame(conn);
            setSwitch(false);
            lastedFrameCmd="";//交易完成
            break;
        default:
            qWarning() << "Received unhandled frame type:" << hex << QString("0x%1").arg(static_cast<quint8>(frame.DATA[0]), 2, 16, QLatin1Char('0')).toUpper();
        }
    });
}

void Server::sendHeartbeat(QTcpSocket* conn, quint8& seq) {
    if(isHeartbeatOpened){
        //QTimer* timer = new QTimer(this);
        //connect(timer, &QTimer::timeout, this, [=, &seq] {
        Frame frame(seq, QByteArray::fromHex("B20000000080"));
        QByteArray encodedFrame = FrameOperations::encode(frame);
        conn->write(encodedFrame);

        //        ++seq;
        //        if (seq > 9) {
        //            seq = 1;
        //        }
        //});
        //timer->start(10000);
    }
}

void Server::sendB0Frame(QTcpSocket* conn, quint8 version) {
    Frame frame(1, QByteArray::fromHex("B0000000").append(static_cast<char>(version)));
    QByteArray encodedFrame = FrameOperations::encode(frame);
    conn->write(encodedFrame);
}

void Server::sendB2Frame(QTcpSocket* conn) {
    QByteArray data = QByteArray::fromHex(B2Data.toUtf8());
    conn->write(data);
    lastedFrameCmd="B2";
}

void Server::sendB3Frame(QTcpSocket* conn) {
    QByteArray data = QByteArray::fromHex(B3Data.toUtf8());
    conn->write(data);
    lastedFrameCmd="B3";
}

void Server::sendB4Frame(QTcpSocket* conn) {
    QByteArray data = QByteArray::fromHex(B4Data.toUtf8());
    conn->write(data);
    lastedFrameCmd="B4";
}

void Server::sendB5Frame(QTcpSocket* conn) {
    QByteArray data = QByteArray::fromHex(B5Data.toUtf8());
    conn->write(data);
    lastedFrameCmd="B5";
}

void Server::sendFrameBasedOnNum(QTcpSocket* conn, quint8 frameNum) {
    switch (frameNum) {
    case 0:
        sendB2Frame(conn);
        break;
    case 1:
        sendB3Frame(conn);
        break;
    case 2:
        sendB4Frame(conn);
        break;
    default:
        qWarning() << "Invalid frame number:" << frameNum;
    }
}

void Server::sendFrameBasedOnLastCmd(QTcpSocket *conn)
{
    if (lastedFrameCmd == ""){
        sendB2Frame(conn);
    }else if (lastedFrameCmd == "B2"){
        sendB3Frame(conn);
    }else if (lastedFrameCmd == "B3"){
        sendB4Frame(conn);
    }else if (lastedFrameCmd == "B4"){
        sendB5Frame(conn);
    }
}
