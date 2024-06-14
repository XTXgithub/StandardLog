#ifndef MYDEBUGSTREAM_H
#define MYDEBUGSTREAM_H

#include <QIODevice>
#include <QPlainTextEdit>
#include <QDebug>

class MyDebugStream : public QIODevice
{
    Q_OBJECT

public:
    MyDebugStream(QPlainTextEdit* textEdit, QObject* parent = nullptr);

    bool open(OpenMode mode) override;
    void close() override;

protected:
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char* data, qint64 maxSize) override;

private:
    QPlainTextEdit* textEdit;
};

#endif // MYDEBUGSTREAM_H
