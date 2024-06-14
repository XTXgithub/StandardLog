#include "B4FrameParser.h"
#include <QByteArray>
#include <QDateTime>
#include <QStringList>
#include <QStandardItem>
#include <QListView>
#include <QMessageBox>

B4FrameParser::B4FrameParser(QObject *parent) : QObject(parent), model(new QStandardItemModel(this))
{
}

void B4FrameParser::parseFrame(const QString &frameHex, QListView *listView)
{
    QString error;
    QByteArray data = parseCommunicationFrame(frameHex, error);
    if (!data.isEmpty())
    {
        // 解析B4帧前10个字节
        quint8 frameType = static_cast<quint8>(data[0]);  // 帧类型
        QString obuid = data.mid(1, 4).toHex().toUpper(); // 车载单元MAC地址
        quint8 errorCode = static_cast<quint8>(data[5]);  // 执行状态代码
        quint8 transType = static_cast<quint8>(data[6]);  // 交易类型
        quint32 cardRestMoney = static_cast<quint32>(data.mid(7, 4).toUInt(nullptr, 16));  // 卡余额

        // 从DATA中提取IssuerInfo和LastStation字段
        QByteArray issuerInfo = data.mid(11, 50);  // 发卡方信息
        QByteArray lastStation = data.mid(61, 43);  // 最后过站信息

        // 解析字段
        QString issuerInfoStr = parseIssuerInfo(issuerInfo);
        QString lastStationStr = parseLastStation(lastStation);

        // 显示解析结果
        model->clear();
        model->appendRow(new QStandardItem(QString("帧类型: %1").arg(frameType, 2, 16, QChar('0')).toUpper()));
        model->appendRow(new QStandardItem(QString("OBUID: %1").arg(obuid)));
        model->appendRow(new QStandardItem(QString("执行状态代码: %1").arg(errorCode, 2, 16, QChar('0')).toUpper()));
        model->appendRow(new QStandardItem(QString("交易类型: %1").arg(transType, 2, 16, QChar('0')).toUpper()));
        model->appendRow(new QStandardItem(QString("卡余额: %1").arg(cardRestMoney)));
        model->appendRow(new QStandardItem("发卡方信息:"));
        model->appendRow(new QStandardItem(issuerInfoStr));
        model->appendRow(new QStandardItem("最后过站信息:"));
        model->appendRow(new QStandardItem(lastStationStr));

        listView->setModel(model);
    }
    else
    {
        QMessageBox::critical(nullptr, "Error", error);
    }
}

int B4FrameParser::bytesToInt(const QByteArray &bytes) {
    int value = 0;
    for (int i = 0; i < bytes.size(); ++i) {
        value = (value << 8) + static_cast<quint8>(bytes[i]);
    }
    return value;
}

QByteArray B4FrameParser::parseCommunicationFrame(const QString &frameHex, QString &error)
{
    try
    {
        QByteArray frameBytes = QByteArray::fromHex(frameHex.toLatin1());
        // 提取STX, VER, SEQ, LEN
        QByteArray stx = frameBytes.mid(0, 2);
        //quint8 ver = frameBytes[2];
        //quint8 seq = frameBytes[3];
        QByteArray lenBytes = frameBytes.mid(4, 4);
        qint32 tlen=bytesToInt(lenBytes);
        quint16 lenData =static_cast<quint16>(tlen);// static_cast<quint16>(frameBytes.mid(4, 4).toInt(nullptr, 16)) & 0xFFFF;  // 取低16位作为长度

        // 根据LEN字段提取DATA字段
        QByteArray data = frameBytes.mid(8, lenData);
        // CRC检验略过，直接返回DATA部分
        error.clear();
        return data;
    }
    catch (...)
    {
        error = "Error parsing communication frame.";
        return QByteArray();
    }
}

QString B4FrameParser::parseIssuerInfo(const QByteArray &infoBytes)
{
    QString issuerIdentifier = infoBytes.mid(0, 8).toHex().toUpper();
    quint8 cardType = static_cast<quint8>(infoBytes[8]);
    quint8 cardVersion = static_cast<quint8>(infoBytes[9]);
    quint16 cardNetwork = static_cast<quint16>(infoBytes.mid(10, 2).toUInt(nullptr, 16));
    QString internalCardNumber = infoBytes.mid(12, 8).toHex().toUpper();
    QDate startDate = QDate::fromString(infoBytes.mid(20, 4).toHex(), "yyyyMMdd");
    QDate expiryDate = QDate::fromString(infoBytes.mid(24, 4).toHex(), "yyyyMMdd");
    QString vehiclePlate = QString::fromLocal8Bit(infoBytes.mid(28, 12)).trimmed();
    quint8 userType = static_cast<quint8>(infoBytes[40]);
    quint8 plateColor = static_cast<quint8>(infoBytes[41]);
    quint8 vehicleClass = static_cast<quint8>(infoBytes[42]);

    return QString("发卡方标识: %1\n卡片类型: %2\n卡片版本号: %3\n"
                   "卡片网络编号: %4\n内部卡号: %5\n启用日期: %6\n"
                   "到期日期: %7\n车牌号码: %8\n用户类型: %9\n"
                   "车牌颜色: %10\n车型: %11")
        .arg(issuerIdentifier)
        .arg(cardType)
        .arg(cardVersion)
        .arg(cardNetwork)
        .arg(internalCardNumber)
        .arg(startDate.toString("yyyy-MM-dd"))
        .arg(expiryDate.toString("yyyy-MM-dd"))
        .arg(vehiclePlate)
        .arg(userType)
        .arg(plateColor)
        .arg(vehicleClass);
}

QString B4FrameParser::parseLastStation(const QByteArray &stationBytes)
{
    quint8 applicationIdentifier = static_cast<quint8>(stationBytes[0]);
    quint8 recordLength =static_cast<quint8>( stationBytes[1]);
    quint8 applicationLockFlag = static_cast<quint8>(stationBytes[2]);
    QString netCode = stationBytes.mid(3, 2).toHex().toUpper();  // 网络编号
    QString stationCode = stationBytes.mid(5, 2).toHex().toUpper();  // 站点编号
    quint8 laneCode = static_cast<quint8>(stationBytes[7]);
    QDateTime passTime = QDateTime::fromSecsSinceEpoch(static_cast<quint32>(stationBytes.mid(8, 4).toUInt(nullptr, 16)));
    quint8 vehicleType = static_cast<quint8>(stationBytes[12]);
    quint8 entryExitStatus = static_cast<quint8>(stationBytes[13]);
    QString etcBoothNumber = stationBytes.mid(14, 3).toHex().toUpper();
    QDateTime tollBoothTime = QDateTime::fromSecsSinceEpoch(static_cast<quint32>(stationBytes.mid(17, 4).toUInt(nullptr, 16)));
    QString vehiclePlate = QString::fromLocal8Bit(stationBytes.mid(21, 12)).trimmed();
    quint8 plateColor = static_cast<quint8>(stationBytes[33]);
    quint8 axleCount = static_cast<quint8>(stationBytes[34]);
    quint32 totalWeight = static_cast<quint32>(stationBytes.mid(35, 3).toUInt(nullptr, 16));
    quint8 vehicleStatus = static_cast<quint8>(stationBytes[38]);
    quint32 transactionAmount = static_cast<quint32>(stationBytes.mid(39, 4).toUInt(nullptr, 16));

    return QString("应用标识符: %1\n记录长度: %2\n"
                   "应用锁定标志: %3\n网络编号: %4\n站点编号: %5\n"
                   "车道编号: %6\n通过时间: %7\n车辆类型: %8\n"
                   "进出口状态: %9\nETC门架编号: %10\n"
                   "收费门架时间: %11\n车牌号码: %12\n车牌颜色: %13\n"
                   "车轴数: %14\n总重: %15\n车辆状态: %16\n"
                   "交易金额: %17")
        .arg(applicationIdentifier)
        .arg(recordLength)
        .arg(applicationLockFlag)
        .arg(netCode)
        .arg(stationCode)
        .arg(laneCode)
        .arg(passTime.toString("yyyy-MM-dd HH:mm:ss"))
        .arg(vehicleType)
        .arg(entryExitStatus)
        .arg(etcBoothNumber)
        .arg(tollBoothTime.toString("yyyy-MM-dd HH:mm:ss"))
        .arg(vehiclePlate)
        .arg(plateColor)
        .arg(axleCount)
        .arg(totalWeight)
        .arg(vehicleStatus)
        .arg(transactionAmount);
}
