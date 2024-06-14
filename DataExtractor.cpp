#include "DataExtractor.h"
#include <QFileInfo>
#include <QTextCodec>
#include <iostream>

DataExtractor::DataExtractor()
{
    initializeVehTypeMap();
}

DataExtractor& DataExtractor::instance()
{
    static DataExtractor instance;
    return instance;
}

QString DataExtractor::formatTime(const QString &timeStr)
{
    QString timePart = timeStr.split(' ')[1];
    QString formattedTime = timePart.replace(":", "").replace(".", "");
    return formattedTime;
}

void DataExtractor::initializeVehTypeMap()
{
    vehTypeMap["1"] = "一型客车";
    vehTypeMap["2"] = "二型客车";
    vehTypeMap["3"] = "三型客车";
    vehTypeMap["4"] = "四型客车";
    vehTypeMap["11"] = "一型货车";
    vehTypeMap["12"] = "二型货车";
    vehTypeMap["13"] = "三型货车";
    vehTypeMap["14"] = "四型货车";
    vehTypeMap["15"] = "五型货车";
    vehTypeMap["16"] = "六型货车";
    vehTypeMap["21"] = "一型专项作业车";
    vehTypeMap["22"] = "二型专项作业车";
    vehTypeMap["23"] = "三型专项作业车";
    vehTypeMap["24"] = "四型专项作业车";
    vehTypeMap["25"] = "五型专项作业车";
    vehTypeMap["26"] = "六型专项作业车";
}

QString DataExtractor::convertVehTypeToDescription(const QString &vehType)
{
    if (vehTypeMap.contains(vehType)) {
        return vehTypeMap[vehType];
    } else {
        return "未知车型";
    }
}

QString DataExtractor::convertDescriptionToVehType(const QString &description)
{
    return vehTypeMap.key(description, "未知类型"); // 如果找不到对应的键，返回"未知类型"
}

QString DataExtractor::getFinalQueryText(const QSqlQuery &query)
{
    QString queryString = query.lastQuery();
    QMap<QString, QVariant> boundValues = query.boundValues();

    QMapIterator<QString, QVariant> it(boundValues);
    while (it.hasNext()) {
        it.next();
        QString placeholder = it.key();
        QString value = it.value().toString();
        queryString.replace(placeholder, value);
    }

    return queryString;
}

QList<QMap<QString, QString>> DataExtractor::extractDataGroups(const QString &filePath)
{
    QList<QMap<QString, QString>> dataGroups;
    QMap<QString, QString> currentGroup = {{"LaneType", ""}, {"plate", ""}, {"vehType", ""}, {"B2", ""}, {"B3", ""}, {"B4", ""}, {"B5", ""}};
    QString currentId;
    QString temporaryValue;
    QString temporaryId;
    QString temporaryLaneType;
    bool startFlag = false;

    QFileInfo checkFile(filePath);
    if (!checkFile.exists() || !checkFile.isFile()) {
        std::cerr << "文件不存在: " << filePath.toStdString() << std::endl;
        return dataGroups;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "无法打开文件: " << filePath.toStdString() << std::endl;
        return dataGroups;
    }

    QTextStream in(&file);
    in.setCodec(QTextCodec::codecForName("GB2312"));

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.contains("B20000000080") || line.contains("0000000FD0")) {
            continue;
        }

        QStringList parts = line.split('|');
        if (parts.size() < 7) {
            continue;
        }

        QString timestamp = parts[0];
        QString level = parts[1];
        QString mainkey = parts[2];
        QString mainkeyid = parts[3];
        QString submainkey = parts[4];
        QString submainkeyid = parts[5];
        QString value = parts[6];

        if (submainkey == "LaneType") {
            if (value == "3") {
                temporaryLaneType = "入口";
            } else {
                temporaryLaneType = "出口";
            }
            currentGroup["LaneType"] = temporaryLaneType;
        }

        if (!startFlag) {
            if (mainkey == "FrameData" && submainkey == "RSU Passed" && mainkeyid == submainkeyid) {
                startFlag = true;
                currentId = formatTime(timestamp);
                temporaryValue = value;
            }
            continue;
        }

        if (startFlag) {
            if (submainkey == "ErrCode" && value == "B2:00H") {
                if (submainkeyid == currentId) {
                    if (currentGroup["B2"].isEmpty()) {
                        currentGroup["B2"] = temporaryValue;
                        continue;
                    } else if (value.contains("B2")) {
                        startFlag = false;
                        currentGroup = {{"LaneType", ""}, {"plate", ""}, {"vehType", ""}, {"B2", ""}, {"B3", ""}, {"B4", ""}, {"B5", ""}};
                        currentGroup["LaneType"] = temporaryLaneType;
                        continue;
                    }
                } else {
                    if (currentGroup["B2"] == temporaryValue) {
                        currentId = temporaryId;
                        continue;
                    } else {
                        if (currentGroup["B3"].isEmpty()) {
                            currentGroup["B2"] = temporaryValue;
                            currentId = temporaryId;
                            continue;
                        }
                    }
                }
            }

            if (mainkey == "FrameData" && submainkey == "RSU Passed") {
                temporaryValue = value;
                temporaryId = formatTime(timestamp);
                continue;
            }

            if (submainkey == "ErrCode" && submainkeyid == currentId && currentGroup["B3"].isEmpty()) {
                if (value == "B3:00H") {
                    currentGroup["B3"] = temporaryValue;
                    continue;
                } else if (value.contains("B3")) {
                    startFlag = false;
                    currentGroup = {{"LaneType", ""}, {"plate", ""}, {"vehType", ""}, {"B2", ""}, {"B3", ""}, {"B4", ""}, {"B5", ""}};
                    currentGroup["LaneType"] = temporaryLaneType;
                    continue;
                }
            }

            if (submainkey == "ErrCode" && submainkeyid == currentId && currentGroup["B4"].isEmpty()) {
                if (value == "B4:00H") {
                    currentGroup["B4"] = temporaryValue;
                    continue;
                } else if (value.contains("B4")) {
                    startFlag = false;
                    currentGroup = {{"LaneType", ""}, {"plate", ""}, {"vehType", ""}, {"B2", ""}, {"B3", ""}, {"B4", ""}, {"B5", ""}};
                    currentGroup["LaneType"] = temporaryLaneType;
                    continue;
                }
            }

            if (submainkey == "OBUPlate" && submainkeyid == currentId) {
                currentGroup["plate"] = value;
                continue;
            }

            if (submainkey == "OBUVehClass" && submainkeyid == currentId) {
                currentGroup["vehType"] = value;
                continue;
            }

            if (submainkey == "ErrCode" && submainkeyid == currentId && currentGroup["B5"].isEmpty()) {
                if (value == "B5:00H") {
                    currentGroup["B5"] = temporaryValue;
                    currentGroup["LaneType"] = temporaryLaneType;
                    dataGroups.append(currentGroup);
                    currentGroup = {{"LaneType", ""}, {"plate", ""}, {"vehType", ""}, {"B2", ""}, {"B3", ""}, {"B4", ""}, {"B5", ""}};
                    startFlag = false;
                } else {
                    startFlag = false;
                    currentGroup = {{"LaneType", ""}, {"plate", ""}, {"vehType", ""}, {"B2", ""}, {"B3", ""}, {"B4", ""}, {"B5", ""}};
                }
            }
        }
    }

    file.close();
    return dataGroups;
}

bool DataExtractor::saveDataToDatabase(const QList<QMap<QString, QString>> &dataGroups, const QString &dbPath)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qWarning() << "无法打开数据库:" << db.lastError();
        return false;
    }

    QSqlQuery query;

    // 检查表是否存在
    QString checkTableQuery = "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='DataGroups'";
    if (query.exec(checkTableQuery)) {
        if (query.next() && query.value(0).toInt() == 0) {
            // 创建表结构
            QString createTableQuery = R"(
                CREATE TABLE DataGroups (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    LaneType TEXT,
                    plate TEXT,
                    vehType TEXT,
                    B2 TEXT,
                    B3 TEXT,
                    B4 TEXT,
                    B5 TEXT
                )
            )";

            if (!query.exec(createTableQuery)) {
                qWarning() << "无法创建表结构:" << query.lastError();
                return false;
            }
        }
    } else {
        qWarning() << "检查表是否存在失败:" << query.lastError();
        return false;
    }

    // 插入数据
    QString insertQuery = R"(
        INSERT INTO DataGroups (LaneType, plate, vehType, B2, B3, B4, B5)
        VALUES (:LaneType, :plate, :vehType, :B2, :B3, :B4, :B5)
    )";

    query.prepare(insertQuery);

    for (const auto &group : dataGroups) {
        query.bindValue(":LaneType", group["LaneType"]);
        query.bindValue(":plate", group["plate"]);
        query.bindValue(":vehType", group["vehType"]);
        query.bindValue(":B2", group["B2"]);
        query.bindValue(":B3", group["B3"]);
        query.bindValue(":B4", group["B4"]);
        query.bindValue(":B5", group["B5"]);

        if (!query.exec()) {
            qWarning() << "插入数据失败:" << query.lastError();
            return false;
        }
    }

    db.close();
    return true;
}

QList<QMap<QString, QString>> DataExtractor::queryDataByVehTypeAndLaneType(const QString &vehType, const QString &laneType, const QString &dbPath)
{
    QList<QMap<QString, QString>> results;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qWarning() << "无法打开数据库:" << db.lastError();
        return results;
    }

    QSqlQuery query;
    query.prepare("SELECT LaneType, plate, vehType, B2, B3, B4, B5 FROM DataGroups WHERE vehType = :vehType AND LaneType = :LaneType");
    query.bindValue(":vehType", vehType);
    query.bindValue(":LaneType", laneType);

    if (!query.exec()) {
        qWarning() << "查询失败:" << query.lastError();
        db.close();
        return results;
    }

    while (query.next()) {
        QMap<QString, QString> record;
        record["LaneType"] = query.value("LaneType").toString();
        record["plate"] = query.value("plate").toString();
        record["vehType"] = query.value("vehType").toString();
        record["B2"] = query.value("B2").toString();
        record["B3"] = query.value("B3").toString();
        record["B4"] = query.value("B4").toString();
        record["B5"] = query.value("B5").toString();
        results.append(record);
    }

    db.close();
    return results;
}

QList<VehicleInfo> DataExtractor::queryDataByLaneTypeVehTypeAndPlate(const QString &laneType, const QString &vehType, const QString &plate, const QString &dbPath)
{
    QList<VehicleInfo> results;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qWarning() << "无法打开数据库:" << db.lastError();
        return results;
    }

    QSqlQuery query;
    query.prepare("SELECT LaneType, plate, vehType, B2, B3, B4, B5 FROM DataGroups WHERE LaneType = :LaneType AND vehType = :vehType AND plate = :plate");
    query.bindValue(":LaneType", laneType);
    query.bindValue(":vehType", vehType);
    query.bindValue(":plate", plate);

    if (!query.exec()) {
        qWarning() << "查询失败:" << query.lastError();
        db.close();
        return results;
    }

    while (query.next()) {
        VehicleInfo record;
        record.LaneType = query.value("LaneType").toString();
        record.plate = query.value("plate").toString();
        record.vehType = query.value("vehType").toString();
        record.B2 = query.value("B2").toString();
        record.B3 = query.value("B3").toString();
        record.B4 = query.value("B4").toString();
        record.B5 = query.value("B5").toString();
        results.append(record);
    }

    db.close();
    return results;
}

void DataExtractor::displayDataInTreeView(const QString &vehType, const QString &laneType, const QString &dbPath, QTreeView *treeView)
{
    QList<QMap<QString, QString>> data = queryDataByVehTypeAndLaneType(vehType, laneType, dbPath);

    QStandardItemModel *model = new QStandardItemModel;
    model->setHorizontalHeaderLabels({"LaneType-vehType-Plate"});

    QMap<QString, QStandardItem*> laneTypeItems;
    QMap<QString, QStandardItem*> vehTypeItems;

    for (const auto &group : data) {
        QString laneType = group["LaneType"];
        QString vehType = group["vehType"];
        QString plate = group["plate"];

        if (!laneTypeItems.contains(laneType)) {
            QStandardItem *laneTypeItem = new QStandardItem(laneType);
            model->appendRow(laneTypeItem);
            laneTypeItems[laneType] = laneTypeItem;
        }

        if (!vehTypeItems.contains(vehType)) {
            QStandardItem *vehTypeItem = new QStandardItem(vehType);
            laneTypeItems[laneType]->appendRow(vehTypeItem);
            vehTypeItems[vehType] = vehTypeItem;
        }

        QStandardItem *plateItem = new QStandardItem(plate);
        vehTypeItems[vehType]->appendRow(plateItem);

        // 将查询结果隐藏保存在 TreeView 控件中
        QVariantMap dataMap;
        dataMap["LaneType"] = group["LaneType"];
        dataMap["plate"] = group["plate"];
        dataMap["vehType"] = group["vehType"];
        dataMap["B2"] = group["B2"];
        dataMap["B3"] = group["B3"];
        dataMap["B4"] = group["B4"];
        dataMap["B5"] = group["B5"];
        plateItem->setData(dataMap, Qt::UserRole + 1);
    }

    treeView->setModel(model);
}
