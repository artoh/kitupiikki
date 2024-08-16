/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include "selauswg.h"
#include "db/kirjanpito.h"
#include "selausmodel.h"
#include "db/tositetyyppimodel.h"

#include <QDate>
#include <QSortFilterProxyModel>
#include <QSqlQuery>
#include <QScrollBar>
#include <QKeyEvent>
#include <QMenu>
#include <QDebug>
#include <QHeaderView>
#include <QTimer>
#include <QSettings>
#include <QStyleHints>

#include "lisaikkuna.h"
#include "db/yhteysmodel.h"
#include "pilvi/pilvimodel.h"
#include "tositeselausmodel.h"
#include "laskutus/laskudlg/laskudialogitehdas.h"

SelausWg::SelausWg(QWidget *parent) :
    KitupiikkiSivu(parent),
    ui(new Ui::SelausWg)
{
    ui->setupUi(this);

    ui->valintaTab->addTab(QIcon(":/pic/tekstisivu.png"),tr("&Tositteet"));
    ui->valintaTab->addTab(QIcon(":/pic/harmaa.png"), tr("&Luonnokset"));
    ui->valintaTab->addTab(QIcon(":/pic/vientilista.png"),tr("&Viennit"));
    ui->valintaTab->addTab(QIcon(":/pic/harmaahuomio.png"),tr("&Huomioitavat"));
    ui->valintaTab->addTab(QIcon(":/pic/roskis.png"), tr("&Poistetut"));
    ui->valintaTab->addTab(QIcon(":/pic/inbox.png"), tr("&Saapuneet"));    


    model = new SelausModel(this);
    tositeModel = new TositeSelausModel(this);


    // Proxyä käytetään tilien tai tositelajien suodattamiseen
    selausProxy_ = new SelausProxyModel(model, this);
    tositeProxy_ = new TositeSelausProxyModel(tositeModel, this);
    connect( selausProxy_, &QSortFilterProxyModel::modelReset, this, &SelausWg::modelResetoitu);
    connect( selausProxy_, &QSortFilterProxyModel::dataChanged, this, &SelausWg::modelResetoitu);

    connect( tositeProxy_, &QSortFilterProxyModel::modelReset, this, &SelausWg::modelResetoitu);
    connect( tositeProxy_, &QSortFilterProxyModel::dataChanged, this, &SelausWg::modelResetoitu);


    ui->selausView->horizontalHeader()->setStretchLastSection(true);
    ui->selausView->verticalHeader()->hide();
    lataaKoon_ = true;
    ui->selausView->setModel(tositeProxy_);
    ui->selausView->setWordWrap(false);

    ui->selausView->sortByColumn(SelausModel::PVM, Qt::AscendingOrder);

    connect( ui->alkuEdit, SIGNAL(editingFinished()), this, SLOT(paivita()));
    connect( ui->loppuEdit, SIGNAL(editingFinished()), this, SLOT(paivita()));
    connect( ui->tiliCombo, SIGNAL(currentTextChanged(QString)), this, SLOT(suodata()));
    connect( ui->etsiEdit, &QLineEdit::textChanged, this, &SelausWg::etsi);

    connect( ui->selausView, &QTableView::clicked, this, &SelausWg::naytaTositeRivilta);

    ui->valintaTab->setCurrentIndex(0);     // Oletuksena tositteiden selaus
    connect( ui->valintaTab, SIGNAL(currentChanged(int)), this, SLOT(selaa(int)));

    connect( Kirjanpito::db(), SIGNAL(kirjanpitoaMuokattu()), this, SLOT(paivita()));
    connect( kp()->tilikaudet(), &TilikausiModel::modelReset, this, &SelausWg::alusta);

    connect( ui->alkuEdit, SIGNAL(dateChanged(QDate)), this, SLOT(alkuPvmMuuttui()));

    ui->selausView->installEventFilter(this);


    connect( kp(), &Kirjanpito::tilikausiAvattu, this, &SelausWg::alusta);
    connect( ui->paivitaNappi, &QPushButton::clicked, this, &SelausWg::paivita);

    connect( ui->selausView, &QTableView::customContextMenuRequested, this, &SelausWg::contextMenu);
    connect( ui->selausView->horizontalHeader(), &QHeaderView::sectionResized, this, &SelausWg::tallennaKoot);
    connect( ui->selausView->horizontalHeader(), &QHeaderView::sectionMoved, this, &SelausWg::tallennaKoot);
    connect( ui->selausView->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this, &SelausWg::tallennaKoot);

    connect( ui->edellinenNappi, &QPushButton::clicked, this, [this] { this->nuoliSelaus(false); });
    connect( ui->seuraavaNappi, &QPushButton::clicked, this, [this] { this->nuoliSelaus(true);});
    connect( ui->kuukausiNappi, &QPushButton::clicked, this, &SelausWg::tamaKuukausi);
    connect( ui->tilikausiButton, &QPushButton::clicked, this, &SelausWg::tamaTilikausi);

    connect( ui->poistaNappi, &QPushButton::toggled, this, &SelausWg::poistoMoodiin);
    connect( ui->peruPoistoNappi, &QPushButton::clicked, this, &SelausWg::poistoMoodiin);    
    connect( ui->teePoistoNappi, &QPushButton::clicked, this, &SelausWg::teePoisto);

//    connect( ui->huomioButton, &QPushButton::toggled, tositeProxy_, &TositeSelausProxyModel::suodataHuomio);

    ui->selausView->horizontalHeader()->setSectionsMovable(true);

    if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
        ui->poistoGroup->setStyleSheet("QGroupBox, QLabel { background-color: rgb(200, 0, 0); color: white }") ;
    } else {
        ui->poistoGroup->setStyleSheet("QGroupBox, QLabel { background-color: rgb(255, 180, 0); color: black }");
    }

}

SelausWg::~SelausWg()
{
    QDate alkupvm = ui->alkuEdit->date();
    QDate loppupvm = ui->loppuEdit->date();

    kp()->settings()->setValue("SelaaKuukausittain", alkupvm.day() == 1 && loppupvm.addDays(1).day() == 1);

    delete ui;
}

void SelausWg::alusta()
{
    QDate alku = Kirjanpito::db()->tilikaudet()->kirjanpitoAlkaa();
    QDate loppu = Kirjanpito::db()->tilikaudet()->kirjanpitoLoppuu();

    QDate pvm = kp()->db()->paivamaara();
    if( pvm > loppu)
        pvm = loppu;

    Tilikausi nytkausi = Kirjanpito::db()->tilikaudet()->tilikausiPaivalle( pvm );
    bool kuukausittain = kp()->settings()->value("SelaaKuukausittain").toBool();

    QDate nytalkaa = kuukausittain ? QDate(pvm.year(), pvm.month(), 1) : nytkausi.alkaa();
    QDate nytloppuu = kuukausittain ? QDate(pvm.year(), pvm.month(), pvm.daysInMonth() )  : nytkausi.paattyy();

    ui->alkuEdit->setDate(nytalkaa);
    ui->loppuEdit->setDate(nytloppuu);
    ui->alkuEdit->setDateRange(alku, loppu);
    ui->loppuEdit->setDateRange(alku, loppu);


    paivita();

}

void SelausWg::paivita()
{    
    QDate alkupvm = ui->alkuEdit->date();
    QDate loppupvm = ui->loppuEdit->date();

    if( !alkupvm.isValid())
        alkupvm = kp()->tilikaudet()->kirjanpitoAlkaa();
    if( !loppupvm.isValid())
        loppupvm = kp()->tilikaudet()->kirjanpitoLoppuu();

    qApp->processEvents();


    if( ui->valintaTab->currentIndex() == VIENNIT )
    {
        kp()->odotusKursori(true);
        if(selaustili_ > -1)
            selausProxy_->suodataTililla(-1);
        model->lataa( alkupvm, loppupvm, selaustili_);
    }
    else if( ui->valintaTab->currentIndex() == SAAPUNEET) {
        kp()->odotusKursori(true);
        tositeModel->lataa( alkupvm, loppupvm, TositeSelausModel::SAAPUNEET);
    }
    else if( ui->valintaTab->currentIndex() == TOSITTEET || ui->valintaTab->currentIndex() == HUOMIO )
    {
        kp()->odotusKursori(true);
        tositeModel->lataa( alkupvm, loppupvm);
    } else if( ui->valintaTab->currentIndex() == LUONNOKSET){
        kp()->odotusKursori(true);
        tositeModel->lataa( alkupvm, loppupvm, TositeSelausModel::LUONNOKSET);
    } else if( ui->valintaTab->currentIndex() == POISTETUT) {
        kp()->odotusKursori(true);
        tositeModel->lataa( alkupvm, loppupvm, TositeSelausModel::POISTETUT);
    }
    tositeProxy_->suodataHuomio( ui->valintaTab->currentIndex() == HUOMIO );

    bool naytaSelaus = false;

    if( alkupvm.day() == 1 && loppupvm.addDays(1).day() == 1 && alkupvm.daysTo(loppupvm) < 32) {
        // Selattavana on kokonainen kuukausi
        naytaSelaus = true;
        ui->kuukausiNappi->setVisible(false);
        ui->tilikausiButton->setVisible(true);

        ui->edellinenNappi->setToolTip(tr("Edellinen kuukausi"));
        ui->seuraavaNappi->setToolTip(tr("Seuraava kuukausi"));
    } else {
        ui->kuukausiNappi->setVisible(true);

        Tilikausi tilikausi = kp()->tilikaudet()->tilikausiPaivalle(alkupvm);
        if( alkupvm == tilikausi.alkaa() && loppupvm == tilikausi.paattyy()) {
            // Selattavana on kokonainen tilikausi
            ui->tilikausiButton->setVisible(false);
            naytaSelaus = true;
            ui->edellinenNappi->setToolTip(tr("Edellinen tilikausi"));
            ui->seuraavaNappi->setToolTip(tr("Seuraava tilikausi"));
        } else {
            ui->tilikausiButton->setVisible(true);
        }
    }

    ui->edellinenNappi->setVisible(naytaSelaus);
    ui->seuraavaNappi->setVisible(naytaSelaus);
    if( naytaSelaus ) {
        ui->edellinenNappi->setEnabled( kp()->tilikaudet()->onkoTilikautta(alkupvm.addDays(-1)) );
        ui->seuraavaNappi->setEnabled( kp()->tilikaudet()->onkoTilikautta(loppupvm.addDays(1)));
    }

    // Huomio käytettävissä vain tositteita selattaessa
    // ui->huomioButton->setVisible(  ui->valintaTab->currentIndex() != VIENNIT);

    ui->poistaNappi->setVisible( ui->valintaTab->currentIndex() == TOSITTEET &&
                                kp()->yhteysModel() &&
                                kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_MUOKKAUS) &&
                                ui->loppuEdit->date() > kp()->tilitpaatetty() );
    poistoMoodiin( false );

}

void SelausWg::suodata()
{
    QVariant suodatin = ui->tiliCombo->currentData();
    if( suodatin.isNull())
        return;

    if( ui->valintaTab->currentIndex() == VIENNIT) {
        if(selaustili_ > -1 && suodatin.isValid() && suodatin.toInt() != selaustili_) {
            selaustili_ = -1;
            paivita();
        } else {
            selausProxy_->suodataTililla( suodatin.toInt() );
        }
    } else {
        if( suodatin.type() == QVariant::Int )
            tositeProxy_->suodataTositetyyppi(suodatin.toInt());
        else
            tositeProxy_->suodataTositesarja(suodatin.toString());
    }

    paivitaSummat();    
    saldo_ = 0;

    if( ui->valintaTab->currentIndex() == VIENNIT && suodatin.toInt() ) {

        QDate loppupvm = ui->loppuEdit->date();
        if( !loppupvm.isValid())
            loppupvm = kp()->tilikaudet()->kirjanpitoLoppuu();

        KpKysely *saldokysely = kpk("/saldot");
        saldokysely->lisaaAttribuutti("pvm",loppupvm);
        saldokysely->lisaaAttribuutti("tili", suodatin.toInt() );
        connect( saldokysely, &KpKysely::vastaus, this, &SelausWg::paivitaSummat);
        saldokysely->kysy();
    }
}

void SelausWg::paivitaSuodattimet()
{
    if( ui->valintaTab->currentIndex() == VIENNIT)
    {
        if(selaustili_ > -1) {
            ui->tiliCombo->clear();
            ui->tiliCombo->addItem(QString("%1 %2").arg(selaustili_).arg(kp()->tilit()->nimi(selaustili_)), selaustili_);
            ui->tiliCombo->setCurrentText(QString("%1 %2").arg(selaustili_).arg(kp()->tilit()->nimi(selaustili_)));
            ui->tiliCombo->insertItem(0, QIcon(":/pic/Possu64.png"),tr("Kaikki tilit"), -1);

        } else {
            QString valittu = ui->tiliCombo->currentText();
            ui->tiliCombo->clear();
            ui->tiliCombo->insertItem(0, QIcon(":/pic/Possu64.png"),tr("Kaikki tilit"), -1);
            for(int tiliNro : model->tiliLista()) {
                if( tiliNro ) {
                    ui->tiliCombo->addItem(QString("%1 %2").arg(tiliNro).arg(kp()->tilit()->nimi(tiliNro)), tiliNro);
                } else {
                    ui->tiliCombo->addItem(QIcon(":/pic/oranssi.png"), tr("Tiliöimättä"), 0);
                }
            }
            if( !valittu.isEmpty())
                ui->tiliCombo->setCurrentText(valittu);
        }
    } else {
        QString valittu = ui->tiliCombo->currentText();
        ui->tiliCombo->clear();
        ui->tiliCombo->insertItem(0, QIcon(":/pic/Possu64.png"),tr("Kaikki tositteet"), -1);
        for( int tyyppikoodi : tositeModel->tyyppiLista() ) {
            ui->tiliCombo->addItem( kp()->tositeTyypit()->kuvake(tyyppikoodi), tulkkaa(kp()->tositeTyypit()->nimi(tyyppikoodi)), tyyppikoodi );
        }
        QStringList sarjat = tositeModel->sarjaLista();
        if(!sarjat.isEmpty()) {
            ui->tiliCombo->insertSeparator(ui->tiliCombo->count());
            for(const QString& sarja : qAsConst( sarjat )) {
                ui->tiliCombo->addItem( QIcon(":/pic/tyhja.png"), sarja, sarja );
            }
        }

        if( !valittu.isEmpty())
            ui->tiliCombo->setCurrentText(valittu);
    }
}

void SelausWg::paivitaSummat(QVariant *data)
{

    QAbstractItemModel* model = ui->selausView->model();

    if( ui->valintaTab->currentIndex() == VIENNIT)
    {
        Euro debetSumma = 0;
        Euro kreditSumma = 0;



        for(int i=0; i< model->rowCount(QModelIndex()); i++)
        {
            debetSumma +=  model->index(i, SelausModel::DEBET).data(Qt::EditRole).toLongLong();
            kreditSumma += model->index(i, SelausModel::KREDIT).data(Qt::EditRole).toLongLong();
        }

        if( data && data->toMap().count()) {
            saldo_ = data->toMap().first().toString();

        }

        if( saldo_ ) {
            ui->summaLabel->setText( tr("Debet %L1\tKredit %L2 \nLoppusaldo %L3\tLkm %L4")
                                        .arg( debetSumma.display(), kreditSumma.display(),saldo_.display(), QString::number(model->rowCount())));
        } else {
            ui->summaLabel->setText( tr("Debet %L1\tKredit %L2\tLkm %L3")
                                        .arg( debetSumma.display(), kreditSumma.display(), QString::number(model->rowCount())) );
        }


    } else {

        // Lasketaan summat
        Euro summa = 0;
        for(int i=0; i<model->rowCount(QModelIndex()); i++)
        {
            summa += model->index(i, TositeSelausModel::SUMMA).data(Qt::EditRole).toLongLong();
        }
        ui->summaLabel->setText( tr("Summa %1 \tLkm %L2").arg(summa.display(), QString::number(model->rowCount())));

        ui->poistaNappi->setEnabled( model->rowCount() > 0);
    }

}

void SelausWg::naytaTositeRivilta(QModelIndex index)
{
    if( !index.isValid() || ui->poistoGroup->isVisible() )
        return;


    int id = index.data( Qt::UserRole).toInt();
    int tyyppi = index.data(TositeSelausModel::TositeTyyppiRooli).toInt();
    valittu_ = id;
    skrolli_ = ui->selausView->verticalScrollBar()->sliderPosition();

    if( tyyppi >= TositeTyyppi::MYYNTILASKU && tyyppi < TositeTyyppi::SIIRTO) {
        LaskuDialogiTehdas::naytaLasku(id);
    } else {
        QList<int> lista;
        for(int i=0; i < index.model()->rowCount(); i++) {
            QModelIndex indeksi = index.sibling(i,0);
            int sId = indeksi.data(Qt::UserRole).toInt();
            if( !lista.contains(sId)) lista.append(sId);
        }

        emit tositeValittu( id, lista, KirjausSivu::PALATAAN_AINA );
    }

}

void SelausWg::selaa(int tilinumero, const Tilikausi& tilikausi)
{
    selaustili_ = tilinumero;

    // Ohjelmallisesti selaa tiettynä tilikautena tiettyä tiliä
    ui->alkuEdit->setDate( tilikausi.alkaa());
    ui->loppuEdit->setDate( tilikausi.paattyy());

    if(ui->valintaTab->currentIndex() != VIENNIT)
        ui->valintaTab->setCurrentIndex(VIENNIT);
    else
        paivita();
}


void SelausWg::naytaSaapuneet()
{
    ui->valintaTab->setCurrentIndex(SAAPUNEET);
}

void SelausWg::naytaHuomioitavat()
{
    ui->valintaTab->setCurrentIndex(HUOMIO);
    ui->alkuEdit->setDate( kp()->tilitpaatetty().addDays(1) );
    ui->loppuEdit->setDate( kp()->tilikaudet()->kirjanpitoLoppuu() );   
    paivita();
}

void SelausWg::alkuPvmMuuttui()
{
    // Jos siirrytään toiseen tilikauteen...
    if( kp()->tilikaudet()->tilikausiPaivalle(  ui->alkuEdit->date()).alkaa() != kp()->tilikaudet()->tilikausiPaivalle(ui->loppuEdit->date()).alkaa() )
        ui->loppuEdit->setDate( kp()->tilikaudet()->tilikausiPaivalle( ui->alkuEdit->date() ).paattyy() );
    else if( ui->alkuEdit->date() > ui->loppuEdit->date())
        ui->loppuEdit->setDate( ui->alkuEdit->date().addMonths(1).addDays(-1) );
}

void SelausWg::selaa(int kumpi)
{
    ui->etsiEdit->clear();
    lataaKoon_=true;

    if( kumpi == VIENNIT) {
        selausProxy_->setFilterFixedString(ui->etsiEdit->text());
        ui->selausView->setColumnHidden( SelausModel::ALV, !kp()->asetukset()->onko(AsetusModel::AlvVelvollinen) );
        ui->selausView->setModel(selausProxy_);        
    } else {
        tositeProxy_->setFilterFixedString(ui->etsiEdit->text());
        ui->selausView->setModel(tositeProxy_);
    }
    QTimer::singleShot(10, this, [this] { this->lataaKoot();});
    paivita();
    ui->selausView->setFocus();
}

void SelausWg::contextMenu(const QPoint &pos)
{
    QModelIndex index = ui->selausView->indexAt(pos);
    QMenu menu;
    QAction* ikkuna = menu.addAction(QIcon(":/pic/muokkaa.png"),tr("Avaa uudessa ikkunassa"));

    QAction* valittu = menu.exec(ui->selausView->mapToGlobal(pos));
    if( valittu == ikkuna) {
        int id = index.data(Qt::UserRole).toInt();
        if( id ) {
            int tyyppi = index.data(TositeSelausModel::TositeTyyppiRooli).toInt();
            if( tyyppi >= TositeTyyppi::MYYNTILASKU && tyyppi < TositeTyyppi::SIIRTO) {
                naytaTositeRivilta(index);
            } else {
                LisaIkkuna *ikkuna = new LisaIkkuna;
                ikkuna->kirjaa(id);
            }
        }
    }
    // TODO: Avaus
}

void SelausWg::siirrySivulle()
{
    if( qobject_cast<PilviModel*>(kp()->yhteysModel())) {
        if( ui->valintaTab->count() == SAAPUNEET + 1)
            ui->valintaTab->removeTab(SAAPUNEET);
    } else {
        if( ui->valintaTab->count() == SAAPUNEET)
            ui->valintaTab->addTab(QIcon(":/pic/inbox.png"), tr("&Saapuneet"));
    }

    ui->valintaTab->setTabEnabled(POISTETUT, kp()->yhteysModel() &&
                                 ( kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_MUOKKAUS) ||
                                   kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_LUONNOS)));

    selaa( ui->valintaTab->currentIndex() );
}

void SelausWg::modelResetoitu()
{
    if( ui->selausView->model() && ui->selausView->isVisible()) {
        paivitaSummat();
        paivitaSuodattimet();
    }
    kp()->odotusKursori(false);
    QTimer::singleShot(10, this, [this] { this->palautaValinta();});

}

void SelausWg::etsi(const QString &teksti)
{
    if( ui->valintaTab->currentIndex() == VIENNIT)
        selausProxy_->etsi(teksti);
    else
        tositeProxy_->etsi(teksti);
}

void SelausWg::tallennaKoot()
{
    if( lataaKoon_)
        return;
    if( ui->valintaTab->currentIndex() == VIENNIT)
        kp()->settings()->setValue("SelausViennit", ui->selausView->horizontalHeader()->saveState());
    else
        kp()->settings()->setValue("SelausTositteet", ui->selausView->horizontalHeader()->saveState());
}

void SelausWg::lataaKoot()
{
    lataaKoon_ = true;
    if( ui->valintaTab->currentIndex() == VIENNIT ) {
        if( kp()->settings()->contains("SelausViennit"))
            ui->selausView->horizontalHeader()->restoreState(kp()->settings()->value("SelausViennit").toByteArray());
    } else {
        if( kp()->settings()->contains("SelausTositteet"))
            ui->selausView->horizontalHeader()->restoreState(kp()->settings()->value("SelausTositteet").toByteArray());
    }
    lataaKoon_ = false;
}

void SelausWg::nuoliSelaus(bool seuraava)
{
    QDate alku = ui->alkuEdit->date();
    QDate loppu = ui->loppuEdit->date();

    if( alku.day() == 1 && loppu.addDays(1).day() == 1 && alku.daysTo(loppu) < 32) {
        // Kuukausittain
        if( seuraava ) {
            alku = alku.addMonths(1);
            loppu = alku.addMonths(1).addDays(-1);
        } else {
            loppu = alku.addDays(-1);
            alku = alku.addMonths(-1);
        }
    } else {
        // Tilikausittain
        Tilikausi kausi = Kirjanpito::db()->tilikaudet()->tilikausiPaivalle( seuraava ? loppu.addDays(1) : alku.addDays(-1) );
        alku = kausi.alkaa();
        loppu = kausi.paattyy();
    }

    ui->alkuEdit->setDate(alku);
    ui->loppuEdit->setDate(loppu);
    paivita();
}

void SelausWg::tamaKuukausi()
{
    QDate pvm = ui->alkuEdit->date();
    ui->alkuEdit->setDate(QDate(pvm.year(), pvm.month(), 1));
    ui->loppuEdit->setDate(QDate(pvm.year(), pvm.month(), pvm.daysInMonth()));
    paivita();
}

void SelausWg::tamaTilikausi()
{
    Tilikausi tilikausi = kp()->tilikausiPaivalle(ui->alkuEdit->date());
    if(tilikausi.alkaa().isValid()) {
        ui->alkuEdit->setDate(tilikausi.alkaa());
        ui->loppuEdit->setDate(tilikausi.paattyy());
        paivita();
    }
}

void SelausWg::palautaValinta()
{
    if( ui->selausView->model() && ui->selausView->isVisible()) {

        if(valittu_) {
            for(int i=0; i < ui->selausView->model()->rowCount(); i++) {
                if( ui->selausView->model()->index(i,0).data(Qt::UserRole).toInt() == valittu_) {
                    ui->selausView->selectRow(i);
                    break;
                }
            }
        }
        if( skrolli_ > -1) {
            ui->selausView->verticalScrollBar()->setSliderPosition(skrolli_);
            skrolli_ = -1;
        }
    }
}

void SelausWg::poistoMoodiin(bool poistoon)
{
    ui->poistoGroup->setVisible(poistoon);
    ui->selausView->setSelectionMode(poistoon ? QTableView::MultiSelection : QTableView::SingleSelection);
    ui->poistaNappi->setChecked( poistoon );
    if( poistoon ) {
        ui->selausView->clearSelection();
        paivitaPoistoOhje();
        connect( ui->selausView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SelausWg::paivitaPoistoOhje);
    }

    if( !poistoon && ui->selausView->selectionModel()->selectedRows().count() > 1)
        ui->selausView->clearSelection();
}

void SelausWg::paivitaPoistoOhje()
{
    if( !ui->poistoGroup->isVisible()) return;

    QModelIndexList valitutRivit = ui->selausView->selectionModel()->selectedRows(TositeSelausModel::PVM);

    const QDate lukittuPvm = kp()->tilitpaatetty();

    for(const auto& item : valitutRivit) {
        const QDate pvm = item.data(Qt::DisplayRole).toDate();
        if( pvm <= lukittuPvm) {
            ui->poistoLabel->setText( tr("Tositteita ei voi poistaa lukitulta tilikaudelta"));
            ui->teePoistoNappi->setEnabled(false);
            return;
        }
    }


    if( valitutRivit.empty()) {
        ui->poistoLabel->setText(tr("Valitse poistettavat tositteet"));
    } else {
        ui->poistoLabel->setText(tr("Haluatko todella poistaa valitsemasi %1 tositetta?").arg(valitutRivit.count()));
    }
    ui->teePoistoNappi->setEnabled(!valitutRivit.empty());
}

void SelausWg::teePoisto()
{
    QModelIndexList valitutRivit = ui->selausView->selectionModel()->selectedRows();
    poistoLaskuri_ += valitutRivit.count();
    for(const auto& rivi: valitutRivit) {
        KpKysely* kysely = kpk(QString("/tositteet/%1").arg(rivi.data(TositeSelausModel::TositeIdRooli).toInt()), KpKysely::DELETE);
        connect(kysely, &KpKysely::vastaus, this, &SelausWg::poistoValmis);
        kysely->kysy();
    }
}

void SelausWg::poistoValmis()
{
    poistoLaskuri_--;
    if( poistoLaskuri_ < 1)
        emit kp()->kirjanpitoaMuokattu();
}

bool SelausWg::eventFilter(QObject *watched, QEvent *event)
{
    if( watched == ui->selausView && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if( keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return )
            naytaTositeRivilta( ui->selausView->selectionModel()->currentIndex() );
    }
    return false;
}

