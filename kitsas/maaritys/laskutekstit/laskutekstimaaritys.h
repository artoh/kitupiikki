#ifndef LASKUTEKSTIMAARITYS_H
#define LASKUTEKSTIMAARITYS_H

#include "../maarityswidget.h"


namespace Ui {
    class LaskuTekstitMaaritys;
}
class LaskuTekstitModel;
class EmailKentanKorostin;

class LaskuTekstiMaaritys : public MaaritysWidget
{
    Q_OBJECT
public:
    LaskuTekstiMaaritys();
    ~LaskuTekstiMaaritys();

    bool nollaa() override;
    bool tallenna() override;
    bool onkoMuokattu() override;

    QString ohjesivu() override { return "asetukset/laskutekstit";}

    void valitseTeksti();
    void lataaTekstit();

    void otsikkoMuuttunut();
    void sisaltoMuuttunut();

protected:
    QString kieli() const;

protected:
    Ui::LaskuTekstitMaaritys* ui_;
    LaskuTekstitModel* model_;
    EmailKentanKorostin* otsikkoKorostin_;
    EmailKentanKorostin* sisaltoKorostin_;
};

#endif // LASKUTEKSTIMAARITYS_H
