#include "uusitoimistodialog.h"
#include "ui_uusitoimistodialog.h"

#include "grouptreemodel.h"
#include "groupdata.h"

UusiToimistoDialog::UusiToimistoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UusiToimistoDialog)
{
    ui->setupUi(this);
}

UusiToimistoDialog::~UusiToimistoDialog()
{
    delete ui;
}

void UusiToimistoDialog::newOffice(GroupTreeModel *tree, GroupData *data)
{

    if( exec() == QDialog::Accepted) {
        QVariantMap payload;
        payload.insert("name", ui->nimiEdit->text());
        payload.insert("businessid", ui->ytunnusEdit->text());
        payload.insert("officetype", "OFFICE");

        tree->addGroup( data->id(), payload );
    }

}
