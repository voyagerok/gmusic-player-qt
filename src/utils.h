#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <utility>

namespace Utils
{
QString EncryptPasswd(const QString &login, const QString &passwd);
std::pair<QString, QString> EncryptTrackId(const QString &track_id);
QString TimeFormat(int64_t seconds);
QString MacAddress();
QString ThreadId();
} // namespace Utils

#endif // UTILS_H
