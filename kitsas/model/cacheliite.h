#ifndef CACHELIITE_H
#define CACHELIITE_H

#include <QByteArray>

class CacheLiite
{
public:
    enum LiiteTila { ALUSTAMATON, ENNAKKOHAETAAN, HAETAAN, HAETTU, TYHJENNETTY };

    CacheLiite();
    CacheLiite(LiiteTila tila);

    QByteArray data() const { return data_;}

    LiiteTila tila() const { return tila_;}
    void setTila(LiiteTila tila) { tila_ = tila;}

    void setData(const QByteArray& data);

private:
    LiiteTila tila_ = ALUSTAMATON;

    QByteArray data_;

};

#endif // CACHELIITE_H
