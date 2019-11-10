/*
   Copyright (C) 2019 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "tulomenoapuri.h"
#include "ui_tulomenoapuri.h"
#include "tmrivit.h"

#include "db/kirjanpito.h"
#include "model/tosite.h"
#include "model/tositeviennit.h"
#include "db/tositetyyppimodel.h"
#include "rekisteri/asiakastoimittajadlg.h"

#include <QSortFilterProxyModel>
#include <QDebug>
#include <QJsonDocument>

TuloMenoApuri::TuloMenoApuri(QWidget *parent, Tosite *tosite) :
    ApuriWidget (parent, tosite),
    ui(new Ui::TuloMenoApuri),
    rivit_(new TmRivit(this))
{
    ui->setupUi(this);

    veroFiltteri_ = new QSortFilterProxyModel(this);
    veroFiltteri_->setFilterRole( VerotyyppiModel::KoodiTekstiRooli);
    veroFiltteri_->setSourceModel( kp()->alvTyypit());
    ui->alvCombo->setModel(veroFiltteri_);

    ui->alkuEdit->setNull();
    ui->loppuEdit->setNull();
    ui->erapaivaEdit->setNull();


    ui->tilellaView->setModel( rivit_);
    ui->tilellaView->horizontalHeader()->setSectionResizeMode(TmRivit::TILI, QHeaderView::Stretch);


    connect( ui->tiliEdit, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::tiliMuuttui );

    connect( ui->maaraEdit, &KpEuroEdit::textEdited, this, &TuloMenoApuri::maaraMuuttui);
    connect( ui->verotonEdit, &KpEuroEdit::textEdited, this, &TuloMenoApuri::verotonMuuttui);

    connect( ui->alvSpin, SIGNAL( valueChanged(double) ), this, SLOT( veroprossaMuuttui()) );

    connect( ui->lisaaRiviNappi, &QPushButton::clicked, this, &TuloMenoApuri::lisaaRivi);
    connect( ui->poistaRiviNappi, &QPushButton::clicked, this, &TuloMenoApuri::poistaRivi);

    connect( ui->tilellaView->selectionModel(), &QItemSelectionModel::currentRowChanged , this, &TuloMenoApuri::haeRivi);
    connect( ui->seliteEdit, &QLineEdit::textChanged, this, &TuloMenoApuri::seliteMuuttui);
    connect( ui->alvCombo, &QComboBox::currentTextChanged, this, &TuloMenoApuri::verolajiMuuttui);
    connect( ui->vahennysCheck, &QCheckBox::stateChanged, this, &TuloMenoApuri::alvVahennettavaMuuttui);

    connect( ui->kohdennusCombo, &KohdennusCombo::kohdennusVaihtui, this, &TuloMenoApuri::kohdennusMuuttui);
    connect( ui->merkkauksetCC, &CheckCombo::currentTextChanged, this, &TuloMenoApuri::merkkausMuuttui );

    connect( ui->alkuEdit, &KpDateEdit::dateChanged, this, &TuloMenoApuri::jaksoAlkaaMuuttui);
    connect( ui->loppuEdit, &KpDateEdit::dateChanged, this, &TuloMenoApuri::jaksoLoppuuMuuttui);

    connect( ui->maksutapaCombo, &QComboBox::currentTextChanged, this, &TuloMenoApuri::maksutapaMuuttui);
    connect( ui->vastatiliLine, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::tositteelle);

    connect( ui->viiteEdit, &QLineEdit::textChanged, [this] (const QString& text) {this->tosite()->setData(Tosite::VIITE, text);});
    connect( ui->erapaivaEdit, &KpDateEdit::dateChanged, [this] (const QDate& date) {this->tosite()->setData(Tosite::ERAPVM, date);});
    connect( ui->asiakasToimittaja, &AsiakasToimittajaValinta::valittu, [this] { this->tosite()->setData(Tosite::KUMPPANI, this->ui->asiakasToimittaja->id()); });

    connect( tosite, &Tosite::pvmMuuttui, this, &TuloMenoApuri::haeKohdennukset );
    connect( ui->asiakasToimittaja, &AsiakasToimittajaValinta::valittu, this, &TuloMenoApuri::kumppaniValittu);

    connect( ui->vastatiliLine, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::vastatiliMuuttui);
    connect( tosite, &Tosite::pvmMuuttui, this, &TuloMenoApuri::tositteelle);
}

TuloMenoApuri::~TuloMenoApuri()
{
    delete ui;
}

void TuloMenoApuri::otaFokus()
{
    ui->maksutapaCombo->setFocus();
}

void TuloMenoApuri::tuo(QVariantMap map)
{
    if( qAbs(map.value("summa").toDouble()) > 1e-5) {
        ui->maaraEdit->setValue( map.value("summa").toDouble());
        emit ui->maaraEdit->textEdited( ui->maaraEdit->text() );
    }
    if( !map.value("viite").toString().isEmpty())
        ui->viiteEdit->setText( map.value("viite").toString() );

    if( !map.value("kumppaninimi").toString().isEmpty() || !map.value("kumppaniytunnus").toString().isEmpty())
        ui->asiakasToimittaja->tuonti( map );

    if( map.value("erapvm").isValid())
        ui->erapaivaEdit->setDate( map.value("erapvm").toDate());

    tositteelle();
}

void TuloMenoApuri::teeReset()
{

    // Haetaan tietoja mallista ;)
    bool menoa = tosite()->tyyppi() == TositeTyyppi::MENO ||
                 tosite()->tyyppi() == TositeTyyppi::KULULASKU;

    alusta( menoa );

    ui->viiteEdit->clear();
    ui->erapaivaEdit->clear();
    ui->asiakasToimittaja->clear();
    rivit_->clear();

    QVariantList vientiLista = tosite()->viennit()->viennit().toList();

    for( auto item : tosite()->viennit()->viennit().toList()) {

        // Jos vastakirjaus käsitellään itse
        // Muuten riveille
        TositeVienti vienti( item.toMap() );

        if( vienti.tyyppi() % 100 == TositeVienti::VASTAKIRJAUS) {
            Tili* vastatili = kp()->tilit()->tili( vienti.tili());

            ui->vastatiliLine->valitseTiliNumerolla( vastatili->numero() );
            if( vastatili->eritellaankoTase())
                ui->eraCombo->valitse( vienti.eraId() );

            for(int i=0; i < ui->maksutapaCombo->count(); i++) {
                if( ui->maksutapaCombo->itemData(i).toInt() == vastatili->numero() ) {
                    ui->maksutapaCombo->setCurrentIndex(i);
                    break;
                }
            }
            ui->viiteEdit->setText( vienti.viite());
            ui->erapaivaEdit->setDate( vienti.erapaiva());

            ui->asiakasToimittaja->set( vienti.value("kumppani").toMap().value("id").toInt(),
                                    vienti.value("kumppani").toMap().value("nimi").toString());

            maksutapaMuuttui();

        } else {
            rivit_->lisaa(vienti);
        }

    }
    if( !rivit_->rowCount())
        rivit_->lisaaRivi();


    ui->tilellaView->setVisible( rivit_->rowCount() > 1 );
    ui->poistaRiviNappi->setEnabled( rivit_->rowCount() > 1 );
    ui->tilellaView->selectRow(0);    

    tiliMuuttui();    


}

bool TuloMenoApuri::teeTositteelle()
{        
    qlonglong summa = 0l;

    QVariantList viennit = rivit_->viennit( tosite() );

    for( auto vienti : viennit) {
        QVariantMap map = vienti.toMap();
        summa += qRound64( map.value("kredit",0).toDouble() * 100.0 );
        summa -= qRound64( map.value("debet",0).toDouble() * 100.0 );
    }

    if( summa ) {
        bool menoa = tosite()->tyyppi() == TositeTyyppi::MENO ||
                     tosite()->tyyppi() == TositeTyyppi::KULULASKU;

        QString otsikko = tosite()->otsikko();
        if( otsikko.isEmpty()) {
            otsikko = ui->asiakasToimittaja->nimi();
        }

        TositeVienti vasta;
        if( tosite()->viennit()->rowCount() && tosite()->viennit()->vienti(0).tyyppi() % 100 == TositeVienti::VASTAKIRJAUS)
            vasta.setId( tosite()->viennit()->vienti(0).id() );

        vasta.setTyyppi( (menoa ? TositeVienti::OSTO : TositeVienti::MYYNTI) + TositeVienti::VASTAKIRJAUS );
        vasta.insert("pvm", tosite()->pvm());
        Tili vastatili = kp()->tilit()->tiliNumerolla( ui->vastatiliLine->valittuTilinumero() );
        vasta.insert("tili", vastatili.numero() );

        if( vastatili.eritellaankoTase())
            vasta.setEra( ui->eraCombo->valittuEra() );

        if( summa > 0)
            vasta.setDebet( summa );
        else
            vasta.setKredit( 0 - summa );

        vasta.insert("selite", otsikko);

        if( !ui->viiteEdit->text().isEmpty())
            vasta.setViite( ui->viiteEdit->text());
        if( ui->erapaivaEdit->date().isValid())
            vasta.setErapaiva( ui->erapaivaEdit->date());

        // Asiakas tai toimittaja
        if( ui->asiakasToimittaja->id() > 0)
            vasta.setKumppani( ui->asiakasToimittaja->id() );

        viennit.insert(0, vasta);
    }

    tosite()->viennit()->asetaViennit(viennit);

    if(summa)
        viimeMaksutapa__ = ui->maksutapaCombo->currentText();

    return true;
}

void TuloMenoApuri::lisaaRivi()
{
    ui->tilellaView->setVisible(true);
    ui->tilellaView->selectRow( rivit_->lisaaRivi() );
}

void TuloMenoApuri::poistaRivi()
{    
    rivit_->poistaRivi( rivilla() );

    ui->poistaRiviNappi->setEnabled( rivit_->rowCount() > 1 );
}

void TuloMenoApuri::tiliMuuttui()
{
    if( !rivit_->rowCount())
        return;

    Tili tili = ui->tiliEdit->valittuTili();
    rivi()->setTili(tili.numero());

    bool tasapoisto = tili.onko(TiliLaji::TASAERAPOISTO);
    ui->poistoLabel->setVisible(tasapoisto);
    ui->poistoSpin->setVisible(tasapoisto);

    if( !resetoidaanko()) {

        ui->poistoSpin->setValue( tili.luku("tasaerapoisto") / 12 );

        if( kp()->asetukset()->onko(AsetusModel::ALV) )
        {
            ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( tili.luku("alvlaji"), VerotyyppiModel::KoodiRooli ) );
            ui->alvSpin->setValue( tili.str("alvprosentti").toDouble() );
        }

        emit rivit_->dataChanged( rivit_->index(TmRivit::TILI, rivilla()),
                                  rivit_->index(TmRivit::TILI, rivilla()));

        tositteelle();
    }

}

void TuloMenoApuri::verolajiMuuttui()
{    

    int alvkoodi = ui->alvCombo->currentData( VerotyyppiModel::KoodiRooli ).toInt();
    rivi()->setAlvkoodi(  alvkoodi );

    qDebug() << "alvkoodi " << alvkoodi;

    bool naytaMaara = rivi()->naytaBrutto();
    bool naytaVeroton =  rivi()->naytaNetto();

    ui->maaraLabel->setVisible(naytaMaara);
    ui->maaraEdit->setVisible(naytaMaara);

    ui->verotonLabel->setVisible(naytaVeroton);
    ui->verotonEdit->setVisible(naytaVeroton);

    ui->alvSpin->setVisible( !ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() );
    ui->vahennysCheck->setVisible( rivi()->naytaVahennysvalinta());
    ui->vahennysCheck->setChecked( false );

    if( ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() ) {
        ui->alvSpin->setValue(0.0);
    } else  {
        if( ui->alvSpin->value() == 0.0)
            ui->alvSpin->setValue(24.0);
    }


    tositteelle();
}

void TuloMenoApuri::maaraMuuttui()
{
    qlonglong maara = ui->maaraEdit->asCents();
    rivi()->setBrutto( maara );
    ui->verotonEdit->setCents( rivi()->netto() );

    emit rivit_->dataChanged( rivit_->index(TmRivit::EUROA, rivilla()),
                              rivit_->index(TmRivit::EUROA, rivilla()));

    tositteelle();
}

void TuloMenoApuri::verotonMuuttui()
{
    qlonglong veroton = ui->verotonEdit->asCents();

    rivi()->setNetto( veroton );
    ui->maaraEdit->setCents( rivi()->brutto());

    emit rivit_->dataChanged( rivit_->index(TmRivit::EUROA, rivilla()),
                              rivit_->index(TmRivit::EUROA, rivilla()));


    tositteelle();
}

void TuloMenoApuri::veroprossaMuuttui()
{
    double verokanta = ui->alvSpin->value();
    rivi()->setAlvprosentti( verokanta  );
    ui->maaraEdit->setCents( rivi()->brutto());
    ui->verotonEdit->setCents( rivi()->netto() );
    tositteelle();

}

void TuloMenoApuri::alvVahennettavaMuuttui()
{
    rivi()->setAlvvahennys( !ui->vahennysCheck->isChecked() );
    tositteelle();
}

void TuloMenoApuri::seliteMuuttui()
{
    rivi()->setSelite(  ui->seliteEdit->text());
    tositteelle();
}

void TuloMenoApuri::maksutapaMuuttui()
{
    int maksutapatili = ui->maksutapaCombo->currentData().toInt();

    if( maksutapatili)
        ui->vastatiliLine->valitseTiliNumerolla(maksutapatili);

    ui->vastatiliLabel->setVisible( !maksutapatili  );
    ui->vastatiliLine->setVisible( !maksutapatili );

    vastatiliMuuttui();

    emit tosite()->tarkastaSarja( kp()->tilit()->tiliNumerolla(maksutapatili).onko(TiliLaji::KATEINEN) );
}

void TuloMenoApuri::vastatiliMuuttui()
{
    Tili vastatili = kp()->tilit()->tiliNumerolla( ui->vastatiliLine->valittuTilinumero() );

    bool eritellankotaso = vastatili.eritellaankoTase() && !ui->maksutapaCombo->currentData(Qt::UserRole+1).isValid();

    ui->eraLabel->setVisible( eritellankotaso);
    ui->eraCombo->setVisible( eritellankotaso);
    ui->eraCombo->lataa( vastatili.numero() );
    if( vastatili.eritellaankoTase() ) {
        ui->eraCombo->valitse( ui->maksutapaCombo->currentData(Qt::UserRole+1).toInt() );
    }


    bool laskulle = vastatili.onko(TiliLaji::OSTOVELKA) || vastatili.onko(TiliLaji::MYYNTISAATAVA);
    ui->viiteLabel->setVisible( laskulle );
    ui->viiteEdit->setVisible( laskulle );

    ui->erapaivaLabel->setVisible( laskulle );
    ui->erapaivaEdit->setVisible( laskulle );

    emit tosite()->tarkastaSarja( vastatili.onko(TiliLaji::KATEINEN));

}

void TuloMenoApuri::kohdennusMuuttui()
{
    rivi()->setKohdennus( ui->kohdennusCombo->kohdennus());
    tositteelle();    
}

void TuloMenoApuri::merkkausMuuttui()
{
    rivi()->setMerkkaukset( ui->merkkauksetCC->selectedDatas());
    tositteelle();

}

void TuloMenoApuri::jaksoAlkaaMuuttui()
{
    QDate alkupvm = ui->alkuEdit->date();
    rivi()->setJaksoalkaa( alkupvm);
    ui->loppuEdit->setEnabled( alkupvm.isValid() );
    tositteelle();
}

void TuloMenoApuri::jaksoLoppuuMuuttui()
{
    rivi()->setJaksopaattyy( ui->loppuEdit->date());
    tositteelle();
}

void TuloMenoApuri::poistoAikaMuuttuu()
{
    rivi()->setPoistoaika(  ui->poistoSpin->value() * 12);
    tositteelle();
}

void TuloMenoApuri::haeRivi(const QModelIndex &index)
{
    aloitaResetointi();
    int rivilla = index.row();

    TulomenoRivi* rivi = rivit_->rivi(rivilla);
    int tilinumero = rivi->tilinumero();

    qDebug() << "Hae " << rivi->netto() << " " << rivi->alvkoodi() << " " << rivi->viennit( tosite() );

    if( !tilinumero) {
        if( tosite()->tyyppi() == TositeTyyppi::TULO )
            tilinumero = kp()->tilit()->tiliTyypilla(TiliLaji::LVTULO).numero();
        else
            tilinumero = kp()->tilit()->tiliTyypilla(TiliLaji::MENO).numero();
    }

    ui->tiliEdit->valitseTiliNumerolla( tilinumero );
    tiliMuuttui();

    ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( rivi->alvkoodi(), VerotyyppiModel::KoodiRooli ) );
    ui->maaraEdit->setCents( rivi->brutto() );
    ui->verotonEdit->setCents( rivi->netto());

    ui->vahennysCheck->setChecked( !rivi->alvvahennys() );

    ui->alvSpin->setValue( rivi->alvprosentti() );
    ui->poistoSpin->setValue( rivi->poistoaika() / 12 );

    ui->alkuEdit->setDate( rivi->jaksoalkaa() );
    ui->loppuEdit->setEnabled( rivi->jaksoalkaa().isValid());
    ui->loppuEdit->setDate( rivi->jaksopaattyy());


    ui->seliteEdit->setText( rivi->selite() );

    haeKohdennukset();
    lopetaResetointi();
}

void TuloMenoApuri::haeKohdennukset()
{
    int nykyinenKohdennus = rivit_->rowCount() ? rivi()->kohdennus(  ) : 0 ;
    QVariantList merkatut =  rivit_->rowCount() ?  rivi()->merkkaukset( ) : QVariantList();
    QDate pvm = tosite()->data(Tosite::PVM).toDate();

    ui->kohdennusCombo->valitseKohdennus(nykyinenKohdennus);
    ui->kohdennusCombo->suodataPaivalla(pvm);

    ui->kohdennusLabel->setVisible( ui->kohdennusCombo->count() > 1);
    ui->kohdennusCombo->setVisible( ui->kohdennusCombo->count() > 1);

    ui->merkkauksetCC->haeMerkkaukset(pvm);

    ui->merkkauksetLabel->setVisible( ui->merkkauksetCC->count());
    ui->merkkauksetCC->setVisible( ui->merkkauksetCC->count());

    ui->merkkauksetCC->setSelectedItems( merkatut );
}


void TuloMenoApuri::alusta(bool meno)
{
    menoa_ = meno;

    if(meno) {
        ui->tiliLabel->setText( tr("Meno&tili") );
        ui->tiliEdit->suodataTyypilla("(AP|D).*");
        veroFiltteri_->setFilterRegExp("^(0|2[1-79]|927)");
        ui->toimittajaLabel->setText( tr("Toimittaja"));
        if( tosite()->tyyppi() == TositeTyyppi::KULULASKU )
            ui->toimittajaLabel->setText( tr("Laskuttaja"));
    } else {
        ui->tiliLabel->setText( tr("Tulo&tili"));
        ui->tiliEdit->suodataTyypilla("(AP|C).*");
        veroFiltteri_->setFilterRegExp("^(0|1[1-79])");
        ui->toimittajaLabel->setText( tr("Asiakas"));
    }

    // Alustetaan maksutapacombo

    ui->maksutapaCombo->clear();
    for(QVariant mtapa : QJsonDocument::fromJson( kp()->asetukset()->asetus( meno ? "maksutavat-" : "maksutavat+" ).toUtf8() ).toVariant().toList()  ) {
        QVariantMap map( mtapa.toMap());
        KieliKentta kk( map );
        ui->maksutapaCombo->addItem( QIcon( map.contains("KUVA") ? ":/pic/" + map.value("KUVA").toString() + ".png" : ":/pic/tyhja.png"),
                                     kk.teksti(),
                                     map.value("TILI").toInt());
        if( map.contains("ERA"))
            ui->maksutapaCombo->setItemData( ui->maksutapaCombo->count()-1, map.value("ERA").toInt(), Qt::UserRole + 1 );
    }
    ui->maksutapaCombo->addItem( QIcon(":/pic/tyhja.png"), tr("Kaikki vastatilit"), 0 );        

    if( viimeMaksutapa__.length())
        ui->maksutapaCombo->setCurrentIndex( ui->maksutapaCombo->findText( viimeMaksutapa__ ));
    if( ui->maksutapaCombo->currentIndex() < 0)
        ui->maksutapaCombo->setCurrentIndex(0);


    ui->vastatiliLine->suodataTyypilla("[AB]");

    bool alv = kp()->asetukset()->onko( AsetusModel::ALV );
    ui->alvLabel->setVisible(alv);
    ui->alvCombo->setVisible(alv);

    ui->erapaivaEdit->setDateRange(QDate(), QDate());
    ui->loppuEdit->setDateRange( kp()->tilitpaatetty().addDays(1), QDate() );

    ui->asiakasToimittaja->alusta();
}

int TuloMenoApuri::rivilla() const
{

    if( ui->tilellaView->currentIndex().row() < 0 )
        return 0;
    return ui->tilellaView->currentIndex().row();

}

TulomenoRivi *TuloMenoApuri::rivi()
{    
    return rivit_->rivi(rivilla());
}

void TuloMenoApuri::kumppaniValittu(int kumppaniId)
{
    KpKysely *kysely = kpk(QString("/kumppanit/%1").arg(kumppaniId));
    connect(kysely, &KpKysely::vastaus, this, &TuloMenoApuri::kumppaniTiedot);
    kysely->kysy();
}

void TuloMenoApuri::kumppaniTiedot(QVariant *data)
{
    QVariantMap map = data->toMap();

    if(menoa_  ) {
        if( map.contains("menotili"))
            ui->tiliEdit->valitseTiliNumerolla( map.value("menotili").toInt() );
    } else {
        if( map.contains("tulotili"))
            ui->tiliEdit->valitseTiliNumerolla( map.value("tulotili").toInt());
    }

    if( tosite()->tyyppi() == TositeTyyppi::KULULASKU && tosite()->otsikko().isEmpty() )
        tosite()->asetaOtsikko( tr("Kululasku %1").arg(map.value("nimi").toString()) );
}

QString TuloMenoApuri::viimeMaksutapa__ = QString();
