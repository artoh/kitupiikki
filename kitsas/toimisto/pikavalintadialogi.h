#ifndef PIKAVALINTADIALOGI_H
#define PIKAVALINTADIALOGI_H

#include <QDialog>

namespace Ui {
class PikavalintaDialogi;
class OikeusWidget;
class ToimistoOikeudet;
}

class GroupData;

class PikavalintaDialogi : public QDialog
{
    Q_OBJECT

public:
    PikavalintaDialogi(QWidget *parent, GroupData *groupData);
    ~PikavalintaDialogi();

    void accept() override;
    void reject() override;

private:
    void lisaa();
    void paivitaMuokkaus();
    void poista();
    void lataa();
    void poistaPikavalinnat();

    void tallennettu();

private:       
    Ui::PikavalintaDialogi *ui;
    Ui::OikeusWidget *oikeusUi;
    Ui::ToimistoOikeudet *toimistoUi;

    GroupData* group_ = nullptr;
};

#endif // PIKAVALINTADIALOGI_H
