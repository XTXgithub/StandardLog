#ifndef B3FRAMEPARSER_H
#define B3FRAMEPARSER_H

#include <QWidget>
#include <QListView>
#include <QStandardItemModel>
#include <QByteArray>
#include <QDebug>

enum class CVehClass {
    VC_None = 0,
    VC_Car1 = 1,
    VC_Car2,
    VC_Car3,
    VC_Car4,
    VC_Car5,
    VC_Car6,
    VC_Truck = 10,
    VC_Truck1,
    VC_Truck2,
    VC_Truck3,
    VC_Truck4,
    VC_Truck5,
    VC_Truck6,
    VC_YJ1 = 21,
    VC_YJ2,
    VC_YJ3,
    VC_YJ4,
    VC_YJ5,
    VC_YJ6
};

class B3FrameParser : public QWidget {
    Q_OBJECT

public:
    B3FrameParser(QWidget *parent = nullptr);

    void setResultView(QListView *view);
    void parseFrame(const QString &frameContent);

private:
    void parseCommunicationFrame(const QByteArray &frameBytes);
    void parseB3Frame(const QByteArray &dataBytes);
    void parseVehicleInfo(const QByteArray &vehicleInfoBytes);

    bool parseIsCar(int nVehClass);
    int bytesToInt(const QByteArray &bytes);
    void addResult(const QString &text);

    QListView *resultView;
    QStandardItemModel *resultModel;
};

#endif // B3FRAMEPARSER_H
