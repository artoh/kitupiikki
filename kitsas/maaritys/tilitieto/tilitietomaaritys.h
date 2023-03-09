#ifndef TILITIETO_TILITIETOMAARITYS_H
#define TILITIETO_TILITIETOMAARITYS_H

#include "../maarityswidget.h"

class QHBoxLayout;

namespace Ui {
  class TilitietoMaaritys;
}


namespace Tilitieto {

class TilitietoPalvelu;
class UusiYhteysDialog;

class TilitietoMaaritys : public MaaritysWidget
{
    Q_OBJECT
public:
    TilitietoMaaritys();
    ~TilitietoMaaritys();

    bool naytetaankoTallennus() override { return false; }

    bool nollaa() override;

private:
    void paivitaYhteydet();

    void naytaTosite(const QModelIndex& index);
    void haeTiliTapahtumat(int yhteysIndeksi);

    void uusiValtuutus(int pankki);

    Ui::TilitietoMaaritys *ui;
    TilitietoPalvelu* palvelu_;
    UusiYhteysDialog* dlg_;    

    QHBoxLayout* yhteysLeiska_;
    QWidget* yhteysWidget_ = nullptr;
};

} // namespace Tilitieto

#endif // TILITIETO_TILITIETOMAARITYS_H
