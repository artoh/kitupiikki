
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

    proxyNimi = new QSortFilterProxyModel(this);
    asetaModel( kp()->tilit());
    proxyNimi->setFilterRole( TiliModel::NimiRooli);
    proxyNimi->setFilterCaseSensitivity(Qt::CaseInsensitive);

    proxyTyyppi = new QSortFilterProxyModel(this);
    proxyTyyppi->setSourceModel(proxyNimi);
    proxyTyyppi->setFilterRole(TiliModel::TyyppiRooli);
    proxyTyyppi->setSortRole(TiliModel::LajitteluRooli);
    proxyTyyppi->sort(TiliModel::NUMERO);

    proxyTila = new QSortFilterProxyModel(this);
    proxyTila->setSourceModel(proxyTyyppi);
    proxyTila->setFilterRole(TiliModel::TilaRooli);
    proxyTila->setFilterRegExp("[12]");

    ui->view->setModel(proxyTila);
    ui->view->setColumnHidden(TiliModel::NRONIMI, true);

    ui->view->setColumnHidden(TiliModel::ALV, ! kp()->asetukset()->onko("AlvVelvollinen"));

    ui->view->horizontalHeader()->setSectionResizeMode(TiliModel::NIMI, QHeaderView::Stretch);

    ui->infoMerkki->setVisible(false);
    ui->ohjeLabel->setVisible(false);

    connect( ui->suosikitNappi, &QPushButton::clicked,
             this, &TilinValintaDialogi::suodataSuosikit);
    connect( ui->kaikkiNappi, &QPushButton::clicked,
             this, &TilinValintaDialogi::naytaKaikki);
    connect( ui->suodatusEdit, SIGNAL(textChanged(QString)),
             this, SLOT(suodata(QString)));
    connect( ui->view, SIGNAL(clicked(QModelIndex)),
             this, SLOT(klikattu(QModelIndex)));
    connect( ui->view->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
             this, SLOT(valintaMuuttui(QModelIndex)));
    connect( ui->view, &QTableView::entered,
             this, &TilinValintaDialogi::naytaOhje);

    ui->view->setMouseTracking(true);

    ui->suodatusEdit->installEventFilter(this);
    restoreGeometry( kp()->settings()->value("TilinValintaDlg").toByteArray());

    kp()->tilit()->haeSaldot();
}

TilinValintaDialogi::~TilinValintaDialogi()
{
    kp()->settings()->setValue("TilinValintaDlg", saveGeometry());
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
    if( alku.toInt() > 0) {
        proxyNimi->setFilterRole(TiliModel::NroRooli);
        proxyNimi->setFilterRegExp( "^" + alku  );
    }
    else
    {
        proxyNimi->setFilterRole(TiliModel::NimiRooli);
        proxyNimi->setFilterFixedString(alku);
    }



    if( tyyppiSuodatin.isEmpty())
    {
        if( alku.isEmpty())
            proxyTyyppi->setFilterFixedString("");
        else
            proxyTyyppi->setFilterRegExp("[ABCD].*");
    }
    else
        proxyTyyppi->setFilterRegExp( tyyppiSuodatin );


    if( ui->view->selectionModel()->selectedIndexes().isEmpty())
        ui->view->setCurrentIndex( ui->view->model()->index(0,0) );
    alaValitseOtsikoita(1);
}

void TilinValintaDialogi::suodataTyyppi(const QString &regexp)
{
    alkuperainenTyyppiSuodatin = regexp;
    tyyppiSuodatin = alkuperainenTyyppiSuodatin;
    suodata(ui->suodatusEdit->text());
    alaValitseOtsikoita(1);
}

void TilinValintaDialogi::suodataSuosikit(bool suodatetaanko)
{
    if( suodatetaanko)
    {
        ui->kaikkiNappi->setChecked(false);
        proxyTila->setFilterFixedString("2");
    }
    else
        proxyTila->setFilterRegExp("[12]");

    tyyppiSuodatin = alkuperainenTyyppiSuodatin;
    suodata(ui->suodatusEdit->text());
}

void TilinValintaDialogi::naytaKaikki(bool naytetaanko)
{
    if( naytetaanko) {
        ui->suosikitNappi->setChecked(false);
        proxyTila->setFilterFixedString("");
        tyyppiSuodatin = "";
    } else {
        proxyTila->setFilterRegExp("[12]");
        tyyppiSuodatin = alkuperainenTyyppiSuodatin;
    }
    suodata(ui->suodatusEdit->text());
}

void TilinValintaDialogi::asetaModel(TiliModel *model)
{
    tiliModel = model;
    proxyNimi->setSourceModel( model );
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

