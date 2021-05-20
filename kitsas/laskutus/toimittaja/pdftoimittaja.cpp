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
#include "pdftoimittaja.h"
#include <QFileDialog>

#include "laskutus/tulostus/laskuntulostaja.h"
#include "db/kirjanpito.h"
#include <QSettings>

PdfToimittaja::PdfToimittaja(QObject *parent)
    : AbstraktiToimittaja(parent)
{

}

void PdfToimittaja::toimita()
{
    if( hakemisto_.isEmpty()) {
        QWidget *grandParent = qobject_cast<QWidget*>( parent()->parent() );
        hakemisto_ = QFileDialog::getExistingDirectory(
                    grandParent,
                    tr("Valitse laskujen tallennushakemisto"),
                    kp()->settings()->value("PdfLaskuTulostusHakemisto").toString());
    }

    if( hakemisto_.isEmpty()) {
        virhe( tr("Laskun tallentaminen peruttiin"));
        return;
    }
    kp()->settings()->setValue("PdfLaskuTulostusHakemisto", hakemisto_);

    QDir hakemisto(hakemisto_);

    Tosite tosite;
    tosite.lataa(tositeMap());
    LaskunTulostaja tulostaja(kp());

    QString tnimi = tulkkaa("laskuotsikko", tosite.constLasku().kieli().toLower()) +
            tosite.constLasku().numero() +
            ".pdf";

    QFile tiedosto ( hakemisto.absoluteFilePath(tnimi));
    if( tiedosto.open( QFile::WriteOnly | QFile::Truncate)) {
        tiedosto.write( tulostaja.pdf(tosite) );
        tiedosto.close();
        merkkaaToimitetuksi();
    } else {
        virhe( tr("Laskutiedoston kirjoittaminen epäonnistui"));
    }
}
