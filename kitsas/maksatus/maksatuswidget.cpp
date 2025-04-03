#include "maksatuswidget.h"

#include "ui_maksatus.h"
#include "uusimaksudialog.h"
#include "maksutmodel.h"
#include "model/tositeviennit.h"
#include "model/tositevienti.h"

#include "model/tosite.h"

MaksatusWidget::MaksatusWidget(Tosite* tosite, QWidget *parent)
    : QWidget{parent}, ui{new Ui::Maksatus}, model_{new MaksutModel(this)}, tosite_{tosite}

{
    ui->setupUi(this);

    connect( ui->uusiNappi, &QPushButton::clicked, this, &MaksatusWidget::uusiMaksu);

    ui->maksutView->setModel(model_);
    ui->maksutView->horizontalHeader()->setSectionResizeMode(MaksutModel::REF_COLUMN, QHeaderView::Stretch);
}

MaksatusWidget::~MaksatusWidget()
{
    delete ui;
}

void reload()
{

}

void MaksatusWidget::uusiMaksu()
{
    UusiMaksuDialog dlg(this);
    const QString& partnerName = tosite_->kumppaninimi();
    const QString& iban = tosite_->lasku().iban();
    const QString& viite = tosite_->viite();
    const QDate& erapvm = tosite_->erapvm();

    Euro summa = Euro::Zero;

    if( tosite_->viennit()->rowCount()) {
        const TositeVienti& vasta = tosite_->viennit()->vienti(0);
        if( vasta.tyyppi() % 100 == TositeVienti::VASTAKIRJAUS) {
            summa = vasta.kreditEuro();
        }
    }

    dlg.init( partnerName, iban, viite, summa, erapvm );
    dlg.exec();
}
