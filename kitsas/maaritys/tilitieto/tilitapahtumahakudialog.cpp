#include "tilitapahtumahakudialog.h"
#include "ui_tilitapahtumahaku.h"
#include "db/kirjanpito.h"
#include "yhteys.h"
#include "yhdistettytili.h"
#include "tilitietopalvelu.h"
#include "laskutus/iban.h"
#include <QPushButton>
#include "tools/kpdateedit.h"

namespace Tilitieto {

TiliTapahtumaHakuDialog::TiliTapahtumaHakuDialog(TilitietoPalvelu *palvelu, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TiliTapahtumaHakuDialog),
    palvelu_(palvelu)
{
    ui->setupUi(this);

    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("asetukset/tilitiedot"); } );
}

TiliTapahtumaHakuDialog::~TiliTapahtumaHakuDialog()
{
    delete ui;
}

void TiliTapahtumaHakuDialog::accept()
{
    const Iban iban( ui->tiliCombo->currentData().toString() );

    palvelu_->haeTapahtumat(iban, ui->alkuPvm->date(), ui->loppuPvm->date());

    QDialog::accept();
}

void TiliTapahtumaHakuDialog::nayta(int yhteysIndeksi)
{
    Yhteys yhteys = palvelu_->yhteys(yhteysIndeksi);
    const QIcon pankkikuva = yhteys.pankki().icon();
    for(int i=0; i < yhteys.tileja(); i++) {
        const Iban iban = yhteys.tili(i).iban() ;
        Tili kirjanpidossa = kp()->tilit()->tiliIbanilla(iban.valeitta());
        if( kirjanpidossa.onkoValidi()) {
            ui->tiliCombo->addItem(pankkikuva, kirjanpidossa.nimiNumeroIban() , iban.valeitta());
        }
    }
    ui->tiliCombo->model()->sort(0);

    QDate maxDate = QDate().currentDate().addDays(-1);
    QDate minDate = QDate().currentDate().addDays(-90);

    ui->alkuPvm->setDateRange(minDate, maxDate);
    ui->loppuPvm->setDateRange(minDate, maxDate);

    ui->alkuPvm->setDate( maxDate );
    ui->loppuPvm->setDate( maxDate );

    connect( ui->alkuPvm, &KpDateEdit::dateChanged, this, &TiliTapahtumaHakuDialog::tarkastaPvmVali);
    connect( ui->loppuPvm, &KpDateEdit::dateChanged, this, &TiliTapahtumaHakuDialog::tarkastaPvmVali);

    exec();
}

void TiliTapahtumaHakuDialog::tarkastaPvmVali()
{
    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled(
                ui->alkuPvm->date() <= ui->loppuPvm->date() );
}

} // namespace Tilitieto
