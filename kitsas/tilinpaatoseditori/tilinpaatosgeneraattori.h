#ifndef TILINPAATOSGENERAATTORI_H
#define TILINPAATOSGENERAATTORI_H

#include <QObject>
#include "db/tilikausi.h"

class TilinpaatosGeneraattori : public QObject
{
    Q_OBJECT
public:
    explicit TilinpaatosGeneraattori(const Tilikausi& tilikausi, QObject *parent = nullptr);
    void generoi();

    QString html() const;
    QStringList raportit() const;

    enum SaldoTyyppi { Nykyinen, NykyinenAlku, EdellinenAlku};
protected:
    void tilaaSaldot();
    void tilaaSaldo(SaldoTyyppi tyyppi);
    void saldotSaapuu(SaldoTyyppi tyyppi, QVariant* data);
    void jatka();

    void tekstiRivi(const QString& rivi);

    bool ehto(const QString ehto);
    void atRivi(const QString& rivi);
    QString henkilostoTaulukko(const QString& teksti);
    Euro laskenta(const QString& kaava);

signals:
    void valmis();


protected:
    class TilinSaldot {
    public:
        explicit TilinSaldot();

        void setSaldo(TilinpaatosGeneraattori::SaldoTyyppi tyyppi, Euro saldo);
        Euro saldo(const QString& tyyppi) const;

    protected:
        Euro saldo_;
        Euro alkusaldo_;
        Euro edellinenAlku_;
    };

protected:
    Tilikausi tilikausi_;

    QMap<QString, TilinSaldot> saldot_;
    QMap<QString, Euro> muuttujat_;
    int tilattuja_ = 0;

    QStringList raportit_;
    QString html_;

    static QRegularExpression tunnisteRe__;
    static QRegularExpression raporttiRe__;
    static QRegularExpression kaavaRe__;
    static QRegularExpression ehtoRe__;
};

#endif // TILINPAATOSGENERAATTORI_H
