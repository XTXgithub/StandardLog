#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets/QGridLayout>
#include <QTreeView>
#include "Server.h"
#include "DataExtractor.h"
#include "MyDebugStream.h"
#include "B3FrameParser.h"
#include "B4FrameParser.h"
#include "xio_client.h"
#include "VPRClient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
extern MyDebugStream* globalDebugStream;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
     #define XTXNULL nullptr
#else
     #define XTXNULL NULL
#endif
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public:
    QGridLayout * getGridLayout();
    VehicleInfo *m_CurrentVehInfo; //暂时支持10辆车
private slots:
    void on_action_Q_triggered();

    void on_action_I_triggered();

    void on_pushButton_4_clicked();

    //    void onTreeViewItemClicked(QTreeView *treeView);
    void on_pbRSU1_clicked();

    void on_pbRSU2_clicked();

    void on_pbSender_clicked();

    void on_treeView_clicked(const QModelIndex &index);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_8_clicked();

    void on_Sender_RSU(quint32 index);

    void on_Sender_Plate(quint32 index);

    void on_pushButton_7_clicked();

    void on_pushButton_9_clicked();

    void on_pushButton_10_clicked();

    void on_pushButton_11_clicked();

    void on_cbParser_3_stateChanged(int arg1);

public slots:
    void onTreeViewItemClicked(const QModelIndex &index);

    void handleSendSignal(unsigned int signalValue);

    void handleSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);


private:
    Ui::MainWindow *ui;

    QString m_DBFileName;
    bool m_IsServer1Running = false;
    bool m_IsServer2Running = false;
    Server *m_Server1 = nullptr;
    QString m_Server1IP="127.0.0.1";
    QString m_Server1Port ="8888";

    Server *m_Server2 = nullptr;
    QString m_Server2IP="127.0.0.1";
    QString m_Server2Port ="9999";


    MyDebugStream* debugStream; // 用于重定向日志的自定义流

    B3FrameParser *B3parser;
    B4FrameParser *B4parser;

    XIOClient *m_client;

    VPRClient *m_plateClient;

    quint32 currentSignalValue;

    QStandardItemModel *model;

    void goWork(const std::vector<int>& delays,const std::vector<int>& values);
};
//第一种方式实现异步发送信号
/*class Worker : public QObject {
    Q_OBJECT

public:
    Worker() : initialSignalDelay(2000) {}  // 默认初始延迟为 2000 毫秒

    void setInitialSignalDelay(int delay) {
        initialSignalDelay = delay;
    }

    void setSubsequentSignalDelays(const std::vector<int>& delays) {
        subsequentSignalDelays = delays;
    }

    void setSignalValues(const std::vector<int>& values) {
        signalValues = values;
    }

public slots:
    void process() {
        // 发送车辆数据
        qDebug() << "发送车辆数据";
        emit sendVehicleData();

        // 使用 QTimer 发送具有自定义延迟的信号
        QTimer* signalTimer = new QTimer(this);
        connect(signalTimer, &QTimer::timeout, this, &Worker::emitSignal);

        // 初始信号延迟
        signalTimer->start(initialSignalDelay);

        // 发送具有不同延迟的信号
        for (int signalValue : signalValues) {
            int currentDelay = subsequentSignalDelays.at(signalValue % subsequentSignalDelays.size());
            QTimer::singleShot(currentDelay, this, &Worker::emitSignal);
        }
    }

signals:
    void sendVehicleData();
    void sendSignal(unsigned int signalValue);

private slots:
    void emitSignal() {
        // 将当前信号值发送出去，由外部处理
        emit sendSignal(currentSignalValue);
        currentSignalValue++;
    }

private:
    int initialSignalDelay;
    std::vector<int> subsequentSignalDelays = {500, 500, 1200, 500, 400}; // 默认延迟数组
    std::vector<int> signalValues = {8, 12, 4, 2, 0, 1, 0}; // 默认信号值数组
    unsigned int currentSignalValue = 0;
};*/

//第二种方式
class Worker : public QObject {
    Q_OBJECT

public:
    Worker() : initialSignalDelay(2000) {}  // 默认初始延迟为 2000 毫秒

    void setInitialSignalDelay(int delay) {
        initialSignalDelay = delay;
    }

    void setSubsequentSignalDelays(const std::vector<int>& delays) {
        subsequentSignalDelays = delays;
    }

    void setSignalValues(const std::vector<int>& values) {
        signalValues = values;
    }

public slots:
    void process() {
        // 发送车辆数据
//        qDebug() << "发送车辆数据";
//        emit sendVehicleData();

        //        // 初始信号延迟
        //        QTimer::singleShot(initialSignalDelay, this, &Worker::emitInitialSignal);
        // 初始信号延迟
        QTimer::singleShot(initialSignalDelay, this, &Worker::emitSignal);
    }

signals:
    void sendVehicleData(quint32 index);
    void sendSignal(unsigned int signalValue);
    void sendPlate(quint32 index);

private slots:
    //    void emitInitialSignal() {
    //        // 将当前信号值发送出去，由外部处理
    //        //        emit sendSignal(signalValues[currentSignalIndex]);
    //        //        qDebug()<<"send io:"<<signalValues[currentSignalIndex];
    //        //        currentSignalIndex++;

    //        // 发送具有不同延迟的信号
    //        qDebug()<<"IO counts:"<<signalValues.size();
    //        for (size_t i = 0; i < signalValues.size(); ++i) {
    //            //int currentDelay = subsequentSignalDelays.at(signalValues[i] % subsequentSignalDelays.size());
    //            int currentDelay =subsequentSignalDelays[i];
    //            //            QThread::sleep(currentDelay);
    //            //            emit sendSignal(signalValues[i]);
    //            //            qDebug()<<"sended IO["<<i<<"]: "<<signalValues[i];
    //            QTimer::singleShot(currentDelay, [this, i]() {
    //                emit sendSignal(signalValues[i]);
    //                qDebug()<<"sended IO["<<i<<"]: "<<signalValues[i];
    //                //                currentSignalIndex = i + 1;
    //            });
    //        }
    //    }
    void emitSignal() {
        if (currentSignalIndex < signalValues.size()) {
            int signalValue = signalValues[currentSignalIndex];
            qDebug()<<"sended IO["<<currentSignalIndex<<"]: "<<signalValues[currentSignalIndex];
            if (signalValue < 0 && signalValue >-10) {
                // -1~-9 表示发送车辆数据
                emit sendVehicleData(static_cast<quint32>(signalValue*-1));
            } else if(signalValue>=0){
                // 发送IO信号
                emit sendSignal(static_cast<quint32>(signalValue));
            }else if(signalValue < -10 && signalValue >-20){
                // -11~-19 表示发送车辆车牌数据
                emit sendPlate(static_cast<quint32>(signalValue*-1));
            }
            currentSignalIndex++;

            if (currentSignalIndex < signalValues.size()) {
                // 动态后续信号延迟
                int currentDelay = subsequentSignalDelays.at(currentSignalIndex % subsequentSignalDelays.size());
                qDebug()<<"sequentSignalDelays["<<currentSignalIndex<<"]: "<<currentDelay;
                QTimer::singleShot(currentDelay, this, &Worker::emitSignal);
            }
        }
    }
private:
    int initialSignalDelay;
    std::vector<int> subsequentSignalDelays = {0,3000,50,100, 100,300,300, 300,400,300,300}; // 默认延迟数组
    std::vector<int> signalValues =           {-1,0,  8,   12, 4,  0,   2,   0,   1,  0}; // 默认信号值数组
    size_t currentSignalIndex = 0;
};


#endif // MAINWINDOW_H
