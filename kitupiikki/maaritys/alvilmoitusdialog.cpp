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
#include <QDebug>

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
    int bruttoveroayhtSnt = 0;
    int bruttovahennettavaaSnt = 0;

    EhdotusModel ehdotus;
    QSqlQuery query( *kp()->tietokanta() );
    QString txt = tr("Verokausi %1 - %2 \n").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate));

    // 1) Bruttojen oikaisut

    query.exec(  QString("select alvkoodi,alvprosentti,sum(debetsnt) as debetit, sum(kreditsnt) as kreditit, tili from vienti where pvm between \"%3\" and \"%4\" and (alvkoodi=%1 or alvkoodi=%2) group by alvkoodi,tili,alvprosentti")
                 .arg(AlvKoodi::MYYNNIT_BRUTTO).arg(AlvKoodi::OSTOT_BRUTTO)
                 .arg(alkupvm.toString(Qt::ISODate)).arg(loppupvm.toString(Qt::ISODate)));

    qDebug() << query.lastQuery();

    while( query.next() && query.value("alvprosentti").toInt())
    {

        Tili tili = kp()->tilit()->tiliIndeksilla( query.value("tili").toInt() );
        int alvprosentti = query.value("alvprosentti").toInt();
        int saldoSnt = query.value("kreditit").toInt() - query.value("debetit").toInt();

        VientiRivi rivi;
        rivi.pvm = loppupvm;
        rivi.tili = tili;
        // Brutosta erotetaan verot
        int veroSnt = ( alvprosentti * saldoSnt ) / ( 100 + alvprosentti) ;
        int nettoSnt = saldoSnt - veroSnt;


        if( query.value("alvkoodi").toInt() == AlvKoodi::MYYNNIT_BRUTTO )
        {
            verotKannoittainSnt[ alvprosentti ] = verotKannoittainSnt.value(alvprosentti, 0) + veroSnt;
            bruttoveroayhtSnt += veroSnt;

            rivi.selite = tr("Alv-kirjaus %1 - %2 %3 % vero (NETTO %L4 €, BRUTTO %L5€) ").arg(alkupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(loppupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(alvprosentti)
                    .arg(nettoSnt / 100.0,0, 'f',2)
                    .arg(saldoSnt / 100.0,0, 'f', 2);
            rivi.debetSnt = veroSnt;

        }
        else
        {
            bruttovahennettavaaSnt += qAbs(veroSnt);

            rivi.selite = tr("Alv-kirjaus %1 - %2 %3 % vähennys (NETTO %L4 €, BRUTTO %L5€) ").arg(alkupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(loppupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(alvprosentti)
                    .arg(qAbs(nettoSnt) / 100.0,0, 'f',2)
                    .arg(qAbs(saldoSnt) / 100.0,0, 'f', 2);
            rivi.kreditSnt = veroSnt;
        }
    qDebug() << rivi.selite;
        ehdotus.lisaaVienti(rivi);
    }

    // 2) Nettokirjausten koonti
    query.exec( QString("select alvprosentti, sum(debetsnt) as debetit, sum(kreditsnt) as kreditit from vienti where pvm between \"%1\" and \"%2\" and alvkoodi=%3 group by alvprosentti")
                .arg(alkupvm.toString(Qt::ISODate)).arg(loppupvm.toString(Qt::ISODate)).arg(AlvKoodi::ALVKIRJAUS + AlvKoodi::MYYNNIT_NETTO) );

    qDebug() << query.lastQuery();
    while( query.next())
    {
        int alvprosentti = query.value("alvprosentti").toInt();
        int saldo = query.value("kreditit").toInt() - query.value("debetit").toInt();
        verotKannoittainSnt[ alvprosentti ] = verotKannoittainSnt.value(alvprosentti) + saldo;
    }


    // Muut kirjaukset tauluihin
    query.exec( QString("select alvkoodi, sum(debetsnt) as debetit, sum(kreditsnt) as kreditit from vienti where pvm between \"%1\" and \"%2\" group by alvkoodi")
                .arg(alkupvm.toString(Qt::ISODate)).arg(loppupvm.toString(Qt::ISODate)) );

    qDebug() << query.lastQuery();
    QMap<int,int> kooditaulu;

    int nettoverosnt = 0;
    int nettovahennyssnt = 0;

    while( query.next())
    {
        int saldo = query.value("kreditit").toInt() - query.value("debetit").toInt();
        int koodi = query.value("alvkoodi").toInt();

        if( koodi  > AlvKoodi::ALVVAHENNYS)
        {
            nettovahennyssnt += 0 - saldo;
            kooditaulu.insert(koodi, 0-saldo);
        }
        else if( koodi > AlvKoodi::ALVKIRJAUS)
        {
            nettoverosnt += saldo;
            kooditaulu.insert(koodi, saldo);
        }
        else
        {
            if( koodi > 20)
                kooditaulu.insert(koodi, 0-saldo);
            else
                kooditaulu.insert(koodi, saldo);
        }
    }

    QMapIterator<int,int> iter(verotKannoittainSnt);
    while( iter.hasNext())
    {
        iter.next();
        txt.append( tr(" %1 %  -  %L2 € \n").arg(iter.key()).arg(iter.value() / 100.0 ,0,'f',2));
    }

    txt.append("\n\n");
    QMapIterator<int,int> kIter(kooditaulu);
    while( kIter.hasNext() )
    {
        kIter.next();
        txt.append( tr(" %1  -  %L2 € \n").arg(kIter.key()).arg(kIter.value() / 100.0 ,0,'f',2));
    }

    txt.append( tr("Veroa %1 \n").arg( ( bruttoveroayhtSnt + nettoverosnt ) / 100.0, 0, 'f', 2 ));
    txt.append( tr("Vähennettävää %1 \n").arg( (bruttovahennettavaaSnt + nettovahennyssnt) / 100.0, 0, 'f', 2 ));


    AlvIlmoitusDialog dlg;
    dlg.ui->ilmoitusBrowser->setPlainText(txt);
    dlg.exec();

    return QDate();
}
