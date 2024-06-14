#include "MyDebugStream.h"

MyDebugStream::MyDebugStream(QPlainTextEdit* textEdit, QObject* parent)
    : QIODevice(parent), textEdit(textEdit)
{
}

bool MyDebugStream::open(OpenMode mode)
{
    setOpenMode(mode);
    return true;
}

void MyDebugStream::close()
{
    setOpenMode(NotOpen);
}

qint64 MyDebugStream::readData(char* data, qint64 maxSize)
{
    // Reading is not implemented
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    return -1;
}

qint64 MyDebugStream::writeData(const char* data, qint64 maxSize)
{
    QString text = QString::fromUtf8(data, static_cast<int>(maxSize));
    textEdit->appendPlainText(text);
    return maxSize;
}
