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

    CacheLiite* liite(int liiteId);
    void ennakkoHaku(int liiteId);
    void tositteenLiitteidenEnnakkoHaku(int tositeId);

    void tyhjenna();
signals:
    void liiteHaettu(int liiteId);

protected:
    CacheLiite* haku(int liiteId, bool ennakkohaku);

    bool kaynnistaHaku(int liiteId);
    void liiteSaapuu(int liiteId, QVariant* data);
    void tositeSaapuuu(QVariant* data);


    QHash<int,CacheLiite*> liitteet_;
    KitsasInterface* kitsas_;

};

#endif // LIITECACHE_H
