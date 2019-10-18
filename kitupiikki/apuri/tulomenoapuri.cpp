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
#include "rekisteri/asiakastoimittajadlg.h"

#include <QSortFilterProxyModel>
#include <QDebug>
#include <QJsonDocument>

TuloMenoApuri::TuloMenoApuri(QWidget *parent, Tosite *tosite) :
    ApuriWidget (parent, tosite),
    ui(new Ui::TuloMenoApuri),
    rivit_(new TmRivit),
    kohdennusProxy_( new KohdennusProxyModel(this))

{
    ui->setupUi(this);

    veroFiltteri_ = new QSortFilterProxyModel(this);
    veroFiltteri_->setFilterRole( VerotyyppiModel::KoodiTekstiRooli);
    veroFiltteri_->setSourceModel( kp()->alvTyypit());
    ui->alvCombo->setModel(veroFiltteri_);

    ui->kohdennusCombo->setModel(kohdennusProxy_);
    ui->kohdennusCombo->setModelColumn( KohdennusModel::NIMI);

    ui->alkuEdit->setNull();
    ui->loppuEdit->setNull();
    ui->erapaivaEdit->setNull();


    ui->tilellaView->setModel( rivit_);
    ui->tilellaView->horizontalHeader()->setSectionResizeMode(TmRivit::TILI, QHeaderView::Stretch);


    connect( ui->tiliEdit, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::tiliMuuttui );
    connect( ui->maaraEdit, &KpEuroEdit::textChanged, this, &TuloMenoApuri::maaraMuuttui);
    connect( ui->verotonEdit, &KpEuroEdit::textChanged, this, &TuloMenoApuri::verotonMuuttui);
    connect( ui->alvSpin, SIGNAL( valueChanged(double) ), this, SLOT( veroprossaMuuttui()) );

    connect( ui->lisaaRiviNappi, &QPushButton::clicked, this, &TuloMenoApuri::lisaaRivi);
    connect( ui->poistaRiviNappi, &QPushButton::clicked, this, &TuloMenoApuri::poistaRivi);

    connect( ui->tilellaView->selectionModel(), &QItemSelectionModel::currentRowChanged , this, &TuloMenoApuri::haeRivi);
    connect( ui->seliteEdit, &QLineEdit::textChanged, this, &TuloMenoApuri::seliteMuuttui);
    connect( ui->alvCombo, &QComboBox::currentTextChanged, this, &TuloMenoApuri::verolajiMuuttui);
    connect( ui->vahennysCheck, &QCheckBox::stateChanged, this, &TuloMenoApuri::alvVahennettavaMuuttui);

    connect( ui->kohdennusCombo, &QComboBox::currentTextChanged, this, &TuloMenoApuri::kohdennusMuuttui);
    connect( ui->merkkauksetCC, &CheckCombo::currentTextChanged, this, &TuloMenoApuri::merkkausMuuttui );

    connect( ui->alkuEdit, &KpDateEdit::dateChanged, this, &TuloMenoApuri::jaksoAlkaaMuuttui);
    connect( ui->loppuEdit, &KpDateEdit::dateChanged, this, &TuloMenoApuri::jaksoLoppuuMuuttui);

    connect( ui->maksutapaCombo, &QComboBox::currentTextChanged, this, &TuloMenoApuri::maksutapaMuuttui);
    connect( ui->vastatiliCombo, &TiliCombo::tiliValittu, this, &TuloMenoApuri::tositteelle);

    connect( ui->viiteEdit, &QLineEdit::textChanged, [this] (const QString& text) {this->tosite()->setData(Tosite::VIITE, text);});
    connect( ui->erapaivaEdit, &KpDateEdit::dateChanged, [this] (const QDate& date) {this->tosite()->setData(Tosite::ERAPVM, date);});
    connect( ui->asiakasToimittaja, &AsiakasToimittajaValinta::valittu, [this] { this->tosite()->setData(Tosite::KUMPPANI, this->ui->asiakasToimittaja->id()); });

    connect( tosite, &Tosite::pvmMuuttui, this, &TuloMenoApuri::haeKohdennukset );
    connect( ui->asiakasToimittaja, &AsiakasToimittajaValinta::valittu, this, &TuloMenoApuri::kumppaniValittu);

    connect( ui->vastatiliCombo, &TiliCombo::tiliValittu, this, &TuloMenoApuri::vastatiliMuuttui);
    connect( tosite, &Tosite::pvmMuuttui, this, &TuloMenoApuri::teeTositteelle);
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
    ui->maaraEdit->setValue( map.value("summa").toDouble());
    ui->viiteEdit->setText( map.value("viite").toString() );
    ui->asiakasToimittaja->set(map.value("kumppaniid").toInt(),
                               map.value("kumppaninimi").toString());
    ui->erapaivaEdit->setDate( map.value("erapvm").toDate());
}

void TuloMenoApuri::teeReset()
{

    // Haetaan tietoja mallista ;)
    bool menoa = tosite()->tyyppi() == TositeTyyppi::MENO ||
                 tosite()->tyyppi() == TositeTyyppi::KULULASKU;
    alusta( menoa );



    // Haetaan rivien tiedot

    QVariantList vientiLista = tosite()->viennit()->viennit().toList();
    if( vientiLista.count())
    {
        TositeVienti vastavienti(vientiLista.at(0).toMap());

        Tili* vastatili = kp()->tilit()->tili( vastavienti.tili());

        ui->vastatiliCombo->valitseTili( vastatili->numero() );
        if( vastatili->eritellaankoTase())
            ui->eraCombo->valitse( vastavienti.eraId() );

        for(int i=0; i < ui->maksutapaCombo->count(); i++) {
            if( ui->maksutapaCombo->itemData(i).toInt() == vastatili->numero() ) {
                ui->maksutapaCombo->setCurrentIndex(i);
                break;
            }

        }

        ui->viiteEdit->setText( vastavienti.viite());
        ui->erapaivaEdit->setDate( vastavienti.erapaiva());

        ui->asiakasToimittaja->set( vastavienti.value("kumppani").toMap().value("id").toInt(),
                                vastavienti.value("kumppani").toMap().value("nimi").toString());


        maksutapaMuuttui();
    } else {
        ui->viiteEdit->clear();
        ui->erapaivaEdit->clear();
        ui->asiakasToimittaja->clear();

    }
    rivit_->clear();
    int rivi = 0;
    int i=1;

    if( vientiLista.count() && vientiLista.at(0).toMap().value("alvkoodi").toInt() == AlvKoodi::MAAHANTUONTI)
        i=0;    // Jos kirjataan maahantuonnin veroa, ei ekana ollutkaan välttis vastakirjausta

    while( i < vientiLista.count())
    {
        TositeVienti map = vientiLista.at(i).toMap();
        rivit_->lisaaRivi( map.id() );
        rivit_->setTili( rivi,  map.value("tili").toInt() );
        rivit_->setSelite(rivi, map.value("selite").toString());
        rivit_->setKohdennus(rivi, map.value("kohdennus").toInt());

        rivit_->setMerkkaukset(rivi, map.value("merkkaukset").toList() );
        rivit_->setPoistoaika( rivi, map.tasaerapoisto() );


        qlonglong maara = menoa ? qRound( map.value("debet").toDouble() * 100.0 ) - qRound( map.value("kredit").toDouble() * 100.0 )  :
                                  qRound( map.value("kredit").toDouble() * 100.0 ) - qRound( map.value("debet").toDouble() * 100.0 ) ;


        int verokoodi = map.data(TositeVienti::ALVKOODI).toInt();
        rivit_->setAlvKoodi( rivi, verokoodi );

        rivit_->setAlvProsentti( rivi, map.data(TositeVienti::ALVPROSENTTI).toDouble() );

        rivit_->setJaksoalkaa( rivi, map.data(TositeVienti::JAKSOALKAA).toDate());
        rivit_->setJaksoloppuu( rivi, map.data(TositeVienti::JAKSOLOPPUU).toDate() );

        i++;

        if( verokoodi == AlvKoodi::EIALV || verokoodi == AlvKoodi::ALV0 || verokoodi == AlvKoodi::MYYNNIT_MARGINAALI ||
            verokoodi == AlvKoodi::OSTOT_MARGINAALI || verokoodi == AlvKoodi::YHTEISOMYYNTI_TAVARAT ||
            verokoodi == AlvKoodi::YHTEISOMYYNTI_PALVELUT)
        {
            rivit_->setMaara(rivi, maara);
        } else {
            bool vahennyksia = false;
            qlonglong veroa = 0;

            while( i < vientiLista.count() )
            {
                TositeVienti vmap = vientiLista.at(i).toMap();
                int vkoodi = vmap.data(TositeVienti::ALVKOODI).toInt();
                if( vkoodi < AlvKoodi::ALVKIRJAUS )
                    break;
                if( vkoodi / 100 == AlvKoodi::ALVVAHENNYS / 100 || vkoodi == AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_OSTO) {
                    vahennyksia = true;
                    veroa = qRound( vmap.data(TositeVienti::DEBET).toDouble() * 100.0 - vmap.data(TositeVienti::KREDIT).toDouble() * 100.0);
                } else if( vkoodi / 100 == AlvKoodi::ALVKIRJAUS / 100 || vkoodi == AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_MYYNTI) {
                    veroa = qRound( vmap.data(TositeVienti::KREDIT).toDouble() * 100.0 -  vmap.data(TositeVienti::DEBET).toDouble() * 100.0)  ;
                } else if( vkoodi == AlvKoodi::MAAHANTUONTI_VERO)
                    rivit_->setAlvKoodi( rivi, AlvKoodi::MAAHANTUONTI_VERO);
                i++;
            }
            rivit_->setMaara( rivi, maara + veroa );
            rivit_->setNetto( rivi, maara );

            rivit_->setEiVahennysta(rivi, !vahennyksia);

        }
        rivi++;

    }

    ui->tilellaView->setVisible( rivi > 1 );
    ui->poistaRiviNappi->setEnabled( rivi > 1 );
    if( !rivi )
    {
        rivit_->lisaaRivi();
        if(menoa)
            ui->tiliEdit->valitseTili( kp()->tilit()->tiliTyypilla(TiliLaji::MENO ) );
        else
            ui->tiliEdit->valitseTili( kp()->tilit()->tiliTyypilla(TiliLaji::LVTULO));

        ui->maaraEdit->setCents(0);
        ui->seliteEdit->clear();
        tiliMuuttui();
        maksutapaMuuttui();
        verolajiMuuttui();
        haeKohdennukset();
    }
    else if( ui->tilellaView->selectionModel()->currentIndex().row() == 0)
        haeRivi( rivit_->index(0,0) );
    ui->tilellaView->selectRow(0);

}

bool TuloMenoApuri::teeTositteelle()
{        
    // Lasketaan ensin summa
    qint64 summa = 0l;
    int riveja = rivit_->rowCount();


    bool menoa = tosite()->tyyppi() == TositeTyyppi::MENO ||
                 tosite()->tyyppi() == TositeTyyppi::KULULASKU;
    QDate pvm = tosite()->data(Tosite::PVM).toDate();
    QString otsikko = tosite()->data(Tosite::OTSIKKO).toString();

    QVariantList viennit;


    for(int i=0; i < riveja; i++) {
        double maara = rivit_->maara(i) / 100.0 ;
        double netto = rivit_->netto(i) / 100.0;
        double vero = (rivit_->maara(i) - rivit_->netto(i)) / 100.0;
        double veroprosentti = rivit_->alvProsentti(i);
        int verokoodi = rivit_->alvkoodi(i);

        if( qAbs(maara) < 1e-5 || !rivit_->tili(i).onkoValidi() )
            continue;

        bool maahantuonninvero = false;
        if( verokoodi == AlvKoodi::MAAHANTUONTI_VERO) {
            maahantuonninvero = true;
            verokoodi = AlvKoodi::MAAHANTUONTI;
        }


        TositeVienti vienti;
        vienti.setTyyppi( (menoa ? TositeVienti::OSTO : TositeVienti::MYYNTI) + TositeVienti::KIRJAUS );
        vienti.insert("id", rivit_->vientiId(i));

        // Kirjataanko nettoa vai bruttoa?
        double kirjattava = ( verokoodi == AlvKoodi::MYYNNIT_NETTO  || verokoodi == AlvKoodi::OSTOT_NETTO ||
                                 verokoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI || verokoodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO ||
                                 ((verokoodi == AlvKoodi::RAKENNUSPALVELU_OSTO || verokoodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
                                 verokoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verokoodi == AlvKoodi::MAAHANTUONTI )
                                  && !rivit_->eiVahennysta(i)) ) ? netto : maara;

        vienti.insert("pvm", pvm);
        vienti.insert("tili", rivit_->tili(i).numero());
        if( menoa ) {
            if( kirjattava > 0)
                vienti.insert("debet", kirjattava);
            else
                vienti.insert("kredit", 0 - kirjattava);
        } else {
            if( kirjattava > 0)
                vienti.insert("kredit", kirjattava);
            else
                vienti.insert("debet", 0 - kirjattava);
        }
        QString selite = rivit_->selite(i);
        if( selite.isEmpty())
            vienti.insert("selite", otsikko);
        else
            vienti.insert("selite", selite);

        vienti.setAlvProsentti( veroprosentti);

        if( !kp()->alvTyypit()->nollaTyyppi(verokoodi))
            vienti.setAlvKoodi( verokoodi );

        vienti.setKohdennus( rivit_->kohdennus(i) );
        vienti.setMerkkaukset( rivit_->merkkaukset(i));

        QDate alkupvm = rivit_->jaksoalkaa(i);
        if( alkupvm.isValid())
        {
            vienti.setJaksoalkaa( alkupvm );
            QDate loppupvm = rivit_->jaksoloppuu(i);
            if( loppupvm.isValid() )
                vienti.setJaksoloppuu( loppupvm );
        }

        if( rivit_->tili(i).eritellaankoTase() )
            vienti.setEra( vienti.id() ? vienti.id() : -1  );

        if( rivit_->tili(i).onko(TiliLaji::TASAERAPOISTO) )
            vienti.setTasaerapoisto( ui->poistoSpin->value() );

        // Kirjataan asiakas- ja toimittajatiedot myös vienteihin, jotta voidaan ehdottaa
        // tiliä aiempien kirjausten perusteella
        if( ui->asiakasToimittaja->id() > 0)
            vienti.setKumppani( ui->asiakasToimittaja->id() );

        viennit.append(vienti);


        // Alv-saamisten kirjaaminen
        if( verokoodi == AlvKoodi::OSTOT_NETTO || verokoodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO ||
              ((verokoodi == AlvKoodi::RAKENNUSPALVELU_OSTO || verokoodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
                verokoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verokoodi == AlvKoodi::MAAHANTUONTI )
               && !rivit_->eiVahennysta(i)) ) {

            TositeVienti palautus;
            palautus.setTyyppi( TositeVienti::OSTO + TositeVienti::ALVKIRJAUS );

            palautus.setPvm(pvm);
            if( verokoodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO) {
                palautus.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVSAATAVA).numero() );
                palautus.setAlvKoodi( AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_OSTO );
            } else {
                palautus.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA).numero());
                palautus.setAlvKoodi( AlvKoodi::ALVVAHENNYS + verokoodi );
            }
            if( vero > 0)
                palautus.setDebet( vero );
            else
                palautus.setKredit( 0 - vero);

            palautus.setAlvProsentti( veroprosentti );
            palautus.setSelite( otsikko );
            viennit.append(palautus);
        }

        // Alv-veron kirjaaminen
        if( verokoodi == AlvKoodi::MYYNNIT_NETTO || verokoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI ||
                verokoodi == AlvKoodi::RAKENNUSPALVELU_OSTO || verokoodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
                verokoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verokoodi == AlvKoodi::MAAHANTUONTI )
        {
            TositeVienti verorivi;
            verorivi.setTyyppi( TositeVienti::MYYNTI + TositeVienti::ALVKIRJAUS );
            verorivi.setPvm(pvm);
            if( verokoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI) {
                verorivi.setTili( kp()->tilit()->tiliTyypilla( TiliLaji::KOHDENTAMATONALVVELKA ).numero() );
                verorivi.setAlvKoodi( AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_MYYNTI);
            } else {
                verorivi.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).numero());
                verorivi.setAlvKoodi( AlvKoodi::ALVKIRJAUS + verokoodi);
            }

            if( vero > 0)
                verorivi.setKredit( vero );
            else
                verorivi.setDebet( 0 - vero);

            verorivi.setAlvProsentti( veroprosentti);
            verorivi.setSelite(otsikko);
            viennit.append(verorivi);
        }

        // Mahdollinen maahantuonnin veron kirjaamisen vastakirjaaminen
        if( maahantuonninvero ) {
            TositeVienti tuonti;
            tuonti.setTyyppi(TositeVienti::OSTO + TositeVienti::MAAHANTUONTIVASTAKIRJAUS);
            tuonti.setPvm(pvm);
            tuonti.setTili(rivit_->tili(i).numero());
            if( netto > 0)
                tuonti.setKredit(netto);
            else
                tuonti.setDebet( 0 - netto);

            tuonti.setSelite(otsikko);
            tuonti.setAlvKoodi(AlvKoodi::MAAHANTUONTI_VERO);
            viennit.append(tuonti);

        } else {
            if( verokoodi == AlvKoodi::RAKENNUSPALVELU_OSTO || verokoodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
                    verokoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verokoodi == AlvKoodi::MAAHANTUONTI )
                summa += qRound64(netto * 100.0);
            else
                summa += qRound64(maara * 100.0);
        }
    }

    if( summa ) {
        TositeVienti vasta;
        vasta.setTyyppi( (menoa ? TositeVienti::OSTO : TositeVienti::MYYNTI) + TositeVienti::VASTAKIRJAUS );
        vasta.insert("pvm", pvm);
        Tili vastatili = kp()->tilit()->tiliNumerolla( ui->vastatiliCombo->valittuTilinumero() );
        vasta.insert("tili", vastatili.numero() );

        if( vastatili.eritellaankoTase())
            vasta.setEra( ui->eraCombo->valittuEra() );

        if( menoa ) {
            if( summa > 0)
                vasta.insert("kredit", summa / 100.0);
            else
                vasta.insert("debet", 0 - summa / 100.0);
        } else {
            if( summa > 0)
                vasta.insert("debet", summa / 100.0);
            else
                vasta.insert("kredit", 0 - summa / 100.0);
        }
        vasta.insert("selite", otsikko);

        if( !ui->viiteEdit->text().isEmpty())
            vasta.setViite( ui->viiteEdit->text());
        if( ui->erapaivaEdit->date().isValid())
            vasta.setErapaiva( ui->erapaivaEdit->date());

        // Asiakas tai toimittaja
        if( ui->asiakasToimittaja->id() > 0)
            vasta.setKumppani( ui->asiakasToimittaja->id() );

        qDebug() << "*" << summa << "**" << vasta;

        viennit.insert(0, vasta);
    }

    tosite()->viennit()->asetaViennit(viennit);

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
    Tili tili = ui->tiliEdit->valittuTili();
    rivit_->setTili( rivilla(), tili.numero() );

    bool tasapoisto = tili.onko(TiliLaji::TASAERAPOISTO);
    ui->poistoLabel->setVisible(tasapoisto);
    ui->poistoSpin->setVisible(tasapoisto);
    ui->poistoSpin->setValue( tili.luku("tasaerapoisto") / 12 );

    // TODO: Vero-oletusten hakeminen

    if( kp()->asetukset()->onko(AsetusModel::ALV))
    {
        ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( tili.luku("alvlaji"), VerotyyppiModel::KoodiRooli ) );
        ui->alvSpin->setValue( tili.str("alvprosentti").toDouble() );
    }

    tositteelle();

}

void TuloMenoApuri::verolajiMuuttui()
{
    int verolaji = ui->alvCombo->currentData(VerotyyppiModel::KoodiRooli).toInt();

    bool naytaMaara = !( verolaji == AlvKoodi::RAKENNUSPALVELU_OSTO || verolaji == AlvKoodi::YHTEISOHANKINNAT_TAVARAT
                                 || verolaji == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verolaji == AlvKoodi::MAAHANTUONTI
                                 || verolaji == AlvKoodi::MAAHANTUONTI_VERO)  ;

    bool naytaVeroton =  verolaji == AlvKoodi::OSTOT_NETTO || verolaji == AlvKoodi::MYYNNIT_NETTO ||
                                 verolaji == AlvKoodi::OSTOT_BRUTTO || verolaji == AlvKoodi::MYYNNIT_BRUTTO ||
                                 verolaji == AlvKoodi::MAKSUPERUSTEINEN_OSTO || verolaji == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI ||
                                 !naytaMaara ;

    ui->maaraLabel->setVisible(naytaMaara);
    ui->maaraEdit->setVisible(naytaMaara);
    ui->verotonLabel->setVisible(naytaVeroton);
    ui->verotonEdit->setVisible(naytaVeroton);


    ui->alvSpin->setVisible( !ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() );
    ui->vahennysCheck->setVisible( verolaji == AlvKoodi::RAKENNUSPALVELU_OSTO ||
                                   verolaji == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
                                   verolaji == AlvKoodi::YHTEISOHANKINNAT_PALVELUT ||
                                   verolaji == AlvKoodi::MAAHANTUONTI ||
                                   verolaji == AlvKoodi::MAAHANTUONTI_VERO);

    rivit_->setAlvKoodi( rivilla(), verolaji );
    if( ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() )
        rivit_->setAlvProsentti(rivilla(), 0.0);
    else
    {
        if( ui->alvSpin->value() == 0.0)
            ui->alvSpin->setValue(24.0);
        rivit_->setAlvProsentti(rivilla(), ui->alvSpin->value() );
    }


    tositteelle();
}

void TuloMenoApuri::maaraMuuttui()
{
    qlonglong maara = ui->maaraEdit->asCents();

    if( ui->maaraEdit->hasFocus()) {
        bruttoSnt_ = maara;
        nettoSnt_ = 0;
        double verokanta = ui->alvSpin->value();
        qlonglong vero = qRound( maara * verokanta / ( 100 + verokanta) );
        ui->verotonEdit->setCents( maara - vero);
    }

    rivit_->setMaara( rivilla(), maara);
    tositteelle();
}

void TuloMenoApuri::verotonMuuttui()
{
    qlonglong veroton = ui->verotonEdit->asCents();

    if( ui->verotonEdit->hasFocus()) {
        nettoSnt_ = veroton;
        bruttoSnt_ = 0;
        double verokanta = ui->alvSpin->value();
        ui->maaraEdit->setCents( qRound( ( 100 + verokanta) * veroton / 100.0 ) );
    }
    rivit_->setNetto(rivilla(), veroton);
    tositteelle();
}

void TuloMenoApuri::veroprossaMuuttui()
{
    double verokanta = ui->alvSpin->value();
    rivit_->setAlvProsentti(rivilla(), verokanta  );

    if( bruttoSnt_ ) {
        qlonglong vero = qRound( bruttoSnt_ * verokanta / ( 100 + verokanta) );
        ui->verotonEdit->setCents( bruttoSnt_ - vero);
    } else if( nettoSnt_) {
        ui->maaraEdit->setCents( qRound( (100 + verokanta) * nettoSnt_ / 100.0 ) );
    }
}

void TuloMenoApuri::alvVahennettavaMuuttui()
{
    rivit_->setEiVahennysta( rivilla(), ui->vahennysCheck->isChecked() );
    tositteelle();
}

void TuloMenoApuri::seliteMuuttui()
{
    rivit_->setSelite( rivilla(), ui->seliteEdit->text());
    tositteelle();
}

void TuloMenoApuri::maksutapaMuuttui()
{
    int maksutapatili = ui->maksutapaCombo->currentData().toInt();

    if( maksutapatili)
        ui->vastatiliCombo->valitseTili(maksutapatili);

    ui->vastatiliLabel->setVisible( !maksutapatili  );
    ui->vastatiliCombo->setVisible( !maksutapatili );

    vastatiliMuuttui();

}

void TuloMenoApuri::vastatiliMuuttui()
{
    Tili vastatili = kp()->tilit()->tiliNumerolla( ui->vastatiliCombo->valittuTilinumero() );

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
    rivit_->setKohdennus( rivilla(), ui->kohdennusCombo->currentData(KohdennusModel::IdRooli).toInt());
    tositteelle();    
}

void TuloMenoApuri::merkkausMuuttui()
{

    rivit_->setMerkkaukset( rivilla(), ui->merkkauksetCC->selectedDatas());
    tositteelle();

}

void TuloMenoApuri::jaksoAlkaaMuuttui()
{
    QDate alkupvm = ui->alkuEdit->date();
    rivit_->setJaksoalkaa( rivilla(), alkupvm);
    ui->loppuEdit->setEnabled( alkupvm.isValid() );
    tositteelle();
}

void TuloMenoApuri::jaksoLoppuuMuuttui()
{
    rivit_->setJaksoloppuu( rivilla(), ui->loppuEdit->date());
    tositteelle();
}

void TuloMenoApuri::poistoAikaMuuttuu()
{
    rivit_->setPoistoaika( rivilla(), ui->poistoSpin->value() * 12);
    tositteelle();
}

void TuloMenoApuri::haeRivi(const QModelIndex &index)
{
    aloitaResetointi();
    int rivi = index.row();
    ui->tiliEdit->valitseTili( rivit_->tili(rivi));
    ui->maaraEdit->setCents( rivit_->maara(rivi) );
    ui->verotonEdit->setCents( rivit_->netto(rivi));
    ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( rivit_->alvkoodi(rivi), VerotyyppiModel::KoodiRooli ) );
    ui->alvSpin->setValue( rivit_->alvProsentti(rivi) );
    ui->vahennysCheck->setChecked( rivit_->eiVahennysta(rivi) );
    ui->poistoSpin->setValue( rivit_->poistoaika(rivi) / 12 );

    ui->alkuEdit->setDate( rivit_->jaksoalkaa(rivi) );
    ui->loppuEdit->setEnabled( rivit_->jaksoalkaa(rivi).isValid());
    ui->loppuEdit->setDate( rivit_->jaksoloppuu(rivi));

    bruttoSnt_ = rivit_->maara(rivi);
    nettoSnt_ = 0;

    ui->seliteEdit->setText( rivit_->selite(rivi));

    haeKohdennukset();
    lopetaResetointi();
}

void TuloMenoApuri::haeKohdennukset()
{
    int nykyinenKohdennus = rivit_->rowCount() ? rivit_->kohdennus( rivilla() ) : 0 ;
    QVariantList merkatut =  rivit_->rowCount() ?  rivit_->merkkaukset( rivilla()) : QVariantList();
    QDate pvm = tosite()->data(Tosite::PVM).toDate();

    kohdennusProxy_->asetaKohdennus( nykyinenKohdennus );
    kohdennusProxy_->asetaPaiva( pvm );

    ui->kohdennusLabel->setVisible( kohdennusProxy_->rowCount() > 1);
    ui->kohdennusCombo->setVisible( kohdennusProxy_->rowCount() > 1);

    ui->kohdennusCombo->setCurrentIndex( ui->kohdennusCombo->findData(nykyinenKohdennus, KohdennusModel::IdRooli ));

    KohdennusProxyModel merkkausproxy(this, pvm, -1, KohdennusProxyModel::MERKKKAUKSET );
    ui->merkkauksetCC->clear();

    ui->merkkauksetLabel->setVisible( merkkausproxy.rowCount());
    ui->merkkauksetCC->setVisible( merkkausproxy.rowCount());

    for(int i=0; i < merkkausproxy.rowCount(); i++) {
        int koodi = merkkausproxy.data( merkkausproxy.index(i,0), KohdennusModel::IdRooli ).toInt();
        QString nimi = merkkausproxy.data( merkkausproxy.index(i,0), KohdennusModel::NimiRooli ).toString();

        Qt::CheckState state = merkatut.contains( koodi ) ? Qt::Checked : Qt::Unchecked;
        ui->merkkauksetCC->addItem(nimi, koodi, state);
    }

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
    ui->vastatiliCombo->suodataTyypilla("[AB]");

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
