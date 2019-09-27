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

#include <QDesktopServices>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSqlQuery>

#include "tilinpaatoseditori.h"
#include "tilinpaatostulostaja.h"
#include "db/kirjanpito.h"
#include "tpaloitus.h"
#include "naytin/naytinikkuna.h"

TilinpaatosEditori::TilinpaatosEditori(const Tilikausi& tilikausi, QWidget *parent)
    : QMainWindow(parent),
      tilikausi_(tilikausi)
{
    editori_ = new MRichTextEdit;
    setCentralWidget( editori_);
    setWindowTitle( tr("Tilinpäätöksen liitetiedot %1").arg(tilikausi.kausivaliTekstina()));

    setWindowIcon( QIcon(":/pic/Possu64.png"));

    luoAktiot();
    luoPalkit();

    setAttribute(Qt::WA_DeleteOnClose);

    lataa();
}

void TilinpaatosEditori::tulosta(QPagedPaintDevice *printer) const
{
    QString teksti = raportit_ + "\n" + editori_->toHtml();

    TilinpaatosTulostaja::tulostaTilinpaatos( printer, tilikausi_, teksti);
}

QString TilinpaatosEditori::otsikko() const
{
    return tr("Tilinpäätös %1 - %2")
            .arg(tilikausi_.alkaa().toString("dd.MM.yyyy"))
            .arg(tilikausi_.paattyy().toString("dd.MM.yyyy"));

}

void TilinpaatosEditori::esikatselu()
{    
    esikatsele();
}

void TilinpaatosEditori::luoAktiot()
{
    tallennaAktio_ = new QAction( QIcon(":/pic/tiedostoon.png"), tr("Tallenna"), this);
    connect( tallennaAktio_, SIGNAL(triggered(bool)), this, SLOT(tallenna()));

    esikatseleAction_ = new QAction( QIcon(":/pic/print.png"), tr("Esikatsele"), this);
    connect( esikatseleAction_, SIGNAL(triggered(bool)), this, SLOT(esikatselu()));

    aloitaUudelleenAktio_ = new QAction( QIcon(":/pic/uusitiedosto.png"), tr("Aloita uudelleen"), this);
    connect( aloitaUudelleenAktio_, SIGNAL(triggered(bool)), this, SLOT(aloitaAlusta()));

    ohjeAktio_ = new QAction( QIcon(":/pic/ohje.png"), tr("Ohje"), this);
    connect( ohjeAktio_, SIGNAL(triggered(bool)), this, SLOT(ohje()));
}

void TilinpaatosEditori::luoPalkit()
{
    tilinpaatosTb_ = addToolBar( tr("&Tilinpäätös"));
    tilinpaatosTb_->addAction( aloitaUudelleenAktio_ );
    tilinpaatosTb_->addSeparator();
    tilinpaatosTb_->addAction( esikatseleAction_ );
    tilinpaatosTb_->addAction( tallennaAktio_ );
    tilinpaatosTb_->addSeparator();
    tilinpaatosTb_->addAction( ohjeAktio_ );
    tilinpaatosTb_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
}

void TilinpaatosEditori::uusiTp()
{
    QStringList pohja = kp()->asetukset()->lista("TilinpaatosPohja");
    QStringList valinnat = kp()->asetukset()->lista("TilinpaatosValinnat");
    QRegularExpression tunnisteRe("#(?<tunniste>-?\\w+)(?<pois>(\\s-\\w+)*).*");
    tunnisteRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpression raporttiRe("@(?<raportti>.+)[\\*!](?<otsikko>.+)@");
    
    QRegularExpression poisRe("-(?<pois>\\w+)");
    poisRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);

    QString teksti;
    raportit_.clear();

    bool tulosta = true;

    foreach (QString rivi, pohja)
    {
        if( rivi.startsWith('#') )
        {
            QRegularExpressionMatch tmats = tunnisteRe.match(rivi);
            if( tmats.hasMatch())
            {
                // Tulostetaan, jos haluttu tunniste valittu ei ei-ehdolla ei valittu ;)
                QString tunniste = tmats.captured(1);
                if( tunniste.startsWith('-'))
                    tulosta = !valinnat.contains( tunniste.mid(1));
                else
                    tulosta = valinnat.contains(tunniste);

                // Kuitenkin jos rivillä myös -eiehto, niin ei kuitenkaan tulosteta
                QString pois = tmats.captured("pois");
                if( !pois.isEmpty())
                {
                    QStringList poisTunnukset;
                    QRegularExpressionMatchIterator iter = poisRe.globalMatch(pois);
                    while( iter.hasNext())
                    {
                        QString poistunnus = iter.next().captured(1);
                        if( valinnat.contains(poistunnus))
                        {
                            tulosta = false;
                            break;
                        }
                    }
                }
                
            }
            else
                // Pelkkä # tai kelvoton ehto niin tulostetaan joka tapauksessa
                tulosta = true;
        }
        else if( rivi.startsWith('@') && tulosta)
        {
            // Näillä tulostetaan erityisiä kenttiä
            if( rivi == "@sha@")
                ; //teksti.append( tilikausi_.json()->str("ArkistoSHA"));
            else if( rivi == "@henkilosto@")
                teksti.append( henkilostotaulukko());
            else if( rivi == "@tositelajit@")
                teksti.append( tositelajitaulukko() );
            else if( rivi == "@tulos@")
                teksti.append( QString(" %L1 € ").arg(kp()->tilit()->tiliTyypilla(TiliLaji::KAUDENTULOS).saldoPaivalle( tilikausi_.paattyy() ) / 100.0 , 0, 'f', 2 ) );
            else
            {
                QRegularExpressionMatch rmats = raporttiRe.match(rivi);
                if( rmats.hasMatch())
                {
                    raportit_.append( rivi + " " );
                }
            }
        }
        else if( tulosta )
            teksti.append(rivi);
    }
    editori_->setText(teksti);
}

void TilinpaatosEditori::lataa()
{
    QString data = tilikausi_.str("TilinpaatosTeksti");
    if( data.isEmpty())
    {
        if(!aloitaAlusta())
        {
            // Ei päästy edes alkuun!
            close();
        }
    }
    else
    {
        raportit_ = data.left( data.indexOf("\n"));
        editori_->setText( data.mid(data.indexOf("\n")+1));
    }
}

void TilinpaatosEditori::closeEvent(QCloseEvent *event)
{
    QString teksti = raportit_ + "\n" + editori_->toHtml();
    if( teksti != tilikausi_.str("TilinpaatosTeksti"))
    {
        QMessageBox::StandardButton vastaus =
                QMessageBox::question(this, tr("Tilinpäätöstä muokattu"),
                                      tr("Tallennetaanko muokattu tilinpäätös?"),
                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                      QMessageBox::Cancel);
        if( vastaus == QMessageBox::Yes)
        {
            tallenna();
            event->accept();
        }
        else if( vastaus == QMessageBox::Cancel)
            event->ignore();
        else    // Jos halutaan kuitenkin sulkea...
            event->accept();
    }
    else
        event->accept();

}

QString TilinpaatosEditori::henkilostotaulukko()
{
    Tilikausi verrokki;
    if( kp()->tilikaudet()->indeksiPaivalle( tilikausi_.paattyy()))
        verrokki = kp()->tilikaudet()->tilikausiIndeksilla(  kp()->tilikaudet()->indeksiPaivalle(tilikausi_.paattyy()) - 1 );

    QString txt = tr("<table width=100%><tr><td></td><td align=center>%1</td>").arg(tilikausi_.kausivaliTekstina());
    if( verrokki.alkaa().isValid() )
        txt.append( QString("<td align=center>%1</td>").arg(verrokki.kausivaliTekstina()) );

    txt.append(tr("</tr><tr><td>Henkilöstöä keskimäärin</td><td align=center>%1</td>").arg( kp()->tilikaudet()->tilikausiPaivalle( tilikausi_.paattyy() ).henkilosto()));
    if( verrokki.alkaa().isValid())
        txt.append( QString("<td align=center>%1</td>").arg(verrokki.henkilosto()));
    txt.append("</tr></table>");
    return txt;
}

QString TilinpaatosEditori::tositelajitaulukko()
{
    QString kysymys = QString("select tositelaji.nimi, count(tosite.id)  from tosite, tositelaji where tosite.pvm between '%1' and '%21' and  tosite.laji=tositelaji.id group by tositelaji.id order by tositelaji.nimi")
            .arg( tilikausi_.alkaa().toString(Qt::ISODate) )
            .arg( tilikausi_.paattyy().toString(Qt::ISODate));


    QString txt = tr("<table>");
    QSqlQuery kysely(kysymys);
    while(kysely.next())
    {
        txt.append( QString("<tr><td>%1 &nbsp;</td><td align=right>%2 kpl</td></tr>")
                    .arg(kysely.value(0).toString())
                    .arg(kysely.value(1).toInt()));
    }
    txt.append("</table>");
    return txt;
}

bool TilinpaatosEditori::aloitaAlusta()
{
    TpAloitus tpaloitus(tilikausi_);
    if( tpaloitus.exec() == QDialog::Accepted)
    {
        uusiTp();   // Aloitetaan alusta
    }
    else
    {
        return false;
    }
    return true;
}

void TilinpaatosEditori::tallenna()
{
    QString teksti = raportit_ + "\n" + editori_->toHtml();
    tilikausi_.set("TilinpaatosTeksti", teksti);

    QByteArray pdfa = pdf();

//    kp()->liitteet()->asetaLiite( pdfa, tilikausi_.alkaa().toString(Qt::ISODate) );
//    kp()->liitteet()->tallenna();

    // Tallennetaan myös Arkistoon
    QFile out(kp()->arkistopolku()  + "/" + tilikausi_.arkistoHakemistoNimi() + "/tilinpaatos.pdf");
    out.open(QIODevice::WriteOnly);
    out.write( pdfa );
    out.close();

    emit tallennettu();
}


void TilinpaatosEditori::ohje()
{
    kp()->ohje("tilikaudet/tilinpaatos");
}
