#ifndef CACHELIITE_H
#define CACHELIITE_H

#include <QByteArray>
#include <QPixmap>

class CacheLiite
{
public:
    enum LiiteTila { ALUSTAMATON, ENNAKKOHAETAAN, HAETAAN, HAETTU, TYHJENNETTY, KELVOTON };

    CacheLiite();
    CacheLiite(LiiteTila tila);

    QByteArray data() const { return data_;}
    qsizetype size() const { return data_.size();}


    LiiteTila tila() const { return tila_;}
    void setTila(LiiteTila tila) { tila_ = tila;}

    void setData(const QByteArray& data);

    void sijoitaKarkeen(CacheLiite* karki);
    CacheLiite* seuraava() { return seuraava_;}

    bool lukossa() { return lukko_;}
    void lukitse();
    void vapauta();

    bool kaytettavissa() const;

    void tyhjenna();

    QByteArray *dataPtr();
    QPixmap thumb() const;

private:
    LiiteTila tila_ = ALUSTAMATON;

    QByteArray data_;
    QPixmap thumb_;

    CacheLiite* edellinen_ = nullptr;
    CacheLiite* seuraava_ = nullptr;

    int lukko_ = 0;

};

#endif // CACHELIITE_H
