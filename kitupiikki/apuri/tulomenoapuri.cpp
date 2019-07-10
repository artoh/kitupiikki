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
#include "kirjaus/kohdennusproxymodel.h"

#include <QSortFilterProxyModel>


TuloMenoApuri::TuloMenoApuri(QWidget *parent, Tosite *tosite) :
    ApuriWidget (parent, tosite),
    ui(new Ui::TuloMenoApuri),
    rivit_(new TmRivit)
{
    ui->setupUi(this);

    veroFiltteri_ = new QSortFilterProxyModel(this);
    veroFiltteri_->setFilterRole( VerotyyppiModel::KoodiTekstiRooli);
    veroFiltteri_->setSourceModel( kp()->alvTyypit());
    ui->alvCombo->setModel(veroFiltteri_);

    ui->kohdennusCombo->setModel(new KohdennusProxyModel(this));
    ui->kohdennusCombo->setModelColumn( KohdennusModel::NIMI);
    ui->kohdennusCombo->setCurrentIndex( ui->kohdennusCombo->findData(0, KohdennusModel::IdRooli ));


    ui->alkuEdit->setNull();
    ui->loppuEdit->setNull();
    ui->erapaivaEdit->setNull();

    ui->vastatiliEdit->suodataTyypilla("[AB].*");

    ui->tilellaView->setModel( rivit_);
    ui->tilellaView->horizontalHeader()->setSectionResizeMode(TmRivit::TILI, QHeaderView::Stretch);


    connect( ui->tiliEdit, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::tiliMuuttui );
    connect( ui->maaraEdit, &KpEuroEdit::textChanged, this, &TuloMenoApuri::maaraMuuttui);
    connect( ui->lisaaRiviNappi, &QPushButton::clicked, this, &TuloMenoApuri::lisaaRivi);
    connect( ui->tilellaView->selectionModel(), &QItemSelectionModel::currentRowChanged , this, &TuloMenoApuri::haeRivi);
    connect( ui->seliteEdit, &QLineEdit::textChanged, this, &TuloMenoApuri::seliteMuuttui);

    connect( ui->maksutapaCombo, &QComboBox::currentTextChanged, this, &TuloMenoApuri::maksutapaMuuttui);
    connect( ui->vastatiliEdit, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::tositteelle);
}

TuloMenoApuri::~TuloMenoApuri()
{
    delete ui;
}

void TuloMenoApuri::otaFokus()
{
    ui->tiliEdit->setFocus();
}

void TuloMenoApuri::teeReset()
{
    // Haetaan tietoja mallista ;)
    bool menoa = tosite()->data(Tosite::TYYPPI).toInt() == TositeTyyppi::MENO;
    alusta( menoa );

    // Haetaan rivien tiedot

    QVariantList vientiLista = tosite()->viennit()->viennit().toList();
    if( vientiLista.count())
    {
        ui->vastatiliEdit->valitseTiliNumerolla( vientiLista.at(0).toMap().value("tili").toInt() );
    }
    rivit_->clear();
    int rivi = 0;
    for(int i=1; i < vientiLista.count(); i++)
    {
        QVariantMap map = vientiLista.at(i).toMap();
        rivit_->lisaaRivi();
        rivit_->setTili( rivi, *kp()->tilit()->tiliNumerolla( map.value("tili").toInt()  ) );
        rivit_->setSelite(rivi, map.value("selite").toString());
        qlonglong maara = menoa ? qRound( map.value("debet").toDouble() * 100.0 ) :
                                  qRound( map.value("kredit").toDouble() * 100.0 );
        rivit_->setMaara(rivi, maara);
        rivi++;
    }

    ui->tilellaView->setVisible( rivi > 1 );
    ui->poistaRiviNappi->setEnabled( rivi > 1 );
    if( !rivi )
    {
        rivit_->lisaaRivi();
        ui->tiliEdit->clear();
        ui->maaraEdit->setCents(0);
        ui->seliteEdit->clear();
    }
    else if( ui->tilellaView->selectionModel()->currentIndex().row() == 0)
        haeRivi( rivit_->index(0,0) );
    ui->tilellaView->selectRow(0);

}

bool TuloMenoApuri::teeTositteelle()
{
    // Lasketaan ensin summa
    qlonglong summa = 0l;
    int riveja = rivit_->rowCount();

    for(int i=0; i < riveja; i++)
        summa += rivit_->maara(i);

    bool menoa = tosite()->data(Tosite::TYYPPI).toInt() == TositeTyyppi::MENO;
    QDate pvm = tosite()->data(Tosite::PVM).toDate();
    QString otsikko = tosite()->data(Tosite::OTSIKKO).toString();

    QVariantList viennit;

    if( summa ) {
        QVariantMap vasta;
        vasta.insert("pvm", pvm);
        vasta.insert("tili", ui->vastatiliEdit->valittuTilinumero() );
        if( menoa )
            vasta.insert("kredit", summa / 100.0);
        else
            vasta.insert("debet", summa / 100.0);
        vasta.insert("selite", otsikko);
        viennit.append(vasta);
    }

    // TODO: Kaikki verolajit yms.

    for(int i=0; i < riveja; i++) {
        QVariantMap vienti;
        vienti.insert("pvm", pvm);
        vienti.insert("tili", rivit_->tili(i).numero());
        if( menoa )
            vienti.insert("debet", rivit_->maara(i) / 100.0);
        else
           vienti.insert("kredit", rivit_->maara(i) / 100.0);
        QString selite = rivit_->selite(i);
        if( selite.isEmpty())
            vienti.insert("selite", otsikko);
        else
            vienti.insert("selite", selite);

        viennit.append(vienti);
    }
    tosite()->viennit()->asetaViennit(viennit);

    return true;
}

void TuloMenoApuri::lisaaRivi()
{
    ui->tilellaView->setVisible(true);
    ui->tilellaView->selectRow( rivit_->lisaaRivi() );
}

void TuloMenoApuri::tiliMuuttui()
{
    Tili tili = ui->tiliEdit->valittuTili();
    rivit_->setTili( rivilla(), tili );

    bool tasapoisto = tili.onko(TiliLaji::TASAERAPOISTO);
    ui->poistoLabel->setVisible(tasapoisto);
    ui->poistoSpin->setVisible(tasapoisto);

    tositteelle();
    // TODO: Vero-oletusten hakeminen
}

void TuloMenoApuri::maaraMuuttui()
{
    rivit_->setMaara( rivilla(), ui->maaraEdit->asCents());
    tositteelle();
}

void TuloMenoApuri::seliteMuuttui()
{
    rivit_->setSelite( rivilla(), ui->seliteEdit->text());
    tositteelle();
}

void TuloMenoApuri::maksutapaMuuttui()
{
    int maksutapa = ui->maksutapaCombo->currentIndex();
    bool menoa = tosite()->data(Tosite::TYYPPI).toInt() == TositeTyyppi::MENO;

    ui->viiteLabel->setVisible( maksutapa == LASKU);
    ui->viiteEdit->setVisible( maksutapa == LASKU);

    ui->erapaivaLabel->setVisible( maksutapa == LASKU);
    ui->erapaivaEdit->setVisible( maksutapa == LASKU);

    ui->eraLabel->setVisible( maksutapa == HYVITYS || maksutapa == ENNAKKO);
    ui->eraCombo->setVisible(maksutapa == HYVITYS || maksutapa == ENNAKKO);

    // TODO: Tilien valinnat järkevämmin ;)

    switch (maksutapa) {
    case LASKU:
        if(menoa)
            ui->vastatiliEdit->suodataTyypilla("BO");
        else
            ui->vastatiliEdit->suodataTyypilla("AO");
        break;
    case PANKKI:
        ui->vastatiliEdit->suodataTyypilla("ARP");
        ui->vastatiliEdit->valitseTiliNumerolla(1910);
        break;
    case KATEINEN:
        ui->vastatiliEdit->suodataTyypilla("ARK");
        ui->vastatiliEdit->valitseTiliNumerolla(1900);
        break;

    }
}

void TuloMenoApuri::haeRivi(const QModelIndex &index)
{
    int rivi = index.row();
    ui->tiliEdit->valitseTili( rivit_->tili(rivi));
    ui->maaraEdit->setCents( rivit_->maara(rivi) );
    ui->seliteEdit->setText( rivit_->selite(rivi));
}

void TuloMenoApuri::alusta(bool meno)
{
    if(meno) {
        ui->tiliLabel->setText( tr("Menotili") );
        ui->tiliEdit->suodataTyypilla("(AP|D).*");
        veroFiltteri_->setFilterRegExp("^(0|2[1-79]|927)");
        ui->toimittajaLabel->setText( tr("Toimittaja"));
    } else {
        ui->tiliLabel->setText( tr("Tulotili"));
        ui->tiliEdit->suodataTyypilla("(AP|C).*");
        veroFiltteri_->setFilterRegExp("^(0|1[1-79])");
        ui->toimittajaLabel->setText( tr("Asiakas"));
    }


    bool alv = kp()->asetukset()->onko( AsetusModel::ALV );
    ui->alvLabel->setVisible(alv);
    ui->alvCombo->setVisible(alv);
    ui->alvSpin->setVisible(alv);
    ui->verotonLabel->setVisible(alv);
    ui->verotonEdit->setVisible(alv);
    ui->vahennysCheck->setVisible(alv);

    bool kohdennuksia = kp()->kohdennukset()->kohdennuksia();
    ui->kohdennusLabel->setVisible(kohdennuksia);
    ui->kohdennusCombo->setVisible(kohdennuksia);

    bool merkkauksia = kp()->kohdennukset()->merkkauksia();
    ui->merkkauksetLabel->setVisible(merkkauksia);
    ui->merkkauksetEdit->setVisible(merkkauksia);
}

int TuloMenoApuri::rivilla() const
{
    if( ui->tilellaView->currentIndex().row() < 0 )
        return 0;
    return ui->tilellaView->currentIndex().row();

}
