#ifndef VPRDEV_H
#define VPRDEV_H

#include <QObject>
#include <QLibrary>
#include <windows.h>
#include <QImage>
#include <QColor>

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
// 定义回调函数类型
typedef void(WINAPI* CBFunc_GetRegResult)(int nHandle, T_VLPINFO* pVlpResult, void* pUser);
typedef void(WINAPI* CBFunc_GetStatus)(int nHandle, int nStatus, void* pUser);

extern "C" {
    __declspec(dllexport) int WINAPI VLPR_Init();
    __declspec(dllexport) int WINAPI VLPR_DeInit();
    __declspec(dllexport) int WINAPI VLPR_Login(int nType, char* sPara);
    __declspec(dllexport) int WINAPI VLPR_Logout(int nHandle);
    __declspec(dllexport) int WINAPI VLPR_SetResultCallBack(int nHandle, CBFunc_GetRegResult pFunc, void* pUser);
    __declspec(dllexport) int WINAPI VLPR_SetStatusCallBack(int nHandle, int nTimeInvl, CBFunc_GetStatus pFunc, void* pUser);
    __declspec(dllexport) int WINAPI VLPR_ManualSnap(int nHandle);
    __declspec(dllexport) int WINAPI VLPR_GetStatus(int nHandle, int* nStatus);
    __declspec(dllexport) int WINAPI VLPR_GetStatusMsg(int nStatusCode, char* sStatusMsg, int nMsgLen);
    __declspec(dllexport) int WINAPI VLPR_GetHWVersion(int nHandle, char* sVersion, int nVersionLen, char* sAPIVersion, int nAPIVersionLen);
}



class VPRDev : public QObject
{
    Q_OBJECT

public:
    VPRDev();
    ~VPRDev();

    static int Init();
    static int DeInit();
    static int Login(int nType, char* sPara);
    static int Logout(int nHandle);
    static int SetResultCallBack(int nHandle, CBFunc_GetRegResult pFunc, void* pUser);
    static int SetStatusCallBack(int nHandle, int nTimeInvl, CBFunc_GetStatus pFunc, void* pUser);
    static int ManualSnap(int nHandle);
    static int GetStatus(int nHandle, int* nStatus);
    static int GetStatusMsg(int nStatusCode, char* sStatusMsg, int nMsgLen);
    static int GetHWVersion(int nHandle, char* sVersion, int nVersionLen, char* sAPIVersion, int nAPIVersionLen);

    // 新的公共函数
    static void ProcessPlateInfo(int color, const char* plate);

private:
    static CBFunc_GetRegResult m_pRegResultCallback;
    static void* m_pUser;
    static void ConvertToBCD(int value, unsigned char* bcdArray);
    static void GeneratePlateImages(int color, const char* plate, QImage& bigImage, QImage& smallImage);
};

#endif // VPRDEV_H
