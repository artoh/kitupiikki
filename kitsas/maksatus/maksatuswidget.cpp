#include "maksatuswidget.h"

#include "ui_maksatus.h"
#include "uusimaksudialog.h"
#include "maksutmodel.h"
#include "model/tositeviennit.h"
#include "model/tositevienti.h"
#include "db/kirjanpito.h"

#include "model/tosite.h"

MaksatusWidget::MaksatusWidget(Tosite* tosite, QWidget *parent)
    : QWidget{parent}, ui{new Ui::Maksatus}, model_{new MaksutModel(this)}, tosite_{tosite}

{
    ui->setupUi(this);

    connect( ui->uusiNappi, &QPushButton::clicked, this, &MaksatusWidget::uusiMaksu);

    ui->maksutView->setModel(model_);
    ui->maksutView->horizontalHeader()->setSectionResizeMode(MaksutModel::REF_COLUMN, QHeaderView::Stretch);

    connect( ui->maksutView, &QTableView::clicked, this, &MaksatusWidget::itemClicked);

    connect( ui->poistaNappi, &QPushButton::clicked, this, &MaksatusWidget::rejectPayment);
    connect( model_, &MaksutModel::modelReset, ui->maksutView, &QTableView::resizeColumnsToContents);
}

MaksatusWidget::~MaksatusWidget()
{
    delete ui;
}

void MaksatusWidget::reload()
{
    ui->poistaNappi->setEnabled(false);
    model_->load( tosite_->id() );
}

void MaksatusWidget::uusiMaksu()
{
    UusiMaksuDialog dlg(this);
    const QString& partnerName = tosite_->kumppaninimi();
    const QString& iban = tosite_->lasku().iban();
    const QString& viite = tosite_->viite();
    const QDate& erapvm = tosite_->erapvm();
    const QString& laskuNumero = tosite_->laskuNumero();

    Euro summa = Euro::Zero;

    if( tosite_->viennit()->rowCount()) {
        const TositeVienti& vasta = tosite_->viennit()->vienti(0);
        if( vasta.tyyppi() % 100 == TositeVienti::VASTAKIRJAUS) {
            summa = vasta.kreditEuro();
        }
    }

    dlg.init( partnerName, iban, viite, summa, erapvm, laskuNumero );
    if(dlg.exec() == QDialog::Accepted) {
        const QString url = QString("/maksut/%1").arg(tosite_->id());
        KpKysely* kysely = kpk(url, KpKysely::POST);
        connect( kysely, &KpKysely::vastaus, this, &MaksatusWidget::reload);
        kysely->kysy(dlg.data());
    }
}

void MaksatusWidget::itemClicked(const QModelIndex &item)
{
    ui->poistaNappi->setEnabled( item.data(MaksutModel::RejectableRole).toBool() );
}

void MaksatusWidget::rejectPayment()
{
    const QModelIndex& current = ui->maksutView->currentIndex();
    const QString id = current.data(MaksutModel::IdRole).toString();
    if( id.isEmpty()) return;

    const QString url = QString("/maksut/%1").arg(id);
    KpKysely* kysely = kpk(url, KpKysely::DELETE);
    connect( kysely, &KpKysely::vastaus, this, &MaksatusWidget::reload);
    kysely->kysy();
}
