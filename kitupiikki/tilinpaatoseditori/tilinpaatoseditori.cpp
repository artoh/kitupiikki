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

#include "tilinpaatoseditori.h"
#include "tilinpaatostulostaja.h"
#include "db/kirjanpito.h"
#include "tpaloitus.h"

TilinpaatosEditori::TilinpaatosEditori(Tilikausi tilikausi, QWidget *parent)
    : QMainWindow(parent),
      tilikausi_(tilikausi),
      printer_( QPrinter::HighResolution)
{
    editori_ = new MRichTextEdit;
    setCentralWidget( editori_);
    setWindowTitle( tr("Tilinpäätöksen liitetiedot %1").arg(tilikausi.kausivaliTekstina()));

    setWindowIcon( QIcon(":/pic/Possu64.png"));

    printer_.setPageMargins( 25,10,10,10, QPrinter::Millimeter);

    luoAktiot();
    luoPalkit();

    setAttribute(Qt::WA_DeleteOnClose);

    lataa();
}

void TilinpaatosEditori::esikatsele()
{
    tallenna();
    QDesktopServices::openUrl( QUrl::fromLocalFile( kp()->hakemisto().absoluteFilePath("arkisto/" + tilikausi_.arkistoHakemistoNimi() + "/tilinpaatos.pdf") ));
}

void TilinpaatosEditori::luoAktiot()
{
    tallennaAktio_ = new QAction( QIcon(":/pic/tiedostoon.png"), tr("Tallenna"), this);
    connect( tallennaAktio_, SIGNAL(triggered(bool)), this, SLOT(tallenna()));

    esikatseleAction_ = new QAction( QIcon(":/pic/print.png"), tr("Esikatsele"), this);
    connect( esikatseleAction_, SIGNAL(triggered(bool)), this, SLOT(esikatsele()));

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
    QRegularExpression tunnisteRe("#(?<tunniste>-?\\w+).*");
    tunnisteRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpression raporttiRe("@(?<raportti>.+)[\\*!](?<otsikko>.+)@");

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
                QString tunniste = tmats.captured(1);
                if( tunniste.startsWith('-'))
                    tulosta = !valinnat.contains( tunniste.mid(1));
                else
                    tulosta = valinnat.contains(tunniste);
            }
            else
                tulosta = true;
        }
        else if( rivi.startsWith('@') && tulosta)
        {
            // Näillä tulostetaan erityisiä kenttiä
            if( rivi == "@sha@")
                teksti.append( tilikausi_.json()->str("ArkistoSHA"));
            else if( rivi == "@henkilosto@")
            {
                teksti.append( henkilostotaulukko());
            }
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
    QString data = tilikausi_.json()->str("TilinpaatosTeksti");
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
    if( teksti != kp()->tilikaudet()->json(tilikausi_)->str("TilinpaatosTeksti"))
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

    QString txt = tr("<table><tr><th></th><th>%1</th>").arg(tilikausi_.kausivaliTekstina());
    if( verrokki.alkaa().isValid() )
        txt.append( QString("<th>%1</th>").arg(verrokki.kausivaliTekstina()) );

    txt.append(tr("</tr><tr><td>Henkilöstöä keskimäärin</td><td>%1</td>").arg(tilikausi_.henkilosto()));
    if( verrokki.alkaa().isValid())
        txt.append( QString("<td>%1</td>").arg(verrokki.henkilosto()));
    txt.append("</tr></table>");
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
    kp()->tilikaudet()->json(tilikausi_)->set("TilinpaatosTeksti", teksti);
    kp()->tilikaudet()->tallennaJSON();

    TilinpaatosTulostaja::tulostaTilinpaatos( tilikausi_, teksti , &printer_);

    emit tallennettu();
}


void TilinpaatosEditori::ohje()
{
    QDesktopServices::openUrl(QUrl("https://artoh.github.io/kitupiikki/tilinpaatos/"));
}
