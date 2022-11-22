#include "uusitoimistodialog.h"
#include "ui_uusitoimistodialog.h"

#include "grouptreemodel.h"
#include "groupdata.h"

#include "validator/ytunnusvalidator.h"

UusiToimistoDialog::UusiToimistoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UusiToimistoDialog)
{
    ui->setupUi(this);

    ui->ytunnusEdit->setValidator(new YTunnusValidator());

}

UusiToimistoDialog::~UusiToimistoDialog()
{
    delete ui;
}

void UusiToimistoDialog::newOffice(GroupTreeModel *tree, GroupData *data)
{
    for(auto item : data->officeTypes()) {
        const QVariantMap map = item.toMap();
        ui->tyyppiCombo->addItem(QIcon( map.value("icon").toString()),
                                 map.value("label").toString(),
                                 map.value("officetype").toString());
    }


    if( exec() == QDialog::Accepted) {
        QVariantMap payload;
        payload.insert("name", ui->nimiEdit->text());
        payload.insert("businessid", ui->ytunnusEdit->text());
        payload.insert("officetype", ui->tyyppiCombo->currentData(Qt::UserRole).toString());

        tree->addGroup( data->id(), payload );
    }

}
