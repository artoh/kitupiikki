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

#include "lisaikkuna.h"

#include "tositeselausmodel.h"
#include "laskutus/laskudialogi.h"

SelausWg::SelausWg(QWidget *parent) :
    KitupiikkiSivu(parent),
    ui(new Ui::SelausWg)
{
    ui->setupUi(this);

    ui->valintaTab->addTab(QIcon(":/pic/tekstisivu.png"),tr("&Tositteet"));
    ui->valintaTab->addTab(QIcon(":/pic/harmaa.png"), tr("&Luonnokset"));
    ui->valintaTab->addTab(QIcon(":/pic/vientilista.png"),tr("&Viennit"));
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
    ui->selausView->setModel(tositeProxy_);
    ui->selausView->setWordWrap(false);

    ui->selausView->sortByColumn(SelausModel::PVM, Qt::AscendingOrder);

    connect( ui->alkuEdit, SIGNAL(editingFinished()), this, SLOT(paivita()));
    connect( ui->loppuEdit, SIGNAL(editingFinished()), this, SLOT(paivita()));
    connect( ui->tiliCombo, SIGNAL(currentTextChanged(QString)), this, SLOT(suodata()));
    connect( ui->etsiEdit, &QLineEdit::textChanged, this, &SelausWg::etsi);
    connect( ui->selausView, SIGNAL(clicked(QModelIndex)), this, SLOT(naytaTositeRivilta(QModelIndex)));

    ui->valintaTab->setCurrentIndex(0);     // Oletuksena tositteiden selaus
    connect( ui->valintaTab, SIGNAL(currentChanged(int)), this, SLOT(selaa(int)));

    connect( Kirjanpito::db(), SIGNAL(kirjanpitoaMuokattu()), this, SLOT(paivita()));
    connect( kp()->tilikaudet(), &TilikausiModel::modelReset, this, &SelausWg::alusta);

    connect( ui->alkuEdit, SIGNAL(dateChanged(QDate)), this, SLOT(alkuPvmMuuttui()));

    ui->selausView->installEventFilter(this);


    connect( kp(), &Kirjanpito::tilikausiAvattu, [this] {
        this->ui->loppuEdit->setDateRange(kp()->tilikaudet()->kirjanpitoAlkaa(), kp()->tilikaudet()->kirjanpitoLoppuu()); });
    connect( ui->paivitaNappi, &QPushButton::clicked, this, &SelausWg::paivita);

    connect( ui->selausView, &QTableView::customContextMenuRequested, this, &SelausWg::contextMenu);
}

SelausWg::~SelausWg()
{
    delete ui;
}

QPair<int, int> SelausWg::edellinenSeuraava(int tositeId)
{
    QAbstractItemModel *model = ui->selausView->model();
    for(int i=0; i < model->rowCount(); i++) {
        if( model->index(i,0).data(Qt::UserRole).toInt() == tositeId ) {
            int edellinen = 0;
            int seuraava = 0;
            if( i > 0)
                edellinen = model->index(i-1,0).data(Qt::UserRole).toInt();
            if( i < model->rowCount()-1)
                seuraava = model->index(i+1,0).data(Qt::UserRole).toInt();
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


    ui->alkuEdit->setDate(nytalkaa);
    ui->loppuEdit->setDate(nytloppuu);
    ui->alkuEdit->setDateRange(alku, loppu);
    ui->loppuEdit->setDateRange(alku, loppu);


    paivita();

}

void SelausWg::paivita()
{    

    qApp->processEvents();
    lopussa_ = ui->selausView->verticalScrollBar()->value() >=
            ui->selausView->verticalScrollBar()->maximum() - ui->selausView->verticalScrollBar()->pageStep();

    if( ui->valintaTab->currentIndex() == VIENNIT )
    {
        kp()->odotusKursori(true);
        if(selaustili_)
            selausProxy_->suodataTililla(0);
        model->lataa( ui->alkuEdit->date(), ui->loppuEdit->date(), selaustili_);
    }
    else if( ui->valintaTab->currentIndex() == SAAPUNEET) {
        kp()->odotusKursori(true);
        tositeModel->lataa( ui->alkuEdit->date(), ui->loppuEdit->date(), TositeSelausModel::SAAPUNEET);
    }
    else if( ui->valintaTab->currentIndex() == TOSITTEET )
    {
        kp()->odotusKursori(true);
        tositeModel->lataa( ui->alkuEdit->date(), ui->loppuEdit->date());
    } else if( ui->valintaTab->currentIndex() == LUONNOKSET){
        kp()->odotusKursori(true);
        tositeModel->lataa( ui->alkuEdit->date(), ui->loppuEdit->date(), TositeSelausModel::LUONNOKSET);
    }

}

void SelausWg::suodata()
{
    QVariant suodatin = ui->tiliCombo->currentData();
    if( suodatin.isNull())
        return;

    if( ui->valintaTab->currentIndex() == VIENNIT) {
        if(selaustili_ && suodatin.isValid() && suodatin.toInt() != selaustili_) {
            selaustili_ = 0;
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
        KpKysely *saldokysely = kpk("/saldot");
        saldokysely->lisaaAttribuutti("pvm",ui->loppuEdit->date());
        saldokysely->lisaaAttribuutti("tili", suodatin.toInt() );
        connect( saldokysely, &KpKysely::vastaus, this, &SelausWg::paivitaSummat);
        saldokysely->kysy();
    }
}

void SelausWg::paivitaSuodattimet()
{
    if( ui->valintaTab->currentIndex() == VIENNIT)
    {
        if(selaustili_) {
            ui->tiliCombo->clear();
            ui->tiliCombo->addItem(QString("%1 %2").arg(selaustili_).arg(kp()->tilit()->nimi(selaustili_)), selaustili_);
            ui->tiliCombo->setCurrentText(QString("%1 %2").arg(selaustili_).arg(kp()->tilit()->nimi(selaustili_)));
            ui->tiliCombo->insertItem(0, QIcon(":/pic/Possu64.png"),"Kaikki tilit", 0);

        } else {
            QString valittu = ui->tiliCombo->currentText();
            ui->tiliCombo->clear();
            ui->tiliCombo->insertItem(0, QIcon(":/pic/Possu64.png"),"Kaikki tilit", 0);
            for(int tiliNro : model->tiliLista()) {
                ui->tiliCombo->addItem(QString("%1 %2").arg(tiliNro).arg(kp()->tilit()->nimi(tiliNro)), tiliNro);
            }
            if( !valittu.isEmpty())
                ui->tiliCombo->setCurrentText(valittu);
        }
    } else {
        QString valittu = ui->tiliCombo->currentText();
        ui->tiliCombo->clear();
        ui->tiliCombo->insertItem(0, QIcon(":/pic/Possu64.png"),"Kaikki tositteet", -1);
        for( int tyyppikoodi : tositeModel->tyyppiLista() ) {
            ui->tiliCombo->addItem( kp()->tositeTyypit()->kuvake(tyyppikoodi), kp()->tositeTyypit()->nimi(tyyppikoodi), tyyppikoodi );
        }
        QStringList sarjat = tositeModel->sarjaLista();
        if(!sarjat.isEmpty()) {
            ui->tiliCombo->insertSeparator(ui->tiliCombo->count());
            for(QString sarja : sarjat) {
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
        qlonglong debetSumma = 0;
        qlonglong kreditSumma = 0;



        for(int i=0; i< model->rowCount(QModelIndex()); i++)
        {
            debetSumma += qRound64( model->index(i, SelausModel::DEBET).data(Qt::EditRole).toDouble() * 100);
            kreditSumma += qRound64(model->index(i, SelausModel::KREDIT).data(Qt::EditRole).toDouble() * 100);
        }

        if( data && data->toMap().count()) {
            saldo_ = data->toMap().first().toDouble();

        }

        if( qAbs(saldo_) > 1e-5) {
            ui->summaLabel->setText( tr("Debet %L1 €  Kredit %L2 €\nLoppusaldo %L3 €")
                    .arg( debetSumma / 100.0,0,'f',2)
                    .arg(kreditSumma / 100.0,0,'f',2)
                    .arg(saldo_,0,'f',2));
        } else {
            ui->summaLabel->setText( tr("Debet %L1 €\tKredit %L2 €")
                    .arg( debetSumma / 100.0,0,'f',2)
                    .arg(kreditSumma / 100.0,0,'f',2) );
        }


    } else {

        // Lasketaan summat
        qlonglong summa = 0;
        for(int i=0; i<model->rowCount(QModelIndex()); i++)
        {
            summa += qRound64(model->index(i, TositeSelausModel::SUMMA).data(Qt::EditRole).toDouble() * 100);
        }
        ui->summaLabel->setText( tr("Summa %L1€").arg(summa / 100.0,0,'f',2));

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

    if( kumpi == VIENNIT) {
        ui->selausView->setModel(selausProxy_);
    } else
        ui->selausView->setModel(tositeProxy_);

    paivita();
    ui->selausView->setFocus();
//    paivitaSuodattimet();
//    modelResetoitu();
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
        if( ui->valintaTab->count() == 4)
            ui->valintaTab->removeTab(SAAPUNEET);
    } else {
        if( ui->valintaTab->count() == 3)
            ui->valintaTab->addTab(QIcon(":/pic/inbox.png"), tr("&Saapuneet"));
    }

    selaa( ui->valintaTab->currentIndex() );
}

void SelausWg::modelResetoitu()
{


    if( ui->selausView->model() && ui->selausView->isVisible()) {

        paivitaSummat();
        qApp->processEvents();
        int riveja =  ui->selausView->model()->rowCount();

        if( riveja > 0 && riveja <= 100) {

            ui->selausView->resizeColumnToContents(0);

            if( ui->valintaTab->currentIndex()==VIENNIT) {
                ui->selausView->resizeColumnToContents(SelausModel::TILI);
                ui->selausView->resizeColumnToContents(SelausModel::KUMPPANI);
                ui->selausView->resizeColumnToContents(SelausModel::DEBET);
                ui->selausView->resizeColumnToContents(SelausModel::KREDIT);
            } else {
                ui->selausView->resizeColumnToContents(TositeSelausModel::TOSITETYYPPI);
                ui->selausView->resizeColumnToContents(TositeSelausModel::ASIAKASTOIMITTAJA);
                ui->selausView->resizeColumnToContents(TositeSelausModel::SUMMA);
            }

        }

        if(valittu_) {
            for(int i=0; i < ui->selausView->model()->rowCount(); i++) {
                if( ui->selausView->model()->index(i,0).data(Qt::UserRole).toInt() == valittu_) {
                    ui->selausView->selectRow(i);                    
                    break;
                }
            }
        }
        paivitaSuodattimet();
        if( lopussa_ )
            ui->selausView->verticalScrollBar()->setValue( ui->selausView->verticalScrollBar()->maximum() );

    }
    kp()->odotusKursori(false);


}

void SelausWg::etsi(const QString &teksti)
{
    if( ui->valintaTab->currentIndex() == VIENNIT)
        selausProxy_->etsi(teksti);
    else
        tositeProxy_->etsi(teksti);
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

