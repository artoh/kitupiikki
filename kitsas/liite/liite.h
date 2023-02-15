#ifndef LIITE_H
#define LIITE_H

#include <QObject>

#include "cacheliite.h"
class LiitteetModel;

class Liite : public QObject {
    Q_OBJECT

public:
    enum LiiteTila { HAETAAN, HAETTU, TALLENNETTAVA, TALLENNETTU, LIITETAAN, LIITETTY };

//    explicit Liite();
//    Liite(const Liite& liite);

    Liite(LiitteetModel* model, const QVariantMap& map);
    Liite(LiitteetModel* model, const QByteArray& ba, const QString& polku = QString(), const QString &rooli = QString());

    ~Liite();

    bool kaytettavissa() const;

    int id() const { return liiteId_;}
    QString nimi() const { return nimi_;}
    QString rooli() const { return rooli_;}
    QString tyyppi() const { return tyyppi_;}
    QString polku() const { return polku_;}

    LiiteTila tila() const;

    QPixmap thumb() const;
    QByteArray* dataPtr() const;

    void liita(bool ocr = false);
    void tallenna(int tositeId);
    void poista();

    void poistaInboxistaLisattyTiedosto(const QString &siirtokansio);


protected:
    void setThumb();

    void liitetty(const QVariant& reply, int lisattyId);
    void tallennettu(const QVariant& reply, int lisattyId);

    void tallennusVirhe(int virhe, const QString selitys);

    void vaihdaTila(Liite::LiiteTila uusiTila);


    LiitteetModel* model_ = nullptr;

    int liiteId_ = 0;
    QString nimi_;
    QString rooli_;
    QString tyyppi_;
    QString polku_;
    LiiteTila tila_ = HAETAAN;
    CacheLiite* cache_ = nullptr;
};

#endif // LIITE_H
