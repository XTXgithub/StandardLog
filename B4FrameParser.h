#ifndef B4FRAMEPARSER_H
#define B4FRAMEPARSER_H

#include <QObject>
#include <QString>
#include <QListView>
#include <QStandardItemModel>
#include <QDateTime>

class B4FrameParser : public QObject
{
    Q_OBJECT
public:
    explicit B4FrameParser(QObject *parent = nullptr);
    void parseFrame(const QString &frameHex, QListView *listView);

private:
    QByteArray parseCommunicationFrame(const QString &frameHex, QString &error);
    QString parseIssuerInfo(const QByteArray &infoBytes);
    QString parseLastStation(const QByteArray &stationBytes);
    int bytesToInt(const QByteArray &bytes);

    QStandardItemModel *model;
};

#endif // B4FRAMEPARSER_H
