#ifndef VPRCLIENT_H
#define VPRCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QLibrary>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
     #define XTXNULL nullptr
#else
     #define XTXNULL NULL
#endif
// 定义 T_VLPINFO 结构体
typedef struct _vlp_info
{
    int nVlpInfoSize;
    char sVlpTime[20];
    int nVlpClass;
    unsigned char bVlpColor[2];
    unsigned char sVlpText[16];
    unsigned int VlpReliability;
    unsigned int nImageLength[3];
    unsigned char *sImage[3];
} T_VLPINFO;
typedef void(* CBFunc_GetRegResult)(int nHandle, T_VLPINFO* pVlpResult, void* pUser);
typedef void(* CBFunc_GetStatus)(int nHandle, int nStatus, void* pUser);
// 定义函数指针类型
typedef int (*Func_VLPR_Init)();
typedef int (*Func_VLPR_DeInit)();
typedef int (*Func_VLPR_Login)(int nType, char* sPara);
typedef int (*Func_VLPR_Logout)(int nHandle);
typedef int (*Func_VLPR_SetResultCallBack)(int nHandle, CBFunc_GetRegResult pFunc, void* pUser);
typedef int (*Func_VLPR_SetStatusCallBack)(int nHandle, int nTimeInvl, CBFunc_GetStatus pFunc, void* pUser);
typedef int (*Func_VLPR_ManualSnap)(int nHandle);
typedef int (*Func_VLPR_GetStatus)(int nHandle, int* nStatus);
typedef int (*Func_VLPR_GetStatusMsg)(int nStatusCode, char* sStatusMsg, int nMsgLen);
typedef int (*Func_VLPR_GetHWVersion)(int nHandle, char* sVersion, int nVersionLen, char* sAPIVersion, int nAPIVersionLen);

class VPRClient : public QObject
{
    Q_OBJECT

public:
    VPRClient(const QString& host, quint16 port, QObject* parent = XTXNULL);
    ~VPRClient();

    bool connectToServer();

    void stop(); // New function to disconnect

    void sendPlateInfo(int color, const QString& plate);

    // 公共函数，供外部调用
    static void SendPlateInfo(const QString& host, quint16 port, int color, const QString& plate);

    // 动态加载库及调用函数
    bool loadVPRDevLibrary(const QString& libraryPath);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    QString m_host;
    quint16 m_port;
    QTcpSocket* m_socket;

    QLibrary m_library;
    Func_VLPR_Init m_init;
    Func_VLPR_DeInit m_deinit;
    Func_VLPR_Login m_login;
    Func_VLPR_Logout m_logout;
    Func_VLPR_SetResultCallBack m_setResultCallback;
    Func_VLPR_SetStatusCallBack m_setStatusCallback;
    Func_VLPR_ManualSnap m_manualSnap;
    Func_VLPR_GetStatus m_getStatus;
    Func_VLPR_GetStatusMsg m_getStatusMsg;
    Func_VLPR_GetHWVersion m_getHWVersion;
};

#endif // VPRCLIENT_H
