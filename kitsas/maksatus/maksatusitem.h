#ifndef MAKSATUSITEM_H
#define MAKSATUSITEM_H

#include <QString>
#include <QDate>
#include "model/euro.h"
#include <QVariantMap>

class MaksatusItem
{
public:

    enum MaksatusTila {
        UNKNOWN,
        PENDING,
        PENDING_CONFIRMATION,
        INPROGRESS,
        PAID,
        REJECTED,
        SCHEDULED,
        ERROR
    };

    MaksatusItem();
    MaksatusItem(const QVariant& variant);

    void lataa(const QVariant& variant);

    QString id() const { return id_;}
    MaksatusTila tila() const { return tila_;}
    Euro euro() const { return euro_;}
    QDate pvm() const { return pvm_;}
    QString viite() const {return viite_;}
    QString viesti() const {return viesti_;}

    QString viiteTaiViesti() const;

    static MaksatusTila strToTila(const QString& tilaStr);

private:
    QString id_;
    MaksatusTila tila_;
    Euro euro_;
    QDate pvm_;
    QString viite_;
    QString viesti_;
};


#endif // MAKSATUSITEM_H
