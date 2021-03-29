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
#include "tilikarttapaivitys.h"

#include "uusikirjanpito/uusivelho.h"

#include "db/kirjanpito.h"
#include "db/yhteysmodel.h"
#include "pilvi/pilvimodel.h"
#include <QDate>

#include <QJsonDocument>
#include <QMap>
#include <QMessageBox>
#include <QDebug>
#include <QDialog>
#include "ui_paivitetaandlg.h"

TilikarttaPaivitys::TilikarttaPaivitys(QWidget *parent)
    : MaaritysWidget(parent),
      ui( new Ui::TilikarttaPaivitys)
{
    ui->setupUi(this);

    connect( ui->paivitaNappi, &QPushButton::clicked, this, &TilikarttaPaivitys::paivita);
}

bool TilikarttaPaivitys::nollaa()
{
    ui->karttaNimi->setText( kp()->asetus("Tilikartta") );

    QDate nykypvm = kp()->asetukset()->pvm("TilikarttaPvm");
    QDate uusipvm = paivitysPvm();

    ui->nykyPvm->setText( nykypvm.toString("dd.MM.yyyy"));
    ui->uusiPvm->setText( uusipvm.toString("dd.MM.yyyy"));

    ui->varmuusLabel->setVisible( !qobject_cast<PilviModel*>( kp()->yhteysModel() ) );

    ui->paivitaNappi->setEnabled( uusipvm > nykypvm );
    return true;
}

QDate TilikarttaPaivitys::paivitysPvm()
{
    QString polku = ":/tilikartat/" + kp()->asetus("VakioTilikartta") + ".kitsaskartta";
    QFile kartta(polku);
    if( kartta.open(QIODevice::ReadOnly))
        return  QJsonDocument::fromJson( kartta.readAll() ).toVariant().toMap().value("asetukset").toMap().value("TilikarttaPvm").toDate();
    return QDate();
}

bool TilikarttaPaivitys::onkoPaivitettavaa()
{
    QDate nykypvm = kp()->asetukset()->pvm("TilikarttaPvm");
    QDate uusipvm = paivitysPvm();
    return uusipvm > nykypvm;
}

void TilikarttaPaivitys::paivita()
{
    ui->paivitaNappi->setEnabled(false);
    odotaDlg = new QDialog(this);
    odotaUi = new Ui::PaivitetaanDlg();
    odotaUi->setupUi(odotaDlg);
    odotaDlg->show();
    qApp->processEvents();


    KpKysely *kysely = kpk("/init", KpKysely::PATCH);

    connect( kysely, &KpKysely::vastaus, this, &TilikarttaPaivitys::paivitetty);
    QString polku = ":/tilikartat/" + kp()->asetus("VakioTilikartta") + ".kitsaskartta";
    kysely->kysy( UusiVelho::kartta(polku) );
}

void TilikarttaPaivitys::paivitetty()
{
    if( odotaDlg ) {
        odotaDlg->close();
        delete odotaUi;
        delete odotaDlg;
        odotaDlg = nullptr;
    }
    QMessageBox::information(this, tr("Tilikartta päivitetty"), tr("Tilikartta päivitetty uuteen versioon"));

    kp()->yhteysModel()->alusta();
    emit kp()->perusAsetusMuuttui();
}

QVariantMap TilikarttaPaivitys::lataaPaivitys(const QString &polku)
{
    QVariantMap map;

    map.insert("asetukset", UusiVelho::asetukset(polku) );
    {
        QFile tilit(polku + "/tilit.json");
        if( tilit.open(QIODevice::ReadOnly) )
            map.insert("tilit", QJsonDocument::fromJson( tilit.readAll() ).toVariant() );
    }

    return map;
}

