#include "laskutekstimaaritys.h"

#include "ui_laskutekstitmaaritys.h"
#include "laskutekstitmodel.h"
#include "emailkentankorostin.h"

#include <QModelIndex>
#include <QItemSelectionModel>

LaskuTekstiMaaritys::LaskuTekstiMaaritys() :
    ui_(new Ui::LaskuTekstitMaaritys), model_(new LaskuTekstitModel(this))
{
    ui_->setupUi(this);
    ui_->listView->setModel(model_);

    ui_->kieliTab->addTab(QIcon(":/liput/fi.png"), tr("suomi"));
    ui_->kieliTab->addTab(QIcon(":/liput/sv.png"), tr("ruotsi"));
    ui_->kieliTab->addTab(QIcon(":/liput/en.png"), tr("englanti"));

    ui_->otsikkoEdit->setFixedHeight( ui_->otsikkoLabel->height() * 3 / 2 );

    connect( ui_->listView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &LaskuTekstiMaaritys::valitseTeksti);
    connect( ui_->kieliTab, &QTabBar::currentChanged, this, &LaskuTekstiMaaritys::lataaTekstit);

    connect( ui_->otsikkoEdit, &QPlainTextEdit::textChanged, this, &LaskuTekstiMaaritys::otsikkoMuuttunut);
    connect( ui_->sisaltoEdit, &QPlainTextEdit::textChanged, this, &LaskuTekstiMaaritys::sisaltoMuuttunut);

    new EmailKentanKorostin( ui_->otsikkoEdit->document());
    new EmailKentanKorostin( ui_->sisaltoEdit->document());
}

LaskuTekstiMaaritys::~LaskuTekstiMaaritys()
{
    delete ui_;
}

bool LaskuTekstiMaaritys::nollaa()
{
    model_->nollaa();
    ui_->listView->selectionModel()->setCurrentIndex( model_->index(0), QItemSelectionModel::Select);
    return true;
}

bool LaskuTekstiMaaritys::tallenna()
{
    model_->tallenna();
    return true;
}

bool LaskuTekstiMaaritys::onkoMuokattu()
{
    return model_->muokattu();
}

void LaskuTekstiMaaritys::valitseTeksti()
{
    int rivi = ui_->listView->currentIndex().row();
    if( rivi > -1) {
        ui_->kuvausLabel->setText( model_->kuvaus(rivi) );
        ui_->otsikkoLabel->setVisible( model_->onkoOtsikkoa(rivi) );
        ui_->otsikkoEdit->setVisible( model_->onkoOtsikkoa(rivi));
        lataaTekstit();
    }
}

void LaskuTekstiMaaritys::lataaTekstit()
{
    int rivi = ui_->listView->currentIndex().row();
    const QString nKieli = kieli();
    if( rivi > -1) {
        ui_->otsikkoEdit->setPlainText( model_->otsikko(rivi, nKieli) );
        ui_->sisaltoEdit->setPlainText( model_->sisalto(rivi, nKieli) );
    }
}

void LaskuTekstiMaaritys::otsikkoMuuttunut()
{
    int rivi = ui_->listView->currentIndex().row();
    if( rivi > -1) {
        model_->asetaOtsikko(rivi, kieli(), ui_->otsikkoEdit->toPlainText());
        emit tallennaKaytossa( onkoMuokattu() );
    }
}

void LaskuTekstiMaaritys::sisaltoMuuttunut()
{
    int rivi = ui_->listView->currentIndex().row();
    if( rivi > -1) {
        model_->asetaSisalto(rivi, kieli(), ui_->sisaltoEdit->toPlainText());
        emit tallennaKaytossa( onkoMuokattu() );
    }
}

QString LaskuTekstiMaaritys::kieli() const
{
    int tab = ui_->kieliTab->currentIndex();

    if( tab == 1)
        return "sv";
    else if( tab == 2)
        return "en";
    else
        return "fi";
}
