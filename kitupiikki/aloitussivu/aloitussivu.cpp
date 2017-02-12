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

#include <QFile>
#include <QStringList>
#include <QSqlQuery>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QFileDialog>

#include <QListWidget>

#include "aloitussivu.h"

#include "sisalto.h"

#include "db/kirjanpito.h"
#include "uusikp/uusikirjanpito.h"

AloitusSivu::AloitusSivu() :
    KitupiikkiSivu(0)
{
    view = new QWebEngineView();
    sisalto = new Sisalto();
    view->setPage(sisalto);


    QVBoxLayout *sivuleiska = new QVBoxLayout;
    QPushButton *uusinappi = new QPushButton(QIcon(":/pic/uusitiedosto.png"),"Uusi kirjanpito");
    QPushButton *avaanappi = new QPushButton(QIcon(":/pic/avaatiedosto.png"),"Avaa kirjanpito");
    QPushButton *tuonappi = new QPushButton(QIcon(":/pic/tuotiedosto.png"),"Tuo kirjanpito");
    QPushButton *aboutnappi = new QPushButton(QIcon(":/pic/info.png"),"Tietoja");
    QPushButton *webnappi = new QPushButton(QIcon(":/pic/kotisivu.png"),"Ohjelman kotisivu");

    viimelista = new QListWidget;


    sivuleiska->addWidget(uusinappi);
    sivuleiska->addWidget(avaanappi);
    sivuleiska->addWidget(tuonappi);

    sivuleiska->addWidget(viimelista);
    sivuleiska->addWidget(aboutnappi);
    sivuleiska->addWidget(webnappi);

    QHBoxLayout *paaleiska = new QHBoxLayout;
    paaleiska->addWidget( view, 1);
    paaleiska->addLayout(sivuleiska, 0);
    setLayout( paaleiska );


    connect(sisalto, SIGNAL(selaa(int)), this, SLOT(selaaTilia(int)));

    connect( uusinappi, SIGNAL(clicked(bool)), this, SLOT(uusiTietokanta()));
    connect( avaanappi, SIGNAL(clicked(bool)), this, SLOT(avaaTietokanta()));
    connect( viimelista, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(viimeisinTietokanta(QListWidgetItem*)));
    connect(webnappi, SIGNAL(clicked(bool)), this, SLOT(kotisivulle()));

    lisaaViimetiedostot();

}

AloitusSivu::~AloitusSivu()
{

}


void AloitusSivu::siirrySivulle()
{
    // Lataa aloitussivun
    lisaaTxt("<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/aloitus/aloitus.css\"></head><body>");

    if( Kirjanpito::db()->asetus("nimi").isEmpty())
    {

        lisaaTxt("<h1>Tervetuloa!</h1>"
                 "<p>Kitupiikki on suomalainen avoimen lähdekoodin kirjanpito-ohjelmisto. Ohjelmistoa saa käyttää, kopioida ja muokata "
                 "maksutta.</p>"
                 "<p>Tutustu lukemalla ohjeita, tai aloita heti kokeilemalla <a href=ktp:uusi>uuden kirjanpidon luomista</a>. Ohjelmisto neuvoo sekä "
                 "uuden kirjanpidon aloittamisessa että myöhemmin matkan varrella.<p>");
        sisalto->valmis("qrc:/aloitus/");

    }
    else
    {
        kpAvattu();
        sisalto->valmis( Kirjanpito::db()->hakemisto().absoluteFilePath("index"));
    }
}

void AloitusSivu::selaaTilia(int tilinumero)
{
    emit selaus(tilinumero, tilikausi);
}

void AloitusSivu::uusiTietokanta()
{
    QString uusitiedosto = UusiKirjanpito::aloitaUusiKirjanpito();
    if( !uusitiedosto.isEmpty())
        Kirjanpito::db()->avaaTietokanta(uusitiedosto + "/kitupiikki.sqlite");
}

void AloitusSivu::avaaTietokanta()
{
    QString polku = QFileDialog::getOpenFileName(this, "Avaa kirjanpito",
                                                 QDir::homePath(),"Kirjanpito (kitupiikki.sqlite)");
    if( !polku.isEmpty())
        Kirjanpito::db()->avaaTietokanta(polku);

}

void AloitusSivu::viimeisinTietokanta(QListWidgetItem *item)
{
    Kirjanpito::db()->avaaTietokanta( item->data(Qt::UserRole).toString());
}

void AloitusSivu::kotisivulle()
{
    sisalto->load(QUrl("http://kitupiikki.wordpress.com"));
}


void AloitusSivu::lisaaTxt(const QString &txt)
{
    sisalto->lisaaTxt(txt);
}


void AloitusSivu::kpAvattu()
{
    if( QFile::exists( Kirjanpito::db()->hakemisto().absoluteFilePath("logo128.png")))
    {
        lisaaTxt("<img class=kpkuvake src=logo128.png>");
    }
    lisaaTxt(tr("<h1>%1</h1>").arg( Kirjanpito::db()->asetus("nimi")));

    if( Kirjanpito::db()->asetukset()->onko("tilinavaus") )
    {
        sisalto->lisaaLaatikko("Tee tilinavaus","Syötä viimesimmältä tilinpäätökseltä tilien "
                 "avaavat saldot järjestelmään.");
    }
    saldot();

}

void AloitusSivu::saldot()
{
    // TODO: Miten tämä vaihdetaan
    tilikausi = Kirjanpito::db()->tilikausiPaivalle( Kirjanpito::db()->paivamaara());

    lisaaTxt(tr("<h2><img src=qrc:/aloitus/previous.png>Tilikausi %1 - %2<img src=qrc:/aloitus/next.png></h2>").arg(tilikausi.alkaa().toString(Qt::SystemLocaleShortDate))
             .arg(tilikausi.paattyy().toString(Qt::SystemLocaleShortDate)));

    QSqlQuery kysely;

    kysely.exec(QString("select nro, nimi, sum(debetsnt), sum(kreditsnt) from vientivw where tyyppi like \"AR%\" and pvm <= \"%1\" group by tili")
                .arg(tilikausi.paattyy().toString(Qt::ISODate)));

    lisaaTxt("<h3>Rahavarat</h3><table>");
    int saldosumma = 0;
    while( kysely.next())
    {
        int saldosnt = kysely.value(2).toInt() - kysely.value(3).toInt();
        saldosumma += saldosnt;
        lisaaTxt( tr("<tr><td><a href=\"selaa:%1\">%1 %2</a></td><td class=euro>%L3 €</td></tr>").arg(kysely.value(0).toInt())
                                                           .arg(kysely.value(1).toString())
                                                           .arg( ((double) saldosnt ) / 100,0,'f',2 ) );
    }
    lisaaTxt( tr("<tr class=summa><td>Rahavarat yhteensä</td><td class=euro>%L1 €</td></tr>").arg( ((double) saldosumma ) / 100,0,'f',2 ) );
    lisaaTxt("</table>");

    // Sitten tulot
    kysely.exec(QString("select nro, nimi, sum(debetsnt), sum(kreditsnt) from vientivw where tyyppi like \"T%\" AND pvm BETWEEN \"%1\" AND \"%2\" group by tili")
                .arg(tilikausi.alkaa().toString(Qt::ISODate)  )
                .arg(tilikausi.paattyy().toString(Qt::ISODate)));

    lisaaTxt("<h3>Tulot</h3><table>");
    int summatulot = 0;

    while( kysely.next())
    {
        int saldosnt = kysely.value(3).toInt() - kysely.value(2).toInt();
        summatulot += saldosnt;
        lisaaTxt( tr("<tr><td><a href=\"selaa:%1\">%1 %2</a></td><td class=euro>%L3 €</td></tr>").arg(kysely.value(0).toInt())
                                                           .arg(kysely.value(1).toString())
                                                           .arg( ((double) saldosnt ) / 100,0,'f',2 ) );
    }
    lisaaTxt( tr("<tr class=summa><td>Tulot yhteensä</td><td class=euro>%L1 €</td></tr>").arg( ((double) summatulot ) / 100,0,'f',2 ) );
    lisaaTxt("</table>");


    // Lopuksi menot
    kysely.exec(QString("select nro, nimi, sum(debetsnt), sum(kreditsnt) from vientivw where tyyppi like \"M%\" AND pvm BETWEEN \"%1\" AND \"%2\" group by tili")
                .arg(tilikausi.alkaa().toString(Qt::ISODate)  )
                .arg(tilikausi.paattyy().toString(Qt::ISODate)));


    lisaaTxt("<h3>Menot</h3><table>");
    int summamenot = 0;

    while( kysely.next())
    {
        int saldosnt = kysely.value(2).toInt() - kysely.value(3).toInt();
        summamenot += saldosnt;
        lisaaTxt( tr("<tr><td><a href=\"selaa:%1\">%1 %2</a></td><td class=euro>%L3 €</td></tr>").arg(kysely.value(0).toInt())
                                                           .arg(kysely.value(1).toString())
                                                           .arg( ((double) saldosnt ) / 100,0,'f',2 ) );
    }
    lisaaTxt( tr("<tr class=summa><td>Menot yhteensä</td><td class=euro>%L1 €</td></tr>").arg( ((double) summamenot ) / 100,0,'f',2 ) );
    lisaaTxt("</table>");

    lisaaTxt( tr("<p><table><tr class=kokosumma><td>Yli/alijäämä</td><td class=euro> %L1 €</td></tr></table>").arg(( ((double) (summatulot - summamenot) ) / 100), 0,'f',2 )) ;


}


void AloitusSivu::lisaaViimetiedostot()
{
    QSettings settings;
    QStringList lista = settings.value("viimeiset").toStringList();

    viimelista->clear();

    foreach (QString rivi, lista)
    {
        QStringList palat = rivi.split(';');
        QString polku = palat.at(0);
        QString nimi = palat.at(1);
        QString kuvake = QFileInfo(polku).absoluteDir().absoluteFilePath("logo64.png");

        if( polku.contains(".sqlite") && QFile::exists(polku))
        {

            QListWidgetItem *item = new QListWidgetItem;
            item->setText( nimi );
            if( QFile::exists(kuvake))
                item->setIcon( QIcon(kuvake));
            item->setData(Qt::UserRole, polku);
            viimelista->addItem(item);
        }
    }


}
