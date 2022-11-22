#ifndef UUSITOIMISTOONSIVU_H
#define UUSITOIMISTOONSIVU_H

#include <QWizardPage>
#include "uusikirjanpito/uusivelho.h"

namespace  Ui {
    class UusiToimistoon;
}
class GroupData;


class UusiToimistoonSivu : public QWizardPage
{
    Q_OBJECT
public:
    UusiToimistoonSivu(UusiVelho* velho);
    void yhdista(GroupData* toimisto);

protected:
    void initializePage() override;
    bool validatePage() override;

    Ui::UusiToimistoon *ui;
    GroupData *groupData;

    UusiVelho* velhoni = nullptr;
};

#endif // UUSITOIMISTOONSIVU_H
