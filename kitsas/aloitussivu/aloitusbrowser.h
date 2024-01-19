#ifndef ALOITUSBROWSER_H
#define ALOITUSBROWSER_H

#include <QTextBrowser>
#include "aloitusinfot.h"
#include "model/euro.h"
#include "qdatetime.h"

class Tili;

class AloitusBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    AloitusBrowser(QWidget* parent = nullptr);

    void paivita();

    void haSaldot(const QDate& saldoPvm);
    void asetaTilioimatta(int tilioimatta);

protected:
    void naytaPaivitetty();
    void paivitaAvattu();

    void vinkkaa(const QString& luokka, const QString& otsikko, const QString& teksti,
                 const QString& linkki = QString(), const QString kuva = QString(), const QString ohjelinkki = QString());

    void paivitaVinkit();
    QString eiOikeuttaUrputus() const;

    void paivitaTestiVinkki();
    void paivitaTiliointiVinkki();

    void paivitaPankkiyhteysVinkki();
    void paivitaVarmuuskopioVinkki();
    void paivitaVerotonValitus();
    void paivitaPaivitysVinkki();
    void paivitaAloitusVinkit();
    void paivitaAlvVinkki();
    void paivitaTilikausiVinkki();
    void paivitaTilinpaatosVinkki();
    void paivitaPassiivinenVinkki();
    void paivitaExtraVinkki();

    void paivitaSaldot();
    void saldotSaapuu(QVariant* data);
    QString saldoTaulu();

    QString memo() const;

    void naytaTervetuloa();    

private:
    class SaldoTieto {
    public:
        SaldoTieto();
        SaldoTieto(const QString& tilinumero, const QString& saldo);

        Tili *tili() const;
        Euro saldo() const;
    private:
        int tilinumero_;
        Euro saldo_;

    };

private:
    AloitusInfot vinkit_;

    int tilioimatta_ = 0;
    int saldollisia_ = 0;

    QDate saldoPvm_;

    QList<SaldoTieto> saldot_;
};

#endif // ALOITUSBROWSER_H
