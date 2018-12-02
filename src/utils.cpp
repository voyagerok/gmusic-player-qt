#include "utils.h"
#include "config.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QNetworkInterface>
#include <QStandardPaths>
#include <chrono>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <thread>

namespace Utils
{

struct RsaDeleter {
    void operator()(RSA *ptr)
    {
        RSA_free(ptr);
    }
};

struct PkeyDeleter {
    void operator()(EVP_PKEY *pkey)
    {
        EVP_PKEY_free(pkey);
    }
};

struct MdCtxDeleter {
    void operator()(EVP_MD_CTX *ctx)
    {
        EVP_MD_CTX_destroy(ctx);
    }
};

using RsaPtr    = std::unique_ptr<RSA, RsaDeleter>;
using Byte      = unsigned char;
using ByteArray = std::unique_ptr<Byte[]>;
using PkeyPtr   = std::unique_ptr<EVP_PKEY, PkeyDeleter>;
using MdCtxPtr  = std::unique_ptr<EVP_MD_CTX, MdCtxDeleter>;

static int decode_key_comp(const QByteArray &bytes, int start_index, BIGNUM **component)
{
    int comp_size = 0;
    comp_size |= static_cast<unsigned char>(bytes[start_index]) << 24;
    comp_size |= static_cast<unsigned char>(bytes[start_index + 1]) << 16;
    comp_size |= static_cast<unsigned char>(bytes[start_index + 2]) << 8;
    comp_size |= static_cast<unsigned char>(bytes[start_index + 3]);
    auto data  = reinterpret_cast<const unsigned char *>(bytes.data());
    *component = BN_bin2bn(&data[start_index + 4], comp_size, nullptr);
    return comp_size;
}

static QByteArray rsa_encrypt(const QByteArray &rsa_in, BIGNUM *mod, BIGNUM *exp)
{
    RsaPtr rsa(RSA_new(), RsaDeleter());
#ifdef LCRYPTO_HAS_RSA_SET0_KEY
    RSA_set0_key(rsa.get(), mod, exp, nullptr);
#else
    rsa.get()->n = mod;
    rsa.get()->e = exp;
#endif

    int rsa_out_len_exp = RSA_size(rsa.get());
    ByteArray rsa_out(new Byte[rsa_out_len_exp]);
    int rsa_out_len_act = RSA_public_encrypt(rsa_in.size(), (const unsigned char *)rsa_in.data(),
                                             rsa_out.get(), rsa.get(), RSA_PKCS1_OAEP_PADDING);
    return QByteArray((char *)rsa_out.get(), rsa_out_len_act);
}

QString EncryptPasswd(const QString &login, const QString &passwd)
{
    static const char b64_key[] = "AAAAgMom/1a/v0lblO2Ubrt60J2gcuXSljGFQXgcyZWveWLEwo6prwgi3"
                                  "iJIZdodyhKZQrNWp5nKJ3srRXcUW+F1BD3baEVGcmEgqaLZUNBjm057pK"
                                  "RI16kB0YppeGx5qIQ5QjKzsR8ETQbKLNWgRY0QRNVz34kMJR3P/LgHax/"
                                  "6rmf5AAAAAwEAAQ==";
    QByteArray key_bytes   = QByteArray::fromRawData(b64_key, sizeof(b64_key));
    QByteArray decoded_key = QByteArray::fromBase64(key_bytes);

    BIGNUM *modulus = nullptr, *exponent = nullptr;
    int mod_size = decode_key_comp(decoded_key, 0, &modulus);
    decode_key_comp(decoded_key, mod_size + 4, &exponent);

    QByteArray hash = QCryptographicHash::hash(decoded_key, QCryptographicHash::Sha1);

    char signature[5];
    signature[0] = 0;
    signature[1] = hash[0];
    signature[2] = hash[1];
    signature[3] = hash[2];
    signature[4] = hash[3];

    QByteArray combined;
    combined.append(login);
    combined.append('\0');
    combined.append(passwd);

    QByteArray rsa_out = rsa_encrypt(combined, modulus, exponent);
    QByteArray raw_output;
    raw_output.append(signature, 5);
    raw_output.append(rsa_out);
    return QString::fromLatin1(raw_output.toBase64(QByteArray::Base64UrlEncoding));
}

std::pair<QString, QString> EncryptTrackId(const QString &track_id)
{
    using ResultType = std::pair<QString, QString>;

    static char s1[] = "VzeC4H4h+T2f0VI180nVX8x+Mb5HiTtGnKgH52Otj8ZCGDz9jRW"
                       "yHb6QXK0JskSiOgzQfwTY5xgLLSdUSreaLVMsVVWfxfa8Rw==";
    static char s2[] = "ZAPnhUkYwQ6y5DdQxWThbvhJHN8msQ1rqJw0ggKdufQjelrKuiG"
                       "GJI30aswkgCWTDyHkTGK9ynlqTkJ5L4CiGGUabGeo8M6JTQ==";
    QByteArray s1_dec = QByteArray::fromBase64(QByteArray::fromRawData(s1, sizeof(s1)));
    QByteArray s2_dec = QByteArray::fromBase64(QByteArray::fromRawData(s2, sizeof(s2)));

    Q_ASSERT(s1_dec.size() == s2_dec.size());
    QByteArray key = QByteArray(s1_dec.size(), Qt::Uninitialized);
    for (int i = 0; i < s1_dec.size(); ++i) {
        key[i] = s1_dec[i] ^ s2_dec[i];
    }

    static QByteArray hash_salt;
    if (hash_salt.isEmpty()) {
        auto timestamp = QDateTime::currentMSecsSinceEpoch();
        hash_salt.setNum(timestamp);
    }

    PkeyPtr pkey(
        EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, nullptr, (const unsigned char *)key.data(), key.size()),
        PkeyDeleter());
    MdCtxPtr ctx(EVP_MD_CTX_create(), MdCtxDeleter());

    EVP_DigestInit(ctx.get(), EVP_sha1());
    EVP_DigestSignInit(ctx.get(), nullptr, EVP_sha1(), nullptr, pkey.get());
    EVP_DigestSignUpdate(ctx.get(), track_id.toLatin1(), track_id.size());
    EVP_DigestSignUpdate(ctx.get(), hash_salt, hash_salt.size());

    static unsigned char res_hash[EVP_MAX_MD_SIZE];
    size_t res_hash_len;
    if (EVP_DigestSignFinal(ctx.get(), res_hash, &res_hash_len) != 1) {
        qDebug() << __FUNCTION__ << ": could not create digest message";
        return ResultType();
    }

    QByteArray hash     = QByteArray::fromRawData((const char *)res_hash, (int)res_hash_len);
    QByteArray hash_b64 = hash.toBase64(QByteArray::Base64UrlEncoding);
    return std::make_pair(QString::fromLatin1(hash_b64), QString::fromLatin1(hash_salt));
}

QString TimeFormat(int64_t secs)
{
    int64_t minutes     = secs / 60;
    int64_t leftSeconds = secs % 60;

    return QString("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(leftSeconds, 2, 10, QLatin1Char('0'));
}

QString MacAddress()
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    foreach (const QNetworkInterface &iface, interfaces) {
        if (!iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            return iface.hardwareAddress();
        }
    }

    return QString();
}

QString ThreadId()
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return QString::fromStdString(ss.str());
}

QString dataPath()
{
    QDir dir;
    QString dataPath = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0);
    if (!dir.exists(dataPath) || !dir.mkpath(dataPath)) {
        return QString();
    }
    return dataPath;
}

QString cachePath()
{
    QDir cacheDir;
    QString cachePath = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).at(0);
    if (!cacheDir.exists(cachePath) && !cacheDir.mkpath(cachePath)) {
        return QString();
    }
    return cachePath;
}

} // namespace Utils
