#ifndef MAKSATUSITEM_H
#define MAKSATUSITEM_H

#include <QString>
#include <QDate>
#include "laskutus/iban.h"
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

    enum IbanEvaluation {
        EXISTING,
        ADDED,
        CHANGED,
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
    Iban iban() const { return iban_;}
    IbanEvaluation evaluation() const { return evaluation_;}

    QString viiteTaiViesti() const;

    static MaksatusTila strToTila(const QString& tilaStr);
    static IbanEvaluation strToEvaluation(const QString& evaluationStr);

private:
    QString id_;
    MaksatusTila tila_;
    Euro euro_;
    QDate pvm_;
    QString viite_;
    QString viesti_;
    Iban iban_;
    IbanEvaluation evaluation_;
};


#endif // MAKSATUSITEM_H
