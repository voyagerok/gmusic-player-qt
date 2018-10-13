#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <optional>
#include <utility>

namespace Utils
{
QString EncryptPasswd(const QString &login, const QString &passwd);
std::pair<QString, QString> EncryptTrackId(const QString &track_id);
QString TimeFormat(int64_t seconds);
QString MacAddress();
QString ThreadId();

template <class T, class U, class F> std::optional<U> map_opt(const std::optional<T> &input, F &&f)
{
    if (!input) {
        return std::nullopt;
    }
    return std::make_optional(f(*input));
}
} // namespace Utils

#endif // UTILS_H
