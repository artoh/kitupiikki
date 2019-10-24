/*
   Copyright (C) 2018 Arto Hyvättinen

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

#include <QSqlQuery>

#include "siirrydlg.h"
#include "db/kirjanpito.h"

SiirryDlg::SiirryDlg() :
    ui(new Ui::SiirryDlg())
{
    ui->setupUi(this);

    if(kp()->asetukset()->onko("Samaansarjaan"))
    {
        ui->tyyppiLabel->hide();
        ui->tyyppiCombo->hide();
    } else {
        // ui->tyyppiCombo->setModel( kp()->tositelajit() );
        // ui->tyyppiCombo->setModelColumn( TositelajiModel::TUNNUS);
    }

    ui->nroEdit->setValidator( new QRegularExpressionValidator( QRegularExpression("\\d+")) );

    ui->kausiCombo->setModel( kp()->tilikaudet());
    ui->kausiCombo->setModelColumn( TilikausiModel::LYHENNE);

    connect( ui->tyyppiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(tarkista()));
    connect( ui->nroEdit, SIGNAL(textChanged(QString)), this, SLOT(tarkista()));
    connect( ui->kausiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(tarkista()));
}

void SiirryDlg::tarkista()
{
    QString kysymys;

    if(kp()->asetukset()->onko("Samaansarjaan"))
    {
        kysymys = QString("SELECT id FROM tosite WHERE tunniste=%1 AND pvm BETWEEN '%2' AND '%3'")
                .arg( ui->nroEdit->text().toInt() )
                .arg( ui->kausiCombo->currentData(TilikausiModel::AlkaaRooli).toDate().toString(Qt::ISODate))
                .arg( ui->kausiCombo->currentData(TilikausiModel::PaattyyRooli).toDate().toString(Qt::ISODate));
    } else {
    }

    QSqlQuery kysely( kysymys );
    tosite = 0;

    if( kysely.next())
        tosite = kysely.value(0).toInt();

    ui->siirryNappi->setEnabled( tosite );
}

int SiirryDlg::tositeId(QDate pvm, const QString& tositelaji)
{
    SiirryDlg dlg;

    dlg.ui->tyyppiCombo->setCurrentText(tositelaji);
    dlg.ui->kausiCombo->setCurrentText( kp()->tilikausiPaivalle(pvm).kausitunnus() );

    if( dlg.exec() == Accepted )
        return dlg.tosite;

    return 0;

}
