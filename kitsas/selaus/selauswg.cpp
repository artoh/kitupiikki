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

#include <QDebug>

#include "tositeselausmodel.h"
#include "laskutus/laskudialogi.h"

SelausWg::SelausWg(QWidget *parent) :
    KitupiikkiSivu(parent),
    ui(new Ui::SelausWg)
{
    ui->setupUi(this);

    ui->valintaTab->addTab(QIcon(":/pic/tekstisivu.png"),tr("&Tositteet"));
    ui->valintaTab->addTab(QIcon(":/pic/inbox.png"), tr("&Saapuneet"));
    ui->valintaTab->addTab(QIcon(":/pic/harmaa.png"), tr("&Luonnokset"));
    ui->valintaTab->addTab(QIcon(":/pic/vientilista.png"),tr("&Viennit"));

    model = new SelausModel(this);
    tositeModel = new TositeSelausModel(this);

    // Proxyä käytetään tilien tai tositelajien suodattamiseen
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);

    etsiProxy = new QSortFilterProxyModel(this);
    etsiProxy->setSortRole(Qt::EditRole);  // Jotta numerot lajitellaan oikein
    etsiProxy->setFilterKeyColumn( SelausModel::TILI);
    etsiProxy->setSortLocaleAware(true);
    etsiProxy->setSourceModel(proxyModel);
    etsiProxy->setFilterKeyColumn( SelausModel::SELITE );
    etsiProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->selausView->setModel( etsiProxy );

    ui->selausView->horizontalHeader()->setStretchLastSection(true);
    ui->selausView->verticalHeader()->hide();

    ui->selausView->sortByColumn(SelausModel::PVM, Qt::AscendingOrder);

    connect( ui->etsiEdit, SIGNAL(textChanged(QString)), etsiProxy, SLOT(setFilterFixedString(QString)));

    connect( ui->alkuEdit, SIGNAL(editingFinished()), this, SLOT(paivita()));
    connect( ui->loppuEdit, SIGNAL(editingFinished()), this, SLOT(paivita()));
    connect( ui->tiliCombo, SIGNAL(currentTextChanged(QString)), this, SLOT(suodata()));
    connect( ui->selausView, SIGNAL(clicked(QModelIndex)), this, SLOT(naytaTositeRivilta(QModelIndex)));

    ui->valintaTab->setCurrentIndex(0);     // Oletuksena tositteiden selaus
    connect( ui->valintaTab, SIGNAL(currentChanged(int)), this, SLOT(selaa(int)));

    connect( Kirjanpito::db(), SIGNAL(kirjanpitoaMuokattu()), this, SLOT(paivita()));
    connect( kp(), &Kirjanpito::tilikausiAvattu, this, &SelausWg::alusta);
    connect( kp(), SIGNAL(tietokantaVaihtui()), this, SLOT(alusta()));

    connect( ui->alkuEdit, SIGNAL(dateChanged(QDate)), this, SLOT(alkuPvmMuuttui()));
    connect( kp(), &Kirjanpito::tilikausiAvattu, this, &SelausWg::alusta);

    connect( proxyModel, &QSortFilterProxyModel::modelReset, ui->selausView, &QTableView::resizeColumnsToContents);
    connect( proxyModel, &QSortFilterProxyModel::modelReset, this, &SelausWg::valitseValittu);

    ui->selausView->installEventFilter(this);
    ui->selausView->horizontalHeader()->setStretchLastSection(true);

    connect( etsiProxy, &QSortFilterProxyModel::modelReset , this, &SelausWg::paivitaSuodattimet );
    connect( etsiProxy, &QSortFilterProxyModel::modelReset , [this] { this->paivitaSummat(); });

    connect( kp(), &Kirjanpito::tilikausiAvattu, [this] { this->ui->loppuEdit->setDateRange(kp()->tilikaudet()->kirjanpitoAlkaa(), kp()->tilikaudet()->kirjanpitoLoppuu()); });

}

SelausWg::~SelausWg()
{
    delete ui;
}

QPair<int, int> SelausWg::edellinenSeuraava(int tositeId)
{
    for(int i=0; i < proxyModel->rowCount(); i++) {
        if( proxyModel->index(i,0).data(Qt::UserRole).toInt() == tositeId ) {
            int edellinen = 0;
            int seuraava = 0;
            if( i > 0)
                edellinen = proxyModel->index(i-1,0).data(Qt::UserRole).toInt();
            if( i < proxyModel->rowCount()-1)
                seuraava = proxyModel->index(i+1,0).data(Qt::UserRole).toInt();
            return qMakePair(edellinen, seuraava);
        }
    }
    return qMakePair(0,0);
}

void SelausWg::alusta()
{
    QDate alku = Kirjanpito::db()->tilikaudet()->kirjanpitoAlkaa();
    QDate loppu = Kirjanpito::db()->tilikaudet()->kirjanpitoLoppuu();

    QDate pvm = kp()->db()->paivamaara();
    if( pvm > loppu)
        pvm = loppu;

    Tilikausi nytkausi = Kirjanpito::db()->tilikaudet()->tilikausiPaivalle( pvm );

    QDate nytalkaa = nytkausi.alkaa();
    QDate nytloppuu = nytkausi.paattyy();


    ui->alkuEdit->setDateRange(alku, loppu);
    ui->loppuEdit->setDateRange(alku, loppu);
    ui->alkuEdit->setDate(nytalkaa);
    ui->loppuEdit->setDate(nytloppuu);

}

void SelausWg::paivita()
{
    bool lopussa = ui->selausView->verticalScrollBar()->value() >=
            ui->selausView->verticalScrollBar()->maximum() - ui->selausView->verticalScrollBar()->pageStep();

    if( ui->valintaTab->currentIndex() == VIENNIT )
    {
        model->lataa( ui->alkuEdit->date(), ui->loppuEdit->date());
    }
    else if( ui->valintaTab->currentIndex() == SAAPUNEET) {
        tositeModel->lataa( ui->alkuEdit->date(), ui->loppuEdit->date(), TositeSelausModel::SAAPUNEET);
    }
    else if( ui->valintaTab->currentIndex() == TOSITTEET )
    {
        tositeModel->lataa( ui->alkuEdit->date(), ui->loppuEdit->date());
    } else if( ui->valintaTab->currentIndex() == LUONNOKSET){
        tositeModel->lataa( ui->alkuEdit->date(), ui->loppuEdit->date(), TositeSelausModel::LUONNOKSET);
    }

    if( lopussa )
        ui->selausView->verticalScrollBar()->setValue( ui->selausView->verticalScrollBar()->maximum() );
}

void SelausWg::suodata()
{
    if( ui->tiliCombo->currentData().toString() == "*")
        proxyModel->setFilterFixedString(QString());
    else
        proxyModel->setFilterFixedString( ui->tiliCombo->currentText());    
    paivitaSummat();


    // Jos haetaan tilin tapahtumia, näytetään myös tilin loppusaldo
    QString teksti = ui->tiliCombo->currentText();
    int numero = teksti.left(teksti.indexOf(' ')).toInt();

    if( ui->valintaTab->currentIndex() == VIENNIT && numero ) {
        KpKysely *saldokysely = kpk("/saldot");
        saldokysely->lisaaAttribuutti("pvm",ui->loppuEdit->date());
        saldokysely->lisaaAttribuutti("tili", numero );
        connect( saldokysely, &KpKysely::vastaus, this, &SelausWg::paivitaSummat);
        saldokysely->kysy();
    }
}

void SelausWg::paivitaSuodattimet()
{
    if( ui->valintaTab->currentIndex() == VIENNIT)
    {
        QString valittu = ui->tiliCombo->currentText();
        ui->tiliCombo->clear();
        ui->tiliCombo->insertItem(0, QIcon(":/pic/Possu64.png"),"Kaikki tilit", QVariant("*"));
        ui->tiliCombo->insertItems(1, model->kaytetytTilit());
        ui->tiliCombo->setCurrentText(valittu);
    } else {
        QString valittu = ui->tiliCombo->currentText();
        ui->tiliCombo->clear();
        ui->tiliCombo->insertItem(0, QIcon(":/pic/Possu64.png"),"Kaikki tositteet", QVariant("*"));
        for( int tyyppikoodi : tositeModel->tyyppiLista() ) {
            ui->tiliCombo->addItem( kp()->tositeTyypit()->kuvake(tyyppikoodi), kp()->tositeTyypit()->nimi(tyyppikoodi) );
        }

        ui->tiliCombo->setCurrentText(valittu);
    }
}

void SelausWg::paivitaSummat(QVariant *data)
{
    if( ui->valintaTab->currentIndex() == VIENNIT)
    {
        double debetSumma = 0;
        double kreditSumma = 0;

        for(int i=0; i<proxyModel->rowCount(QModelIndex()); i++)
        {
            debetSumma += proxyModel->index(i, SelausModel::DEBET).data(Qt::EditRole).toDouble();
            kreditSumma += proxyModel->index(i, SelausModel::KREDIT).data(Qt::EditRole).toDouble();
        }

        if( data && data->toMap().count()) {
            double saldo = data->toMap().first().toDouble();
            ui->summaLabel->setText( tr("Debet %L1 €  Kredit %L2 €\nLoppusaldo %L3 €")
                    .arg( debetSumma,0,'f',2)
                    .arg(kreditSumma,0,'f',2)
                    .arg(saldo,0,'f',2));
        } else

        ui->summaLabel->setText( tr("Debet %L1 €\tKredit %L2 €")
                .arg( debetSumma,0,'f',2)
                .arg(kreditSumma,0,'f',2) );

        // Alkusaldon ja loppusaldon käsittely vaatii vielä kyselyn ;)




    } else {

        // Lasketaan summat
        double summa = 0;
        for(int i=0; i<proxyModel->rowCount(QModelIndex()); i++)
        {
            summa += proxyModel->index(i, TositeSelausModel::SUMMA).data(Qt::EditRole).toDouble();
        }
        ui->summaLabel->setText( tr("Summa %L1€").arg(summa,0,'f',2));

    }

}

void SelausWg::naytaTositeRivilta(QModelIndex index)
{
    int id = index.data( Qt::UserRole).toInt();
    int tyyppi = index.data(TositeSelausModel::TositeTyyppiRooli).toInt();

    if( tyyppi >= TositeTyyppi::MYYNTILASKU && tyyppi < TositeTyyppi::SIIRTO) {
        KpKysely *kysely = kpk( QString("/tositteet/%1").arg(id));
        connect( kysely, &KpKysely::vastaus, [](QVariant* data) {
            LaskuDialogi* dlg = new LaskuDialogi( data->toMap()  );
            dlg->show();
        });
        kysely->kysy();
    } else
        emit tositeValittu( id );
    valittu_ = id;
}

void SelausWg::selaa(int tilinumero, const Tilikausi& tilikausi)
{
    // Ohjelmallisesti selaa tiettynä tilikautena tiettyä tiliä
    ui->alkuEdit->setDate( tilikausi.alkaa());
    ui->loppuEdit->setDate( tilikausi.paattyy());

    ui->valintaTab->setCurrentIndex(VIENNIT);


    paivita();

    Tili selattava = Kirjanpito::db()->tilit()->tiliNumerolla(tilinumero);

    ui->tiliCombo->setCurrentText(QString("%1 %2").arg(selattava.numero() ).arg(selattava.nimi()));

}

void SelausWg::selaaVienteja()
{

    proxyModel->setSourceModel(model);
    proxyModel->setFilterKeyColumn( SelausModel::TILI);
    etsiProxy->setSortRole(Qt::EditRole);  // Jotta numerot lajitellaan oikein
    etsiProxy->setSourceModel(proxyModel);
    etsiProxy->setFilterKeyColumn( SelausModel::SELITE );

    paivita();
}

void SelausWg::selaaTositteita()
{

    proxyModel->setSourceModel(tositeModel);
    proxyModel->setFilterKeyColumn( TositeSelausModel::TOSITETYYPPI);
    etsiProxy->setSortRole(Qt::EditRole);  // Jotta numerot lajitellaan oikein
    etsiProxy->setSourceModel(proxyModel);
    etsiProxy->setFilterKeyColumn( TositeSelausModel::OTSIKKO );

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
    if( kumpi == VIENNIT)
        selaaVienteja();
    else
        selaaTositteita();

    ui->selausView->setFocus();
}

void SelausWg::siirrySivulle()
{

    selaa( ui->valintaTab->currentIndex() );
}

void SelausWg::valitseValittu()
{
    for(int i=0; i < ui->selausView->model()->rowCount(); i++) {
        if( ui->selausView->model()->index(i,0).data(Qt::UserRole).toInt() == valittu_) {
            ui->selausView->selectRow(i);
            return;
        }
    }
    ui->selausView->selectRow(0);
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

