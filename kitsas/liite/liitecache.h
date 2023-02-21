#ifndef LIITECACHE_H
#define LIITECACHE_H

#include <QObject>
#include <QHash>

class KitsasInterface;
class CacheLiite;

class LiiteCache : public QObject
{
    Q_OBJECT
public:
    explicit LiiteCache(QObject *parent, KitsasInterface* kitsas);
    ~LiiteCache();

    CacheLiite* liite(int liiteId);
    void ennakkoHaku(int liiteId);
    void tositteenLiitteidenEnnakkoHaku(int tositeId);

    void tyhjenna();
    void lisaaTallennettu(int liiteId, CacheLiite* liite);
    void poistaPoistettu(int liiteId);

signals:
    void liiteHaettu(int liiteId);
    void hakuVirhe(int virhe, int liiteId);

protected:
    CacheLiite* haku(int liiteId, bool ennakkohaku);

    bool kaynnistaHaku(int liiteId);
    void liiteSaapuu(int liiteId, QVariant* data);
    void tositeSaapuuu(QVariant* data);

    void karkeen(CacheLiite* liite);

    QHash<int,CacheLiite*> liitteet_;
    KitsasInterface* kitsas_;

    CacheLiite* uusin_ = nullptr;
    CacheLiite* vanhin_ = nullptr;

    qsizetype koko_ = 0L;

    qsizetype rajaKoko_ = 1024L * 1024L * 80L; // 80 Mt

};

#endif // LIITECACHE_H
