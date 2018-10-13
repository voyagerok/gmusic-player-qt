#ifndef PROXYRESULT_H
#define PROXYRESULT_H

#include <QObject>
#include <QVariant>

class ProxyResult : public QObject
{
    Q_OBJECT
public:
    enum Status { OK, Error };

    explicit ProxyResult(QObject *parent = nullptr);

signals:
    void ready(int status, QVariant value);
};

Q_DECLARE_METATYPE(ProxyResult *)

#endif // PROXYRESULT_H
