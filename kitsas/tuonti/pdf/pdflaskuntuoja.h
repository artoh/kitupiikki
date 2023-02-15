#ifndef TUONTI_PDFLASKUNTUOJA_H
#define TUONTI_PDFLASKUNTUOJA_H

#include <QStringList>
#include <QDate>

#include "model/euro.h"
#include "pdftiedosto.h"

#include "tuontiapuinfo.h"
#include <QRegularExpression>

namespace Tuonti {

class PdfLaskunTuoja
{
public:
    PdfLaskunTuoja(PdfTiedosto* tiedosto, TuontiApuInfo info);

    const QVariantMap tuo();

protected:
    enum Muoto { Pvm, Viitenumero, Numero, Euromaara};
    enum { MENO = 100, TULO = 200 };

    bool haeLomakkeesta();
    void haeOtsakkeilla();
    void haeTekstista();

    void haeKumppani();
    void haeEuro();

    QVariant hae(const QStringList& tekstit, Muoto muoto);
    QVariant tarkastaMuoto(PdfPala *pala, Muoto muoto);
    QStringList rivitPaloista(QList<PdfPala*> palat, const QStringList& siivottavat = QStringList());
    QString palatTekstina(QList<PdfPala*> palat);

    void lisaaIban(const QString& iban);
    bool lisaaKumppani(const QStringList& rivit);

    QVariantMap map() const;

protected:
    int tyyppi_ = 100;
    QStringList iban_;
    QString kumppaniNimi_;
    QString kumppaniOsoite_;
    QString kumppaniPostinumero_;
    QString kumppaniKaupunki_;
    QString kumppaniYtunnus_;
    QString laskunumero_;
    QString viite_;
    QDate erapvm_;
    QDate toimituspvm_;
    QDate laskupvm_;
    Euro euro_;

    PdfTiedosto* tiedosto_;
    TuontiApuInfo info_;

    static QRegularExpression tyhjaRe__;
    static QRegularExpression ibanRe__;
    static QRegularExpression postiOsoiteRe__;
    static QRegularExpression pvmRe__;
    static QRegularExpression euronSiivous__;
    static QRegularExpression viiteRe__;
    static QRegularExpression numeroRe__;
    static QRegularExpression ytunnusRe__;
    static QRegularExpression rahaRe__;
};

} // namespace Tuonti

#endif // TUONTI_PDFLASKUNTUOJA_H
