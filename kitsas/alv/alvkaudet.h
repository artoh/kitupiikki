#ifndef ALVKAUDET_H
#define ALVKAUDET_H

#include <QObject>
#include <QDate>

class AlvKausi {
public:
    enum IlmoitusTila { PUUTTUVA, KASITELTY, KASITTELYSSA, ARVIOITU, ERAANTYNYT, EIKAUTTA  };

    AlvKausi();
    AlvKausi(const QVariantMap &map);

    QDate alkupvm() const { return alkupvm_;}
    QDate loppupvm() const { return loppupvm_;}
    QDate erapvm() const { return erapvm_;}
    IlmoitusTila tila() const { return tila_;}
    QString tilaInfo() const;

protected:
    QDate alkupvm_;
    QDate loppupvm_;
    QDate erapvm_;
    IlmoitusTila tila_;
};


class AlvKaudet : public QObject
{
    Q_OBJECT
public:
    enum VarmenneTila { EIKAYTOSSA, OK, VIRHE };

    explicit AlvKaudet(QObject *parent = nullptr);

    void haeKaudet();
    void tyhjenna();

    AlvKausi kausi(const QDate& date) const;
    QList<AlvKausi> kaudet() const;
    bool onko();

    static bool descSort(const AlvKausi &a, const AlvKausi& b);
    bool alvIlmoitusKaytossa() const { return varmenneTila == OK;}

signals:
    void haettu();

protected:
    void saapuu(QVariant* data);

private:
    QList<AlvKausi> kaudet_;

    int haussa_ = 0;
    VarmenneTila varmenneTila = EIKAYTOSSA;

};

#endif // ALVKAUDET_H
