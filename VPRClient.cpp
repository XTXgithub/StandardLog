#include "VPRClient.h"
#include <QDebug>
#include <QHostAddress>
#include <QMetaType>

VPRClient::VPRClient(const QString& host, quint16 port, QObject* parent)
    : QObject(parent), m_host(host), m_port(port), m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, this, &VPRClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &VPRClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &VPRClient::onReadyRead);
    //connect(m_socket, qOverload<QAbstractSocket::SocketError>(&QTcpSocket::errorOccurred), this, &VPRClient::onErrorOccurred);
    connect(m_socket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this, &VPRClient::onErrorOccurred);
    //connect(m_socket, &QAbstractSocket::errorOccurred, this, &VPRClient::onErrorOccurred);

}

VPRClient::~VPRClient()
{
    if (m_socket->isOpen()) {
        m_socket->close();
    }
    delete m_socket;
}

bool VPRClient::connectToServer()
{
    m_socket->connectToHost(m_host, m_port);
    return m_socket->waitForConnected(3000);
}

void VPRClient::stop()
{
    //  if (m_socket->isOpen()) {
    //    m_socket->disconnectFromHost(); // Disconnect from server
    //  }
    if (m_socket->state() == QAbstractSocket::ConnectedState || m_socket->state() == QAbstractSocket::ConnectingState) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(3000);
        }
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            qDebug() << "VPRClient: Socket disconnection timed out, aborting.";
            m_socket->abort();
        } else {
            qDebug() << "VPRClient: Socket disconnected successfully.";
        }
    }
}

void VPRClient::sendPlateInfo(int color, const QString& plate)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QString message = QString("{color=%1;plate=%2;}").arg(color).arg(plate);
        m_socket->write(message.toLocal8Bit());
        m_socket->flush();
    } else {
        qWarning() << "VPRClient: Not connected to server.";
    }
}

void VPRClient::onConnected()
{
    qDebug() << "VPRClient: Connected to server.";
}

void VPRClient::onDisconnected()
{
    qDebug() << "VPRClient: Disconnected from server.";
}

void VPRClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    qDebug() << "VPRClient: Received from server:" << data;
}

void VPRClient::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    qWarning() << "VPRClient: Socket error:" << socketError << m_socket->errorString();
}

void VPRClient::SendPlateInfo(const QString& host, quint16 port, int color, const QString& plate)
{
    VPRClient client(host, port);
    if (client.connectToServer()) {
        client.sendPlateInfo(color, plate);
    } else {
        qWarning() << "VPRClient: Failed to connect to server.";
    }
}

bool VPRClient::loadVPRDevLibrary(const QString& libraryPath)
{
    m_library.setFileName(libraryPath);
    if (!m_library.load()) {
        qWarning() << "VPRClient: Failed to load library:" << m_library.errorString();
        return false;
    }

    m_init = reinterpret_cast<Func_VLPR_Init>(m_library.resolve("VLPR_Init"));
    m_deinit = reinterpret_cast<Func_VLPR_DeInit>(m_library.resolve("VLPR_DeInit"));
    m_login = reinterpret_cast<Func_VLPR_Login>(m_library.resolve("VLPR_Login"));
    m_logout = reinterpret_cast<Func_VLPR_Logout>(m_library.resolve("VLPR_Logout"));
    m_setResultCallback = reinterpret_cast<Func_VLPR_SetResultCallBack>(m_library.resolve("VLPR_SetResultCallBack"));
    m_setStatusCallback = reinterpret_cast<Func_VLPR_SetStatusCallBack>(m_library.resolve("VLPR_SetStatusCallBack"));
    m_manualSnap = reinterpret_cast<Func_VLPR_ManualSnap>(m_library.resolve("VLPR_ManualSnap"));
    m_getStatus = reinterpret_cast<Func_VLPR_GetStatus>(m_library.resolve("VLPR_GetStatus"));
    m_getStatusMsg = reinterpret_cast<Func_VLPR_GetStatusMsg>(m_library.resolve("VLPR_GetStatusMsg"));
    m_getHWVersion = reinterpret_cast<Func_VLPR_GetHWVersion>(m_library.resolve("VLPR_GetHWVersion"));

    if (!m_init || !m_deinit || !m_login || !m_logout || !m_setResultCallback ||
            !m_setStatusCallback || !m_manualSnap || !m_getStatus || !m_getStatusMsg || !m_getHWVersion) {
        qWarning() << "VPRClient: Failed to resolve some functions:" << m_library.errorString();
        return false;
    }

    return true;
}
