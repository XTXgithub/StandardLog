#include "B3FrameParser.h"
#include <QVBoxLayout>
#include <QByteArray>
#include <QtEndian> // 包含 qFromBigEndian 定义的头文件
#include <QException>

B3FrameParser::B3FrameParser(QWidget *parent)
    : QWidget(parent) {

    resultModel = new QStandardItemModel(this);
}

void B3FrameParser::setResultView(QListView *view) {
    resultView = view;
    resultView->setModel(resultModel);
}

void B3FrameParser::parseFrame(const QString &frameContent) {
    if (frameContent.isEmpty()) {
        qDebug()<<("请输入B3帧内容。\n");
        return;
    }
    QByteArray frameBytes = QByteArray::fromHex(frameContent.toLatin1());
    //addResult("解析结果：");
    resultModel->clear();
    parseCommunicationFrame(frameBytes);
}

void B3FrameParser::parseCommunicationFrame(const QByteArray &frameBytes) {
    try {
        QByteArray stx = frameBytes.left(2);
        quint8 ver = static_cast<quint8>(frameBytes[2]);
        quint8 seq = static_cast<quint8>(frameBytes[3]);
        QByteArray lenBytes = frameBytes.mid(4, 4);
        QByteArray data = frameBytes.mid(8, frameBytes.size() - 10);
        QByteArray crc = frameBytes.right(2);

        addResult(QString("STX（帧开始标志）：%1").arg(QString(stx.toHex().toUpper())));
        addResult(QString("VER（版本号）：%1").arg(ver));
        addResult(QString("SEQ（序列号）：%1").arg(seq));
        addResult(QString("LEN（长度）：%1").arg(bytesToInt(lenBytes)));
        addResult(QString("CRC（校验值）：%1").arg(QString(crc.toHex().toUpper())));

        parseB3Frame(data);
    } catch (const QException &e) {
        qDebug()<<("解析错误：异常 ")<<e.what();
    }
}

void B3FrameParser::parseB3Frame(const QByteArray &dataBytes) {
    int minExpectedLength = 1 + 4 + 1 + 79;
    if (dataBytes.size() < minExpectedLength) {
        qDebug()<<("数据长度不足，无法完整解析B3帧。\n");
        return;
    }

    try {
        quint8 frameType = static_cast<quint8>(dataBytes[0]);
        if (frameType != 0xB3) {
            qDebug()<<(QString("帧类型错误，期望B3，实际为%1。\n").arg(frameType, 2, 16, QChar('0')));
            return;
        }

        QByteArray obuid = dataBytes.mid(1, 4);
        quint8 errorCode = static_cast<quint8>(dataBytes[5]);
        QByteArray vehicleInfoBytes = dataBytes.mid(6, 79);

        if (errorCode != 0x00) {
            qDebug()<<(QString("执行状态代码表明错误，ErrorCode: %1\n").arg(errorCode));
            return;
        }

        parseVehicleInfo(vehicleInfoBytes);
    } catch (const QException &e) {
        qDebug()<<("解析错误：异常。\n");
    }
}

void B3FrameParser::parseVehicleInfo(const QByteArray &vehicleInfoBytes) {
    QString licensePlateNumber = QString::fromLocal8Bit(vehicleInfoBytes.left(12).replace('\0', ' ').trimmed());
    quint8 licensePlateColorCode = static_cast<quint8>(vehicleInfoBytes[13]);
    quint8 vehicleType = static_cast<quint8>(vehicleInfoBytes[14]);

    addResult(QString("车牌号：%1").arg(licensePlateNumber));
    addResult(QString("车牌颜色代码：%1").arg(licensePlateColorCode));
    addResult(QString("车型：%1").arg(vehicleType));

    if (parseIsCar(vehicleType)) {
        quint8 customerType = static_cast<quint8>(vehicleInfoBytes[15]);
        quint16 length = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(vehicleInfoBytes.mid(16, 2).data()));
        quint8 width = static_cast<quint8>(vehicleInfoBytes[18]);
        quint8 height = static_cast<quint8>(vehicleInfoBytes[19]);
        quint8 axleCount = static_cast<quint8>(vehicleInfoBytes[20]);
        quint8 wheelCount = static_cast<quint8>(vehicleInfoBytes[21]);
        quint16 wheelBase = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(vehicleInfoBytes.mid(22, 2).data()));
        quint32 approvedLoad = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(vehicleInfoBytes.mid(24, 3).prepend('\0').data()));
        QString vehicleFeatures = QString::fromLocal8Bit(vehicleInfoBytes.mid(27, 16).trimmed());
        QString vehicleIdentificationCode = QString(vehicleInfoBytes.mid(43, 16).toHex().toUpper());
        QString reservedFields = QString(vehicleInfoBytes.mid(59, 20).toHex().toUpper());

        addResult(QString("客户类型：%1").arg(customerType));
        addResult(QString("车辆尺寸（长×宽×高，米）：%1×%2×%3").arg(length / 10.0).arg(width / 10.0).arg(height / 10.0));
        addResult(QString("车辆核定载质量／准牵引总质量（kg）：%1").arg(axleCount));
        addResult(QString("车轴数：%1").arg(wheelCount));
        addResult(QString("轴距（米）：%1").arg(wheelBase / 10.0));
        addResult(QString("车辆核定载质量/客车座位数（kg）：%1").arg(approvedLoad));
        addResult(QString("车辆特征描述：%1").arg(vehicleFeatures));
        addResult(QString("车辆识别代码：%1").arg(vehicleIdentificationCode));
        addResult(QString("保留字段：%1").arg(reservedFields));
    } else {
        quint8 customerType = static_cast<quint8>(vehicleInfoBytes[15]);
        quint16 length = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(vehicleInfoBytes.mid(16, 2).data()));
        quint16 width = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(vehicleInfoBytes.mid(18, 2).data()));
        quint16 height = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(vehicleInfoBytes.mid(20, 2).data()));
        quint32 axleCount = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(vehicleInfoBytes.mid(22, 3).prepend('\0').data()));
        quint32 wheelCount = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(vehicleInfoBytes.mid(25, 3).prepend('\0').data()));
        quint32 wheelBase = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(vehicleInfoBytes.mid(28, 3).prepend('\0').data()));
        quint8 totalMass = static_cast<quint8>(vehicleInfoBytes[31]);
        QString vehicleFeatures = QString(vehicleInfoBytes.mid(32, 17).toHex().toUpper());
        QString vehicleIdentificationCode = QString(vehicleInfoBytes.mid(49, 16).toHex().toUpper());
        QString reservedFields = QString(vehicleInfoBytes.mid(65, 14).toHex().toUpper());

        addResult(QString("客户类型：%1").arg(customerType));
        addResult(QString("车辆尺寸（长×宽×高，米）：%1×%2×%3").arg(length / 1000.0).arg(width / 1000.0).arg(height / 1000.0));
        addResult(QString("核定质量/准牵引总质量kg：%1").arg(axleCount));
        addResult(QString("整备质量kg：%1").arg(wheelCount));
        addResult(QString("总质量kg：%1").arg(wheelBase));
        addResult(QString("载人数：%1").arg(totalMass));
        addResult(QString("车辆识别代码：%1").arg(vehicleFeatures));
        addResult(QString("车辆特征描述：%1").arg(vehicleIdentificationCode));
        addResult(QString("保留字段：%1").arg(reservedFields));
    }
}

bool B3FrameParser::parseIsCar(int nVehClass) {
    return static_cast<int>(CVehClass::VC_None) < nVehClass && nVehClass <= static_cast<int>(CVehClass::VC_Car4);
}

int B3FrameParser::bytesToInt(const QByteArray &bytes) {
    int value = 0;
    for (int i = 0; i < bytes.size(); ++i) {
        value = (value << 8) + static_cast<quint8>(bytes[i]);
    }
    return value;
}

void B3FrameParser::addResult(const QString &text) {
    QStandardItem *item = new QStandardItem(text);
    resultModel->appendRow(item);
    //resultView->update();
}
