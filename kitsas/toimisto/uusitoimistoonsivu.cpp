#include "uusitoimistoonsivu.h"
#include "ui_uusitoimistoon.h"

#include "toimisto/groupdata.h"

UusiToimistoonSivu::UusiToimistoonSivu(UusiVelho *velho) :
    QWizardPage{velho},
    ui{new Ui::UusiToimistoon},
    velhoni{velho}
{
    ui->setupUi(this);

    setTitle("Uusi kirjanpito");
}

void UusiToimistoonSivu::yhdista(GroupData *toimisto)
{
    groupData = toimisto;
    setField("pilveen", true);
}

void UusiToimistoonSivu::initializePage()
{
    ui->toimistoLabel->setText( groupData->officeName() );
    ui->ryhmaLabel->setText( groupData->name() );

    for(const auto& item : groupData->products()) {
        const QVariantMap map = item.toMap();
        QListWidgetItem *lItem = new QListWidgetItem( map.value("name").toString(), ui->tuoteList);
        lItem->setData(Qt::UserRole, map.value("id").toString());
    }
    ui->tuoteList->setCurrentRow(0);

}

bool UusiToimistoonSivu::validatePage()
{
    velhoni->tuote_ = ui->tuoteList->currentItem()->data(Qt::UserRole).toInt();
    velhoni->harjoitus_ = ui->harjoitusCheck->isChecked();


    if( velhoni->tuote_ >= 200)
        return true;
    else
        return false;
}
