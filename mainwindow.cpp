#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <ctime>
MyDebugStream* globalDebugStream = nullptr;

void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QByteArray localMsg = msg.toLocal8Bit();
    QString text;
    switch (type) {
    case QtDebugMsg:
        text = QString("Debug: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtInfoMsg:
        text = QString("Info: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtWarningMsg:
        text = QString("Warning: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtCriticalMsg:
        text = QString("Critical: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtFatalMsg:
        text = QString("Fatal: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function);
        abort();
    }

    fprintf(stderr, "%s\n", text.toLocal8Bit().constData());

    if (globalDebugStream) {
        globalDebugStream->write(text.toLocal8Bit());
    }
}

unsigned short CRC16_X25(unsigned char *puchMsg, int usDataLen) {
    int crc = 0xFFFF, cnt, k;
    for (cnt = 0; cnt < usDataLen; cnt++) {
        crc ^= puchMsg[cnt];
        for (k = 0; k < 8; k++) {
            if (crc & 0x1) {
                crc = (crc >> 1) ^ 0x8408;
            } else {
                crc = (crc >> 1);
            }
        }
    }
    crc = (~crc) & 0xFFFF;
    return (unsigned short)crc;
}


// 将时间调整为当前时间减去一天
QString adjustEntryTime(const QString &hex_with_prefix) {
    // Convert hex string to bytes
    std::vector<unsigned char> b4_frame;
    for (int i = 0; i < hex_with_prefix.length(); i += 2) {
        bool ok;
        unsigned int byte = hex_with_prefix.mid(i, 2).toUInt(&ok, 16);
        b4_frame.push_back(static_cast<unsigned char>(byte));
    }

    // Check if the data includes the 16-byte prefix and enough length for B4 frame
    const size_t MIN_FRAME_LENGTH = 121+5;
    if (b4_frame.size() < MIN_FRAME_LENGTH) {
        std::cerr << "Input data is too short." << std::endl;
        return hex_with_prefix;
    }

    // Extract and verify the CRC
    size_t data_len = b4_frame.size() - 2;
    unsigned short crc_original = (b4_frame[data_len] << 8) | b4_frame[data_len + 1];
    unsigned short crc_calculated = CRC16_X25(&b4_frame[2], data_len - 2);
    if (crc_original != crc_calculated) {
        std::cerr << "CRC check failed." << std::endl;
        return hex_with_prefix;
        //return "";
    }

    // Get the current Unix time and adjust entry time to current time minus one day
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    uint32_t unix_time = static_cast<uint32_t>(now_time_t - 86400); // Subtract one day (86400 seconds)

    // Convert adjusted Unix time back to bytes
    const size_t TIME_OFFSET = 77; // Adjust offset to account for the 16-byte prefix
    b4_frame[TIME_OFFSET] = (unix_time >> 24) & 0xFF;
    b4_frame[TIME_OFFSET + 1] = (unix_time >> 16) & 0xFF;
    b4_frame[TIME_OFFSET + 2] = (unix_time >> 8) & 0xFF;
    b4_frame[TIME_OFFSET + 3] = unix_time & 0xFF;

    const size_t TIME_OFFSET_2 = 121;
    b4_frame[TIME_OFFSET_2] = b4_frame[TIME_OFFSET];
    b4_frame[TIME_OFFSET_2 + 1] = b4_frame[TIME_OFFSET + 1];
    b4_frame[TIME_OFFSET_2 + 2] = b4_frame[TIME_OFFSET + 2];
    b4_frame[TIME_OFFSET_2 + 3] = b4_frame[TIME_OFFSET + 3];

    // Recalculate and update CRC
    crc_calculated = CRC16_X25(&b4_frame[2], data_len - 2);
    b4_frame[data_len] = (crc_calculated >> 8) & 0xFF;
    b4_frame[data_len + 1] = crc_calculated & 0xFF;

    // Convert modified frame back to hex string
    QString result;
    for (size_t i = 0; i < b4_frame.size(); ++i) {
        result.append(QString("%1").arg(b4_frame[i], 2, 16, QChar('0')));
    }

    return result;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_Server1(XTXNULL)
    , m_Server2(XTXNULL)
    , m_client(XTXNULL)
    , m_plateClient(XTXNULL)
{
    ui->setupUi(this);

    ui->logArea->setReadOnly(true);

    //    debugStream = new MyDebugStream(ui->logArea, this);
    //    debugStream->open(QIODevice::WriteOnly);
    //    globalDebugStream = debugStream;

    //    // 安装自定义的消息处理器
    //    qInstallMessageHandler(myMessageHandler);

    // 获取当前工作目录
    QString currentDir = QDir::currentPath()+"/db/";
    // 检查目录是否存在
    if (!QDir().exists(currentDir)) {
        // 目录不存在，创建目录
        bool created = QDir().mkdir(currentDir);
        if (created) {
            qDebug() << "Directory created:" << currentDir;
        } else {
            qDebug() << "Failed to create directory:" << currentDir;
        }
    } else {
        // 目录已存在
        qDebug() << "Directory already exists:" << currentDir;
    }
    m_DBFileName = currentDir + "BDB.db";

    B4parser = new B4FrameParser(this);
    B3parser = new B3FrameParser(this);
    B3parser->setResultView(ui->lvB3);

    connect(ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::handleSelectionChanged);

    this->menuBar()->raise();

    m_CurrentVehInfo = new VehicleInfo[10];  // 确保数组大小正确
    for (int i = 0; i < 10; ++i) {
        m_CurrentVehInfo[i] = VehicleInfo();  // 确保对象初始化
    }
}

MainWindow::~MainWindow()
{
    if (m_Server1 != XTXNULL) {
        m_Server1->stopServer();
        delete m_Server1;
        m_Server1 = XTXNULL;
    }

    if (m_Server2 != XTXNULL) {
        m_Server2->stopServer();
        delete m_Server2;
        m_Server2 = XTXNULL;
    }

    if (m_client != XTXNULL) {
        m_client->stop();
        delete m_client;
        m_client = XTXNULL;
    }

    if (m_plateClient != XTXNULL) {
        //m_plateClient->stop();
        delete m_plateClient;
        m_plateClient = XTXNULL;
    }

    delete B4parser;
    delete B3parser;
    delete [] m_CurrentVehInfo;
    delete ui;
}

QGridLayout *MainWindow::getGridLayout()
{
    return XTXNULL;
}

void MainWindow::on_action_Q_triggered()
{
    close();
}

void MainWindow::on_action_I_triggered()
{
    QString filter = "Log Files (*.log)";
    QString selectedFile = QFileDialog::getOpenFileName(nullptr, "Open Log File", "", filter);

    if (!selectedFile.isEmpty()) {
        QMessageBox::information(nullptr, "Selected File", "You selected: " + selectedFile);
        DataExtractor::instance().saveDataToDatabase(DataExtractor::instance().extractDataGroups(selectedFile), m_DBFileName);
    } else {
        QMessageBox::information(nullptr, "No Selection", "No file was selected.");
    }
}

void MainWindow::on_pushButton_4_clicked()
{
    QString tempLaneType;
    QString tempVehType;

    if (ui->rbExit->isChecked()) {
        tempLaneType = "出口";
    } else if (ui->rbEntry->isChecked()) {
        tempLaneType = "入口";
    }

    tempVehType = DataExtractor::instance().convertDescriptionToVehType(ui->comboBox->currentText());

    DataExtractor::instance().displayDataInTreeView(tempVehType, tempLaneType, m_DBFileName, ui->treeView);
}

void MainWindow::onTreeViewItemClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        qWarning() << "无效的索引";
        return;
    }

    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(ui->treeView->model());
    if (!model) {
        qWarning() << "无法转换为 QStandardItemModel";
        return;
    }

    QStandardItem *item = model->itemFromIndex(index);
    if (!item) {
        qWarning() << "无法获取 QStandardItem";
        return;
    }

    QVariant data = item->data(Qt::UserRole + 1);
    if (!data.isValid()) {
        qWarning() << "无效的数据";
        return;
    }

    QVariantMap dataMap = data.toMap();
    VehicleInfo tempVehInfo;
    tempVehInfo.LaneType = dataMap.value("LaneType").toString();
    tempVehInfo.plate = dataMap.value("plate").toString();
    tempVehInfo.vehType = dataMap.value("vehType").toString();
    tempVehInfo.B2 = dataMap.value("B2").toString();
    tempVehInfo.B3 = dataMap.value("B3").toString();
    tempVehInfo.B4 = dataMap.value("B4").toString();
    tempVehInfo.B5 = dataMap.value("B5").toString();

    //m_CurrentVehInfo[0] = tempVehInfo;

    model = new QStandardItemModel(this);  // 确保模型在 UI 退出时被删除
    ui->listView->setModel(model);

    model->setColumnCount(1);

    item = new QStandardItem(QString(tempVehInfo.LaneType + " _ " + DataExtractor::instance().convertVehTypeToDescription(tempVehInfo.vehType)
                                     + " _ " + tempVehInfo.plate));
    model->appendRow(item);

    item = new QStandardItem(QString("B2:%1").arg(tempVehInfo.B2));
    model->appendRow(item);

    item = new QStandardItem(QString("B3:%1").arg(tempVehInfo.B3));
    model->appendRow(item);
    if (ui->cbParser->isChecked())
        B3parser->parseFrame(tempVehInfo.B3);

    item = new QStandardItem(QString("B4:%1").arg(tempVehInfo.B4));
    model->appendRow(item);
    if (ui->cbParser->isChecked())
        B4parser->parseFrame(tempVehInfo.B4, ui->lvB4);

    item = new QStandardItem(QString("B5:%1").arg(tempVehInfo.B5));
    model->appendRow(item);

    ui->listView->show();
}

void MainWindow::on_pbRSU1_clicked()
{
    if (m_IsServer1Running) {
        m_Server1->stopServer();
        ui->pbRSU1->setText("启动天线一");
        m_IsServer1Running = false;
    } else {
        if (m_Server1 == nullptr) {
            m_Server1 = new Server(); // 确保父对象为 MainWindow，以便自动内存管理
        }
        m_Server1->startServer(m_Server1IP, static_cast<quint16>(m_Server1Port.toUInt()));
        ui->pbRSU1->setText("停止天线一");
        m_IsServer1Running = true;
    }
}

void MainWindow::on_pbRSU2_clicked()
{
    if (m_IsServer2Running) {
        m_Server2->stopServer();
        ui->pbRSU2->setText("启动天线二");
        m_IsServer2Running = false;
    } else {
        if (m_Server2 == nullptr) {
            m_Server2 = new Server(); // 确保父对象为 MainWindow，以便自动内存管理
        }
        m_Server2->startServer(m_Server2IP, static_cast<quint16>(m_Server2Port.toUInt()));
        ui->pbRSU2->setText("停止天线二");
        m_IsServer2Running = true;
    }
}

void MainWindow::on_Sender_RSU(quint32 index)
{
    if (index > 9) return;

    VehicleInfo &tempInfo = m_CurrentVehInfo[index-1];
    qDebug() << "on_pbSender_clicked : "<<index;
    qDebug() << tempInfo.plate;
    qDebug() << tempInfo.B2;
    qDebug() << tempInfo.B3;
    qDebug() << tempInfo.B4;
    qDebug() << tempInfo.B5;

    if ((ui->cbRSU1->isChecked()) && (m_Server1 != XTXNULL) && m_IsServer1Running)
    {
        qDebug() << "RSUServer1 Send switch opened";

        m_Server1->setB2Data(tempInfo.B2);
        m_Server1->setB3Data(tempInfo.B3);
        if(ui->cbParser_2->isChecked())
        {
            m_Server1->setB4Data(adjustEntryTime(tempInfo.B4).toUpper());
        }else
            m_Server1->setB4Data(tempInfo.B4);
        m_Server1->setB5Data(tempInfo.B5);

        QMap<QString, QTcpSocket*> tempMap1 = m_Server1->GetMap();
        for (auto it = tempMap1.begin(); it != tempMap1.end(); ++it) {
            qDebug() << "Addr: " << it.key();
            if (!it.value()) {
                qDebug() << "Key not found or type assertion failed";
            } else {
                m_Server1->sendFrameBasedOnNum(it.value(), 0);
                qDebug() << "Send B2 frame to Server 1";
            }
        }
    }
    if ((ui->cbRSU2->isChecked()) && (m_Server2 != nullptr) && m_IsServer2Running)
    {
        qDebug() << "RSUServer2 Send switch opened";

        m_Server2->setB2Data(tempInfo.B2);
        m_Server2->setB3Data(tempInfo.B3);
        if(ui->cbParser_2->isChecked())
        {
            m_Server2->setB4Data(adjustEntryTime(tempInfo.B4).toUpper());
        }else
            m_Server2->setB4Data(tempInfo.B4);
        m_Server2->setB5Data(tempInfo.B5);

        QMap<QString, QTcpSocket*> tempMap1 = m_Server2->GetMap();
        for (auto it = tempMap1.begin(); it != tempMap1.end(); ++it) {
            qDebug() << "Addr: " << it.key();
            if (!it.value()) {
                qDebug() << "Key not found or type assertion failed";
            } else {
                m_Server2->sendFrameBasedOnNum(it.value(), 0);
                qDebug() << "Send B2 frame to Server 1";
            }
        }
    }
}

void MainWindow::on_Sender_Plate(quint32 index)
{
    qDebug()<<"on_Sender_Plate index : "<<index;
    if (index >19 || index < 11) return;

    VehicleInfo &tempInfo = m_CurrentVehInfo[index-11];
    qDebug() << "on_Sender_Plate "<<tempInfo.plate;

    if (m_plateClient != nullptr) {
        QString plate = tempInfo.plate;

        QTextCodec *codec = QTextCodec::codecForName("GB2312");

        QByteArray encodedPlate = codec->fromUnicode(plate);

        int color = 1;
        if (tempInfo.vehType==1 || 11 == tempInfo.vehType || 21 == tempInfo.vehType)
            color = 0;
        m_plateClient->sendPlateInfo(color, QString::fromLocal8Bit(encodedPlate));
    }
}

void MainWindow::on_pbSender_clicked()
{
    if (m_client != nullptr) {
        m_client->sendData(QString::asprintf("%u", 0));
    }
}

void MainWindow::on_treeView_clicked(const QModelIndex &index)
{
    onTreeViewItemClicked(index);
}

void MainWindow::on_pushButton_clicked()
{
    if (m_client == nullptr) {
        m_client = new XIOClient(this); // 确保父对象为 MainWindow，以便自动内存管理
    }

    if (ui->pushButton->text() == "连接IO")
    {
        qDebug() << "连接IO";
        m_client->connectToServer("127.0.0.1", 1234);
        ui->pushButton->setText("断开IO");
    } else if (ui->pushButton->text() == "断开IO")
    {
        qDebug() << "断开IO";
        m_client->stop();
        ui->pushButton->setText("连接IO");
    }
}

void MainWindow::on_pushButton_2_clicked() {
    if (m_client != nullptr) {
        ui->cbRSU1->setChecked(true);

        QThread* workerThread = new QThread(this);
        Worker* worker = new Worker;

        worker->setInitialSignalDelay(3000);

        worker->moveToThread(workerThread);

        connect(workerThread, &QThread::started, worker, &Worker::process);
        connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
        connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

        connect(worker, &Worker::sendSignal, this, &MainWindow::handleSendSignal);

        connect(worker, &Worker::sendVehicleData, this, &MainWindow::on_Sender_RSU);

        workerThread->start();
    }
}

void MainWindow::handleSendSignal(unsigned int signalValue) {
    if (m_client != XTXNULL) {
        m_client->sendData(QString::asprintf("%u", signalValue));
    }
}

void MainWindow::on_pushButton_3_clicked()
{
    goWork({0, 50, 100, 100, 300, 300, 300, 3000, 400, 300, 300},{0, 8, 12, 4, 0, 2, -1, 0, 1, 0});
}


void MainWindow::on_pushButton_5_clicked()
{
    goWork({0, 600,72,  0,576,72,144,648,0},
    {-1, 8, 12,-11, 4,  0,  1,0});
}

void MainWindow::on_pushButton_6_clicked()
{
    goWork({0, 700, 72, 576, 72, 144, 700, 72, 576, 72, 144, 700, 72, 576, 71, 216, 648, 216, 647, 216, 648, 0},
    {-1, 8, 12, 4, 0, -2, 8, 12, 4, 0, -3, 8, 12, 4, 0, 1, 0, 1, 0, 1, 0});

}

void MainWindow::handleSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    Q_UNUSED(selected)
    //memset(m_CurrentVehInfo, 0, sizeof(m_CurrentVehInfo));

    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(ui->treeView->model());
    if (!model) {
        qWarning() << "无法转换为 QStandardItemModel";
        return;
    }

    QModelIndexList selectedIndexes = ui->treeView->selectionModel()->selectedIndexes();

    int count = std::min(selectedIndexes.size(), 10);
    for (int i = 0; i < count; ++i) {
        QModelIndex index = selectedIndexes.at(i);
        QStandardItem *item = model->itemFromIndex(index);
        if (!item) {
            qWarning() << "无法获取 QStandardItem";
            return;
        }

        QVariant data = item->data(Qt::UserRole + 1);
        if (!data.isValid()) {
            qWarning() << "无效的数据";
            return;
        }

        QVariantMap dataMap = data.toMap();
        VehicleInfo tempVehInfo;
        tempVehInfo.LaneType = dataMap.value("LaneType").toString();
        tempVehInfo.plate = dataMap.value("plate").toString();
        tempVehInfo.vehType = dataMap.value("vehType").toString();
        tempVehInfo.B2 = dataMap.value("B2").toString();
        tempVehInfo.B3 = dataMap.value("B3").toString();
        tempVehInfo.B4 = dataMap.value("B4").toString();
        tempVehInfo.B5 = dataMap.value("B5").toString();

        m_CurrentVehInfo[i] = tempVehInfo;

        qDebug() << "Selected Vehicle: " << m_CurrentVehInfo[i].plate;
    }
}

void MainWindow::goWork(const std::vector<int> &delays, const std::vector<int> &values)
{
    if (m_client != XTXNULL) {
        ui->cbRSU1->setChecked(true);

        QThread* workerThread = new QThread(this);
        Worker* worker = new Worker;

        worker->setInitialSignalDelay(3000);
        worker->setSubsequentSignalDelays(delays);
        worker->setSignalValues(          values);

        worker->moveToThread(workerThread);

        connect(workerThread, &QThread::started, worker, &Worker::process);
        connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
        connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

        connect(worker, &Worker::sendSignal, this, &MainWindow::handleSendSignal);
        connect(worker, &Worker::sendVehicleData, this, &MainWindow::on_Sender_RSU);
        connect(worker, &Worker::sendPlate, this, &MainWindow::on_Sender_Plate);

        workerThread->start();
    }
}

void MainWindow::on_pushButton_8_clicked()
{

    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(ui->treeView->model());
    if (!model) {
        qWarning() << "无法转换为 QStandardItemModel";
        return;
    }

    QModelIndexList selectedIndexes = ui->treeView->selectionModel()->selectedIndexes();

    int count = std::min(selectedIndexes.size(), 10);
    for (int i = 0; i < count; ++i) {
        QModelIndex index = selectedIndexes.at(i);
        QStandardItem *item = model->itemFromIndex(index);
        if (!item) {
            qWarning() << "无法获取 QStandardItem";
            return;
        }

        QVariant data = item->data(Qt::UserRole + 1);
        if (!data.isValid()) {
            qWarning() << "无效的数据";
            return;
        }
        QVariantMap dataMap = data.toMap();

        m_CurrentVehInfo[i].LaneType = dataMap.value("LaneType").toString();
        m_CurrentVehInfo[i].plate = dataMap.value("plate").toString();
        m_CurrentVehInfo[i].vehType = dataMap.value("vehType").toString();
        m_CurrentVehInfo[i].B2 = dataMap.value("B2").toString();
        m_CurrentVehInfo[i].B3 = dataMap.value("B3").toString();
        m_CurrentVehInfo[i].B4 = dataMap.value("B4").toString();
        m_CurrentVehInfo[i].B5 = dataMap.value("B5").toString();


        qDebug() << "Selected Vehicle: " << m_CurrentVehInfo[i].plate;
    }
}

void MainWindow::on_pushButton_7_clicked()
{
    goWork({0, 600,72,576,72,144,600,72,576,72,576,216,648,0},
    {-1,  8,12,  4, 0, -2,  8,12,  4, 0,  1,  0,  1,0});
}

void MainWindow::on_pushButton_9_clicked()
{
    if (m_plateClient != nullptr) {
        QString plate = QString::fromUtf8("赣T00009");
        // 获取 GB2312 编码的 QTextCodec 对象
        QTextCodec *codec = QTextCodec::codecForName("GB2312");
        // 将 QString 转换为 GB2312 编码的 QByteArray
        QByteArray encodedPlate = codec->fromUnicode(plate);
        // 将 QByteArray 转换回 QString 以发送
        m_plateClient->sendPlateInfo(1, QString::fromLocal8Bit(encodedPlate));
    }
}

void MainWindow::on_pushButton_10_clicked()
{
    if (m_plateClient == XTXNULL) {
        m_plateClient = new VPRClient(QString("127.0.0.1"),2345,this); // 确保父对象为 MainWindow，以便自动内存管理
    }

    if (ui->pushButton_10->text() == "连接车牌")
    {
        qDebug() << "连接车牌";
        m_plateClient->connectToServer();
        ui->pushButton_10->setText("断开车牌");
    } else if (ui->pushButton_10->text() == "断开车牌")
    {
        qDebug() << "断开车牌";
        m_plateClient->stop();
        ui->pushButton_10->setText("连接车牌");
    }
}

void MainWindow::on_pushButton_11_clicked()
{
    on_Sender_RSU(1);
}

void MainWindow::on_cbParser_3_stateChanged(int arg1)
{
    if(m_Server1!=XTXNULL){
        if(arg1==0){
            m_Server1->setHeartbeatStatus(true);
        }
        else{
            m_Server1->setHeartbeatStatus(false);
        }
    }

    if(m_Server2!=XTXNULL){
        if(arg1==0){
            m_Server2->setHeartbeatStatus(true);
        }
        else{
            m_Server2->setHeartbeatStatus(false);
        }

    }
}
