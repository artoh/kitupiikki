
/*
   Copyright (C) 2017,2018 Arto Hyvättinen

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

#include <QDebug>
#include <QSettings>
#include <QKeyEvent>

#include "tilinvalintadialogi.h"
#include "ui_tilinvalintadialogi.h"

#include "kirjanpito.h"

TilinValintaDialogi::TilinValintaDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TilinValintaDialogi)
{
    ui->setupUi(this);

    lajitteluProxy_ = new QSortFilterProxyModel(this);
    filtteri_ = new TilivalintaDialogiFiltteri(this);
    filtteri_->setSourceModel(lajitteluProxy_);
    asetaModel( kp()->tilit());


    ui->view->setModel(filtteri_);
    ui->view->setColumnHidden(TiliModel::NRONIMI, true);

    ui->view->setColumnHidden(TiliModel::ALV, ! kp()->asetukset()->onko("AlvVelvollinen"));

    ui->view->horizontalHeader()->setSectionResizeMode(TiliModel::NIMI, QHeaderView::Stretch);

    ui->infoMerkki->setVisible(false);
    ui->ohjeLabel->setVisible(false);

    connect( ui->suosikitNappi, &QPushButton::clicked,
             this, &TilinValintaDialogi::suodataSuosikit);
    connect( ui->kaikkiNappi, &QPushButton::clicked,
             this, &TilinValintaDialogi::naytaKaikki);
    connect( ui->suodatusEdit, &QLineEdit::textChanged,
             this, &TilinValintaDialogi::teeSuodatus);
    connect( ui->view, SIGNAL(clicked(QModelIndex)),
             this, SLOT(klikattu(QModelIndex)));
    connect( ui->view->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
             this, SLOT(valintaMuuttui(QModelIndex)));
    connect( ui->view, &QTableView::entered,
             this, &TilinValintaDialogi::naytaOhje);
    connect( ui->saldoButton, &QPushButton::clicked, this, &TilinValintaDialogi::naytaSaldolliset);
    connect( ui->otsikotButton, &QPushButton::toggled, filtteri_, &TilivalintaDialogiFiltteri::naytaOtsikot);

    int viimetila = kp()->settings()->value("TilinvalintaTila").toInt();
    if( viimetila == TilivalintaDialogiFiltteri::SUOSIKIT)
        ui->suosikitNappi->setChecked(true);
    else if(viimetila == TilivalintaDialogiFiltteri::KAIKKI)
        ui->kaikkiNappi->setChecked(true);
    else if(viimetila == TilivalintaDialogiFiltteri::KIRJATTU)
        ui->saldoButton->setChecked(true);
    filtteri_->suodataTilalla(viimetila);
    filtteri_->naytaOtsikot(kp()->settings()->value("TilinvalintaOtsikot", true).toBool());

    ui->view->setMouseTracking(true);

    ui->suodatusEdit->installEventFilter(this);
    restoreGeometry( kp()->settings()->value("TilinValintaDlg").toByteArray());

    kp()->tilit()->haeSaldot();
}

TilinValintaDialogi::~TilinValintaDialogi()
{
    kp()->settings()->setValue("TilinValintaDlg", saveGeometry());
    kp()->settings()->setValue("TilinvalintaTila", filtteri_->suodatusTila());
    kp()->settings()->setValue("TilinvalintaOtsikot", ui->otsikotButton->isChecked());
    delete ui;
}

Tili TilinValintaDialogi::valittu() const
{
    int tiliNumero = ui->view->currentIndex().data(TiliModel::NroRooli).toInt();
    return kp()->tilit()->tiliNumerolla(tiliNumero);
}

void TilinValintaDialogi::accept()
{
    int tiliNumero = ui->view->currentIndex().data(TiliModel::NroRooli).toInt();
    if (tiliNumero) {
        emit tiliValittu(tiliNumero);
    }
    QDialog::accept();
}

void TilinValintaDialogi::suodata(const QString &alku)
{
    ui->suodatusEdit->setText(alku);
}


void TilinValintaDialogi::teeSuodatus(const QString &alku)
{    
    filtteri_->suodataTekstilla(alku);
    if( ui->view->selectionModel()->selectedIndexes().isEmpty())
        ui->view->setCurrentIndex( ui->view->model()->index(0,0) );
    alaValitseOtsikoita(1);
}

void TilinValintaDialogi::suodataTyyppi(const QString &regexp)
{
    alkuperainenTyyppiSuodatin = regexp;
    filtteri_->suodataTyypilla(regexp);
    alaValitseOtsikoita(1);
}

void TilinValintaDialogi::suodataSuosikit(bool suodatetaanko)
{
    if( suodatetaanko)
    {
        ui->kaikkiNappi->setChecked(false);
        ui->saldoButton->setChecked(false);
        filtteri_->suodataTilalla(TilivalintaDialogiFiltteri::SUOSIKIT);
    }
    else
        filtteri_->suodataTilalla(TilivalintaDialogiFiltteri::KAYTOSSA);
}

void TilinValintaDialogi::naytaKaikki(bool naytetaanko)
{
    if( naytetaanko) {
        ui->suosikitNappi->setChecked(false);
        ui->saldoButton->setChecked(false);
        filtteri_->suodataTilalla(TilivalintaDialogiFiltteri::KAIKKI);
        filtteri_->suodataTyypilla(".*");
    } else {
        filtteri_->suodataTilalla(TilivalintaDialogiFiltteri::KAYTOSSA);
        filtteri_->suodataTyypilla(alkuperainenTyyppiSuodatin);
    }
}

void TilinValintaDialogi::asetaModel(TiliModel *model)
{
    tiliModel = model;
    lajitteluProxy_->setSourceModel( model );
}

void TilinValintaDialogi::valitse(int tilinumero)
{
    QString tilistr = QString::number(tilinumero);
    for(int i=0; i < ui->view->model()->rowCount(); i++)
    {
        if( ui->view->model()->data( ui->view->model()->index(i,0), TiliModel::NroRooli ).toString() >= tilistr) {

            ui->view->selectRow(i);
            break;
        }
    }
}

void TilinValintaDialogi::klikattu(const QModelIndex &index)
{
    if(index.data(TiliModel::OtsikkotasoRooli).toInt() == 0 && index.data(TiliModel::TyyppiRooli).toString() != "T")
    {
        // Valittu tili eikä otsikkoa
        accept();
    }
}

void TilinValintaDialogi::valintaMuuttui(const QModelIndex &index)
{
    ui->valitseNappi->setEnabled( index.isValid() && index.data( TiliModel::OtsikkotasoRooli ).toInt() == 0
                                  && index.data(TiliModel::TyyppiRooli).toString() != "T");
    naytaOhje( index );
}

void TilinValintaDialogi::naytaSaldolliset(bool naytetaanko)
{
    if( naytetaanko) {
        ui->suosikitNappi->setChecked(false);
        ui->kaikkiNappi->setChecked(false);
        filtteri_->suodataTilalla(TilivalintaDialogiFiltteri::KIRJATTU);
        filtteri_->suodataTyypilla(".*");
    } else {
        filtteri_->suodataTilalla(TilivalintaDialogiFiltteri::KAYTOSSA);
        filtteri_->suodataTyypilla(alkuperainenTyyppiSuodatin);
    }
}


void TilinValintaDialogi::naytaOhje(const QModelIndex &index)
{
    QString ohje = index.data(TiliModel::OhjeRooli).toString();

    ui->infoMerkki->setVisible( !ohje.isEmpty() );
    ui->ohjeLabel->setVisible( !ohje.isEmpty() );
    ui->ohjeLabel->setText( ohje );
}

void TilinValintaDialogi::alaValitseOtsikoita(int suunta)
{
    if( suunta > 0 )
    {
        while( ui->view->currentIndex().row() < ui->view->model()->rowCount(QModelIndex()) - 1 &&
               ( ui->view->currentIndex().data( TiliModel::OtsikkotasoRooli).toInt() > 0
                || ui->view->currentIndex().data(TiliModel::TyyppiRooli).toString() == "T" ) )
            ui->view->selectRow( ui->view->currentIndex().row() + 1 );            
    }
    else
    {
        while( ui->view->currentIndex().row() > 1 &&
               ( ui->view->currentIndex().data( TiliModel::OtsikkotasoRooli).toInt() > 0
               || ui->view->currentIndex().data(TiliModel::TyyppiRooli).toString() == "T" ))
            ui->view->selectRow( ui->view->currentIndex().row() - 1 );
    }
}

bool TilinValintaDialogi::eventFilter(QObject *object, QEvent *event)
{
    if( object == ui->suodatusEdit &&            event->type() == QEvent::KeyPress )
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if( keyEvent->key() == Qt::Key_Down )
        {
            ui->view->selectRow( ui->view->currentIndex().row()+1 );
            alaValitseOtsikoita(1);
            return true;
        }
        else if( keyEvent->key() == Qt::Key_Up )
        {
            ui->view->selectRow( ui->view->currentIndex().row()-1 );
            alaValitseOtsikoita(-1);
            return true;
        }


    }
    return QDialog::eventFilter(object, event);
}
/*
void TilinValintaDialogi::nayta(const QString &alku, const QString &suodatus)
{
    qDebug() << alku;

    if( alku.startsWith("*") ) {
        valitse( alku.mid(1).toInt());
    } else {
        ui->suodatusEdit->setText(alku);
    }
    suodataTyyppi(suodatus);

    exec();
    setAttribute(Qt::WA_DeleteOnClose);
}
*/



Tili TilinValintaDialogi::valitseTili(const QString &alku, const QString &tyyppiSuodatin, TiliModel *model)
{

    TilinValintaDialogi dlg;
    if( model )
        dlg.asetaModel( model );

    if( alku.left(1) == '*')
    {
        // Jos alku on * + numero, etsitään tili kyseisellä numerolla
        dlg.valitse( alku.mid(1).toInt());
    } else {
        dlg.ui->suodatusEdit->setText(alku);
    }
    dlg.suodataTyyppi(tyyppiSuodatin);

    if( dlg.exec())
    {
        return dlg.valittu();
    } else if(alku.left(1) == "*"  ){
        return model ? model->tiliNumerolla( alku.mid(1).toInt() ) : kp()->tilit()->tiliNumerolla( alku.mid(1).toInt()) ;
    }
    return Tili();
}

