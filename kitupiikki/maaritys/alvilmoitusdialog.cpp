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

#include <QSqlQuery>

#include "alvilmoitusdialog.h"
#include "ui_alvilmoitusdialog.h"

#include "kirjaus/ehdotusmodel.h"
#include "db/kirjanpito.h"


AlvIlmoitusDialog::AlvIlmoitusDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AlvIlmoitusDialog)
{
    ui->setupUi(this);
}

AlvIlmoitusDialog::~AlvIlmoitusDialog()
{
    delete ui;
}

QDate AlvIlmoitusDialog::teeAlvIlmoitus(QDate alkupvm, QDate loppupvm)
{
    QMap<int,int> verotKannoittainSnt;  // verokanta - maksettava vero
    int brutostaveroayhtSnt = 0;
    int vahennettavaaSnt = 0;

    EhdotusModel ehdotus;
    QSqlQuery query( *kp()->tietokanta() );
    QString txt = tr("Verokausi %1 - %2 \n").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate));

    // 1) Bruttojen oikaisut

    query.exec(  QString("select alvkoodi,alvprosentti,sum(debetsnt) as debetit, sum(kreditsnt) as kreditit, tili from vienti where pvm between \"%3\" and \"%4\" and (alvkoodi=%1 or alvkoodi=%2) group by verokoodi,tili,alvprosentti").arg(AlvKoodi::MYYNNIT_BRUTTO).arg(AlvKoodi::OSTOT_BRUTTO)
                 .arg(alkupvm.toString(Qt::ISODate)).arg(loppupvm.toString(Qt::ISODate)));
    while( query.next() && query.value("alvprosentti").toInt())
    {

        Tili tili = kp()->tilit()->tiliIndeksilla( query.value("tili").toInt() );
        int alvprosentti = query.value("alvprosentti").toInt();
        int saldoSnt = query.value("kreditit").toInt() - query.value("debitit").toInt();

        VientiRivi rivi;
        rivi.pvm = loppupvm;
        rivi.tili = tili;
        // Brutosta erotetaan verot
        int veroSnt = ( alvprosentti / ( 100 + alvprosentti )) * saldoSnt;
        int nettoSnt = saldoSnt - veroSnt;


        if( query.value("alvkoodi").toInt() == AlvKoodi::MYYNNIT_BRUTTO )
        {
            verotKannoittainSnt[ alvprosentti ] = verotKannoittainSnt.value(alvprosentti, 0) + veroSnt;
            brutostaveroayhtSnt += veroSnt;

            rivi.selite = tr("Alv-kirjaus %1 - %2 %3 % vero (NETTO %L4 €, BRUTTO %L5€) ").arg(alkupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(loppupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(alvprosentti)
                    .arg(nettoSnt / 100.0,0, 'f',2)
                    .arg(saldoSnt / 100.0,0, 'f', 2);
            rivi.debetSnt = veroSnt;

        }
        else
        {
            vahennettavaaSnt += qAbs(veroSnt);

            rivi.selite = tr("Alv-kirjaus %1 - %2 %3 % vähennys (NETTO %L4 €, BRUTTO %L5€) ").arg(alkupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(loppupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(alvprosentti)
                    .arg(qAbs(nettoSnt) / 100.0,0, 'f',2)
                    .arg(qAbs(saldoSnt) / 100.0,0, 'f', 2);
            rivi.kreditSnt = veroSnt;
        }
        ehdotus.lisaaVienti(rivi);
    }

    // 2) Nettokirjausten koonti
    query.exec( QString("select alvprosentti, sum(debet) as debetit, sum(kredit) as kreditit from vienti where pvm between \"%1\" and \"%2\" and alvkoodi=%3")
                .arg(alkupvm.toString(Qt::ISODate)).arg(loppupvm.toString(Qt::ISODate)).arg(AlvKoodi::ALVKIRJAUS + AlvKoodi::MYYNNIT_NETTO) );
    while( query.next())
    {
        int alvprosentti = query.value("alvprosentti").toInt();
        int saldo = query.value("kreditit").toInt() - query.value("debetit").toInt();
        verotKannoittainSnt[ alvprosentti ] = verotKannoittainSnt.value(alvprosentti) + saldo;
    }
    query.exec( QString("select sum(debet) as debetit, sum(kredit) as kreditit from vienti where pvm between \"%1\" and \"%2\" and alvkoodi=%3")
                .arg(alkupvm.toString(Qt::ISODate)).arg(loppupvm.toString(Qt::ISODate)).arg(AlvKoodi::ALVVAHENNYS + AlvKoodi::OSTOT_NETTO) );
    while( query.next())
    {
        int saldo = query.value("debetit").toInt() - query.value("kreditit").toInt();
        vahennettavaaSnt -= saldo;
    }

    QMapIterator<int,int> iter(verotKannoittainSnt);
    while( iter.hasNext())
    {
        txt.append( tr(" %1 %  -  %L1 € \n").arg(iter.key()).arg(iter.value() / 100.0 ,0,'f',2));
    }
    txt.append( tr("Vähennettävää %1").arg(vahennettavaaSnt / 100.0, 0, 'f', 2 ));


    AlvIlmoitusDialog dlg;
    dlg.ui->ilmoitusBrowser->setPlainText(txt);
    dlg.exec();

    return QDate();
}
