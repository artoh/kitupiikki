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
#include <QPrinter>

#include <QTemporaryFile>
#include <QTextDocument>

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
            if( koodi / 10 % 2 )  // 10 myynti, 20 osto jne.
                kooditaulu.insert(koodi, saldo);
            else
                kooditaulu.insert(koodi, 0 - saldo);
        }
    }
    // Kirjaus alv-saamistililtä ja alv-velkatililtä verovelkatilille
    if( nettoverosnt )
    {
        VientiRivi rivi;
        rivi.pvm = loppupvm;
        rivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA);
        rivi.selite = tr("Alv-kirjaus %1 - %2 ").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate));
        rivi.debetSnt = nettoverosnt;
        ehdotus.lisaaVienti(rivi);
    }
    if( nettovahennyssnt )
    {
        VientiRivi rivi;
        rivi.pvm = loppupvm;
        rivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA);
        rivi.selite = tr("Alv-kirjaus %1 - %2 ").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate));
        rivi.kreditSnt = nettovahennyssnt;
        ehdotus.lisaaVienti(rivi);
    }
    // Ja lopuksi kirjataan verot verotilille
    int maksettavavero = bruttoveroayhtSnt + nettoverosnt - bruttovahennettavaaSnt - nettovahennyssnt;
    if( maksettavavero )
    {
        VientiRivi rivi;
        rivi.pvm = loppupvm;
        rivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::VEROVELKA);
        rivi.selite = tr("Alv-ilmoituksella tilitettävä vero kaudelta %1 - %2").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate));
        if( maksettavavero > 0 )
            rivi.kreditSnt = maksettavavero;
        else
            rivi.debetSnt = 0 - maksettavavero;
        ehdotus.lisaaVienti(rivi);
    }


    // Laskelman tulostus
    QMapIterator<int,int> iter(verotKannoittainSnt);
    iter.toBack();

    QString txt = tr("<h1>Arvonlisäverolaskelma %1 - %2 </h1>").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate));
    txt.append(tr("<h2>Vero kotimaan myynnistä verokannoittain</h2><table>"));
    while( iter.hasPrevious())
    {
        iter.previous();
        txt.append( tr("<tr><td>%1 %:n vero </td><td>%L2 € </td></tr>").arg(iter.key()).arg(iter.value() / 100.0 ,0,'f',2));
    }


    txt.append("</table><h2>Vero ostoista ja maahantuonneista</h2><table>");
    txt.append(tr("<tr><td>Vero tavaraostoista muista EU-maista</td><td>%L2 €</td></tr>").arg( kooditaulu.value(AlvKoodi::ALVKIRJAUS + AlvKoodi::YHTEISOHANKINNAT_TAVARAT) / 100.0, 0,'f',2 ) );
    txt.append(tr("<tr><td>Vero palveluostoista muista EU-maista</td><td>%L2 €</td></tr>").arg( kooditaulu.value(AlvKoodi::ALVKIRJAUS + AlvKoodi::YHTEISOHANKINNAT_PALVELUT) / 100.0, 0,'f',2 ) );
    txt.append(tr("<tr><td>Vero tavaroiden maahantuonnista EU:n ulkopuolelta</td><td>%L2 €</td></tr>").arg( kooditaulu.value(AlvKoodi::ALVKIRJAUS + AlvKoodi::RAKENNUSPALVELU_OSTO) / 100.0, 0,'f',2 ) );
    txt.append(tr("<tr><td>Vero rakentamispalvelun ja metalliromun ostoista</td><td>%L2 €</td></tr>").arg( kooditaulu.value(AlvKoodi::ALVKIRJAUS + AlvKoodi::RAKENNUSPALVELU_OSTO) / 100.0, 0,'f',2 ) );
    txt.append("</table>");

    txt.append("<h2>Vähennettävä vero</h2><table>");
    txt.append(tr("<tr><td>Verokauden vähennettävä vero</td><td>%L2 €</td></tr>").arg( (nettovahennyssnt + bruttovahennettavaaSnt) / 100.0, 0,'f',2  ) );
    txt.append("</table>");

    txt.append("<h2>Myynnit ja ostot</h2><table>");
    txt.append(tr("<tr><td>0-verokannan alainen liikevaihto</td><td>%L2 €</td></tr>").arg( kooditaulu.value(AlvKoodi::ALV0) / 100.0, 0,'f',2 ) );
    txt.append(tr("<tr><td>Tavaroiden myynti muihin EU-maihin</td><td>%L2 €</td></tr>").arg( kooditaulu.value(AlvKoodi::YHTEISOMYYNTI_TAVARAT) / 100.0, 0,'f',2 ) );
    txt.append(tr("<tr><td>Palveluiden myynti muihin EU-maihin</td><td>%L2 €</td></tr>").arg( kooditaulu.value(AlvKoodi::YHTEISOMYYNTI_PALVELUT) / 100.0, 0,'f',2 ) );
    txt.append(tr("<tr><td>Tavaraostot muista EU-maista</td><td>%L2 €</td></tr>").arg( kooditaulu.value(AlvKoodi::YHTEISOHANKINNAT_TAVARAT) / 100.0, 0,'f',2 ) );
    txt.append(tr("<tr><td>Palveluostot muista EU-maista</td><td>%L2 €</td></tr>").arg( kooditaulu.value(AlvKoodi::YHTEISOHANKINNAT_PALVELUT) / 100.0, 0,'f',2 ) );
    txt.append(tr("<tr><td>Tavaroiden maahantuonnit EU:n ulkopuolelta</td><td>%L2 €</td></tr>").arg( kooditaulu.value(AlvKoodi::MAAHANTUONTI) / 100.0, 0,'f',2 ) );
    txt.append(tr("<tr><td>Rakentamispalveluiden ja metalliromun myynnit</td><td>%L2 €</td></tr>").arg( kooditaulu.value(AlvKoodi::RAKENNUSPALVELU_OSTO) / 100.0, 0,'f',2 ) );
    txt.append(tr("<tr><td>Rakentamispalveluiden ja metalliromun ostot</td><td>%L2 €</td></tr>").arg( kooditaulu.value(AlvKoodi::RAKENNUSPALVELU_OSTO) / 100.0, 0,'f',2 ) );


    txt.append(tr("</table><h2>Maksettava vero</h2><table>"));
    txt.append(tr("<tr><td>Vero yhteensä</td><td>%L2 €</td></tr>").arg( (bruttoveroayhtSnt + nettoverosnt) / 100.0, 0,'f',2 ) );
    txt.append(tr("<tr><td>Vähenettävä vero yhteensä</td><td>%L2 €</td></tr>").arg( (bruttovahennettavaaSnt + nettovahennyssnt ) / 100.0, 0,'f',2 ) );
    txt.append(tr("<tr><td>Maksettava vero</td><td>%L2 €</td></tr>").arg( maksettavavero / 100.0, 0,'f',2 ) );
    txt.append("</table>");


    AlvIlmoitusDialog dlg;
    dlg.ui->ilmoitusBrowser->setHtml(txt);
    if( dlg.exec() )
    {
        // Laskelma vahvistettu, tallennetaan tositteeksi
        TositeModel model( kp()->tietokanta());
        model.asetaPvm( loppupvm );
        model.asetaTositelaji(0);
        model.asetaOtsikko( tr("Arvonlisävero %1 - %2").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate)) );
        ehdotus.tallenna( model.vientiModel() );

        // Liitetään laskelma
        QTemporaryFile file( QDir::tempPath() + "/alv-XXXXXX.pdf");
        file.open();
        file.close();
        QTextDocument dokumentti;
        dokumentti.setHtml(txt);
        QPrinter printer;
        printer.setPageSize(QPrinter::A4);
        printer.setOutputFileName( file.fileName() );
        dokumentti.print( &printer );
        qDebug() << file.fileName();
        model.liiteModel()->lisaaTiedosto( file.fileName(), tr("Alv-laskelma"));

        model.tallenna();
        return loppupvm;
    }

    return QDate();
}
