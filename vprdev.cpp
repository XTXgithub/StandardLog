#include "vprdev.h"
#include <QDebug>
#include <cstring>
#include <ctime>
#include <QPainter>
#include <QBuffer>

// 初始化静态成员
CBFunc_GetRegResult VPRDev::m_pRegResultCallback = nullptr;
void* VPRDev::m_pUser = nullptr;

VPRDev::VPRDev()
{
}

VPRDev::~VPRDev()
{
}

int VPRDev::Init()
{
    qDebug() << "VLPR_Init called";
    // 初始化代码
    return 0;
}

int VPRDev::DeInit()
{
    qDebug() << "VLPR_DeInit called";
    // 反初始化代码
    return 0;
}

int VPRDev::Login(int nType, char* sPara)
{
    qDebug() << "VLPR_Login called with type:" << nType << " and parameter:" << sPara;
    // 登录代码
    return 0;
}

int VPRDev::Logout(int nHandle)
{
    qDebug() << "VLPR_Logout called with handle:" << nHandle;
    // 登出代码
    return 0;
}

int VPRDev::SetResultCallBack(int nHandle, CBFunc_GetRegResult pFunc, void* pUser)
{
    qDebug() << "VLPR_SetResultCallBack called with handle:" << nHandle;
    m_pRegResultCallback = pFunc;
    m_pUser = pUser;
    return 0;
}

int VPRDev::SetStatusCallBack(int nHandle, int nTimeInvl, CBFunc_GetStatus pFunc, void* pUser)
{
    qDebug() << "VLPR_SetStatusCallBack called with handle:" << nHandle << " and interval:" << nTimeInvl;
    // 设置状态回调代码
    return 0;
}

int VPRDev::ManualSnap(int nHandle)
{
    qDebug() << "VLPR_ManualSnap called with handle:" << nHandle;
    // 手动抓拍代码
    return 0;
}

int VPRDev::GetStatus(int nHandle, int* nStatus)
{
    qDebug() << "VLPR_GetStatus called with handle:" << nHandle;
    // 获取状态代码
    *nStatus = 0;
    return 0;
}

int VPRDev::GetStatusMsg(int nStatusCode, char* sStatusMsg, int nMsgLen)
{
    qDebug() << "VLPR_GetStatusMsg called with status code:" << nStatusCode;
    // 获取状态消息代码
    strncpy(sStatusMsg, "Status OK", nMsgLen);
    return 0;
}

int VPRDev::GetHWVersion(int nHandle, char* sVersion, int nVersionLen, char* sAPIVersion, int nAPIVersionLen)
{
    qDebug() << "VLPR_GetHWVersion called with handle:" << nHandle;
    // 获取硬件版本代码
    strncpy(sVersion, "1.0", nVersionLen);
    strncpy(sAPIVersion, "1.0", nAPIVersionLen);
    return 0;
}

void VPRDev::ConvertToBCD(int value, unsigned char* bcdArray)
{
    bcdArray[0] = ((value / 1000) << 4) | ((value / 100) % 10);
    bcdArray[1] = (((value / 10) % 10) << 4) | (value % 10);
}

QColor getPlateColor(int color) {
    switch (color) {
        case 0: return QColor("blue");
        case 1: return QColor("yellow");
        case 2: return QColor("black");
        case 3: return QColor("white");
        case 4: return QColor(0, 255, 0); // Gradient green
        case 5: return QColor(255, 255, 0); // Yellow-green double color
        case 6: return QColor(0, 0, 255); // Blue-white gradient
        case 11: return QColor("green");
        case 12: return QColor("red");
        default: return QColor("gray"); // 未确定颜色
    }
}

void VPRDev::GeneratePlateImages(int color, const char* plate, QImage& bigImage, QImage& smallImage)
{
    bigImage = QImage(800, 600, QImage::Format_RGB32);
    smallImage = QImage(200, 150, QImage::Format_RGB32);

    QColor plateColor = getPlateColor(color);
    QRect textRectBig(300, 250, 200, 100);  // Center rectangle for plate text in big image
    QRect textRectSmall(75, 50, 50, 25);    // Center rectangle for plate text in small image

    // Draw big image
    QPainter painterBig(&bigImage);
    painterBig.fillRect(bigImage.rect(), Qt::white);  // Background color
    painterBig.setBrush(plateColor);
    painterBig.drawRect(textRectBig);
    painterBig.setPen(Qt::black);
    painterBig.drawText(textRectBig, Qt::AlignCenter, QString(plate));
    painterBig.end();

    // Draw small image
    QPainter painterSmall(&smallImage);
    painterSmall.fillRect(smallImage.rect(), Qt::white);  // Background color
    painterSmall.setBrush(plateColor);
    painterSmall.drawRect(textRectSmall);
    painterSmall.setPen(Qt::black);
    painterSmall.drawText(textRectSmall, Qt::AlignCenter, QString(plate));
    painterSmall.end();
}

void VPRDev::ProcessPlateInfo(int color, const char* plate)
{
    if (m_pRegResultCallback)
    {
        T_VLPINFO vlpInfo;
        memset(&vlpInfo, 0, sizeof(T_VLPINFO));

        // 填充结构体字段
        vlpInfo.nVlpInfoSize = sizeof(T_VLPINFO);
        std::time_t now = std::time(nullptr);
        std::strftime(vlpInfo.sVlpTime, sizeof(vlpInfo.sVlpTime), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        vlpInfo.nVlpClass = 0;  // 假设车型类别为0

        ConvertToBCD(color, vlpInfo.bVlpColor);

        strncpy(reinterpret_cast<char*>(vlpInfo.sVlpText), plate, sizeof(vlpInfo.sVlpText) - 1);
        vlpInfo.VlpReliability = 100;  // 假设可信度为100

        QImage bigImage, smallImage;
        GeneratePlateImages(color, plate, bigImage, smallImage);

        QByteArray bigImageData;
        QBuffer bigBuffer(&bigImageData);
        bigBuffer.open(QIODevice::WriteOnly);
        bigImage.save(&bigBuffer, "PNG");

        QByteArray smallImageData;
        QBuffer smallBuffer(&smallImageData);
        smallBuffer.open(QIODevice::WriteOnly);
        smallImage.save(&smallBuffer, "PNG");

        vlpInfo.nImageLength[0] = bigImageData.size();
        vlpInfo.sImage[0] = new unsigned char[vlpInfo.nImageLength[0]];
        memcpy(vlpInfo.sImage[0], bigImageData.data(), vlpInfo.nImageLength[0]);

        vlpInfo.nImageLength[1] = smallImageData.size();
        vlpInfo.sImage[1] = new unsigned char[vlpInfo.nImageLength[1]];
        memcpy(vlpInfo.sImage[1], smallImageData.data(), vlpInfo.nImageLength[1]);

        vlpInfo.nImageLength[2] = 0;
        vlpInfo.sImage[2] = nullptr;

        // 调用回调函数
        m_pRegResultCallback(0, &vlpInfo, m_pUser);

        // 清理图像数据
        delete[] vlpInfo.sImage[0];
        delete[] vlpInfo.sImage[1];
    }
    else
    {
        qDebug() << "No result callback registered.";
    }
}

// 导出函数实现
extern "C" {
    __declspec(dllexport) int WINAPI VLPR_Init()
    {
        return VPRDev::Init();
    }

    __declspec(dllexport) int WINAPI VLPR_DeInit()
    {
        return VPRDev::DeInit();
    }

    __declspec(dllexport) int WINAPI VLPR_Login(int nType, char* sPara)
    {
        return VPRDev::Login(nType, sPara);
    }

    __declspec(dllexport) int WINAPI VLPR_Logout(int nHandle)
    {
        return VPRDev::Logout(nHandle);
    }

    __declspec(dllexport) int WINAPI VLPR_SetResultCallBack(int nHandle, CBFunc_GetRegResult pFunc, void* pUser)
    {
        return VPRDev::SetResultCallBack(nHandle, pFunc, pUser);
    }

    __declspec(dllexport) int WINAPI VLPR_SetStatusCallBack(int nHandle, int nTimeInvl, CBFunc_GetStatus pFunc, void* pUser)
    {
        return VPRDev::SetStatusCallBack(nHandle, nTimeInvl, pFunc, pUser);
    }

    __declspec(dllexport) int WINAPI VLPR_ManualSnap(int nHandle)
    {
        return VPRDev::ManualSnap(nHandle);
    }

    __declspec(dllexport) int WINAPI VLPR_GetStatus(int nHandle, int* nStatus)
    {
        return VPRDev::GetStatus(nHandle, nStatus);
    }

    __declspec(dllexport) int WINAPI VLPR_GetStatusMsg(int nStatusCode, char* sStatusMsg, int nMsgLen)
    {
        return VPRDev::GetStatusMsg(nStatusCode, sStatusMsg, nMsgLen);
    }

    __declspec(dllexport) int WINAPI VLPR_GetHWVersion(int nHandle, char* sVersion, int nVersionLen, char* sAPIVersion, int nAPIVersionLen)
    {
        return VPRDev::GetHWVersion(nHandle, sVersion, nVersionLen, sAPIVersion, nAPIVersionLen);
    }
}
