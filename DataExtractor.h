#ifndef DATAEXTRACTOR_H
#define DATAEXTRACTOR_H

#include <QString>
#include <QMap>
#include <QStringList>
#include <QTreeView>
#include <QStandardItemModel>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QList>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

// 定义一个结构体来存储查询结果
struct VehicleInfo {
    QString LaneType;
    QString plate;
    QString vehType;
    QString B2;
    QString B3;
    QString B4;
    QString B5;
};

class DataExtractor
{
public:
    static DataExtractor& instance();
    QList<QMap<QString, QString>> extractDataGroups(const QString &filePath);
    bool saveDataToDatabase(const QList<QMap<QString, QString>> &dataGroups, const QString &dbPath);
    QList<QMap<QString, QString>> queryDataByVehTypeAndLaneType(const QString &vehType, const QString &laneType, const QString &dbPath);
    QList<VehicleInfo> queryDataByLaneTypeVehTypeAndPlate(const QString &laneType, const QString &vehType, const QString &plate, const QString &dbPath);
    void displayDataInTreeView(const QString &vehType, const QString &laneType, const QString &dbPath, QTreeView *treeView);
    QString convertVehTypeToDescription(const QString &vehType);
    QString convertDescriptionToVehType(const QString &description);
    QString getFinalQueryText(const QSqlQuery &query);

private:
    DataExtractor();  // 构造函数私有化
    DataExtractor(const DataExtractor&) = delete;  // 禁用拷贝构造
    DataExtractor& operator=(const DataExtractor&) = delete;  // 禁用赋值运算符
    QString formatTime(const QString &timeStr);

    QMap<QString, QString> vehTypeMap;  // 存储字符数字和字符数字说明的映射
    void initializeVehTypeMap();  // 初始化字符数字和字符数字说明的映射
};

#endif // DATAEXTRACTOR_H
