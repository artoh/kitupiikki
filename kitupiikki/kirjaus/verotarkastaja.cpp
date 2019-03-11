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
#include "verotarkastaja.h"

#include "db/vientimodel.h"
#include "db/kirjanpito.h"
#include <QMap>
#include <QDebug>
#include <QMessageBox>

bool Verotarkastaja::tarkasta(VientiModel *model)
{

    qlonglong laskennallinenvero = 0l;
    qlonglong vero = 0l;

    qlonglong laskennallinenpalautus = 0l;
    qlonglong mahdollinenpalautus = 0l;
    qlonglong palautus = 0;

    int riveja = model->rowCount(QModelIndex());

    bool alvvaro = false;
    QDate alvIlmoitettu = kp()->asetukset()->pvm("AlvIlmoitus");

    for(int i=0; i < riveja; i++)
    {
        QModelIndex indeksi = model->index(i,0);

        int alvkoodi = indeksi.data(VientiModel::AlvKoodiRooli).toInt();
        int alvprosentti = indeksi.data(VientiModel::AlvProsenttiRooli).toInt();

        qlonglong debet = indeksi.data(VientiModel::DebetRooli).toLongLong();
        qlonglong kredit = indeksi.data(VientiModel::KreditRooli).toLongLong();

        if(  indeksi.data(VientiModel::AlvKoodiRooli).toInt() > 0 &&
             indeksi.data(VientiModel::PvmRooli).toDate().daysTo( alvIlmoitettu ) >= 0  )
            alvvaro = true;


        if( alvkoodi == AlvKoodi::MYYNNIT_NETTO)
        {
            laskennallinenvero += (kredit - debet) * alvprosentti / 100;
        }
        else if( alvkoodi == AlvKoodi::RAKENNUSPALVELU_OSTO ||
            alvkoodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
            alvkoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT ||
            alvkoodi == AlvKoodi::MAAHANTUONTI ||
            alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI)
        {
            laskennallinenvero += (debet - kredit) * alvprosentti / 100;
        }
        else if( alvkoodi == AlvKoodi::OSTOT_NETTO ||
                 alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO)
        {
            laskennallinenpalautus += (debet - kredit) * alvprosentti / 100;
        }
        else if( alvkoodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
                 alvkoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT ||
                 alvkoodi == AlvKoodi::RAKENNUSPALVELU_OSTO ||
                 alvkoodi == AlvKoodi::MAAHANTUONTI)
        {
            mahdollinenpalautus += (debet - kredit) * alvprosentti / 100;
        }

        else if( alvkoodi == AlvKoodi::ALVKIRJAUS + AlvKoodi::MYYNNIT_NETTO ||
            alvkoodi == AlvKoodi::ALVKIRJAUS + AlvKoodi::RAKENNUSPALVELU_OSTO ||
            alvkoodi == AlvKoodi::ALVKIRJAUS + AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
            alvkoodi == AlvKoodi::ALVKIRJAUS + AlvKoodi::YHTEISOHANKINNAT_PALVELUT ||
            alvkoodi == AlvKoodi::ALVKIRJAUS + AlvKoodi::MAAHANTUONTI ||
            alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_MYYNTI)
        {
            vero += (kredit - debet);
        }

        else if( alvkoodi == AlvKoodi::ALVVAHENNYS + AlvKoodi::OSTOT_NETTO ||
                 alvkoodi == AlvKoodi::ALVVAHENNYS + AlvKoodi::RAKENNUSPALVELU_OSTO ||
                 alvkoodi == AlvKoodi::ALVVAHENNYS + AlvKoodi::YHTEISOMYYNTI_TAVARAT ||
                 alvkoodi == AlvKoodi::ALVVAHENNYS + AlvKoodi::YHTEISOHANKINNAT_PALVELUT ||
                 alvkoodi == AlvKoodi::ALVVAHENNYS + AlvKoodi::MAAHANTUONTI ||
                 alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_OSTO)
        {
            palautus += debet - kredit;
        }

        // Estetään alv-tileille kirjaaminen ilman alv-koodia
        if( indeksi.data(VientiModel::AlvKoodiRooli).toInt() == 0)
        {
            Tili tili = kp()->tilit()->tiliNumerolla( indeksi.data(VientiModel::TiliNumeroRooli).toInt() );
            if( tili.onko(TiliLaji::ALVSAATAVA) || tili.onko(TiliLaji::ALVVELKA))
            {
                QMessageBox::critical(nullptr, tr("Arvonlisäverokoodi puuttuu"),
                                      tr("Tilille %1 %2 on tehty kirjaus, jossa ei ole määritelty arvonlisäveron ohjaustietoja.\n\n"
                                         "Arvonlisäveroon liittyvät kirjaukset on aina määriteltävä oikeilla verokoodeilla, "
                                         "jotta kausiveroilmoitukseen saadaan oikeat tiedot.\n\n"
                                         "Käyttämällä Kirjausapuria saat automaattisesti oikeat arvonlisäveron ohjaustiedot." )
                                      .arg(tili.numero()).arg(tili.nimi()));
                return false;
            }
        }

        // #62: Estetään kirjaukset lukitulle tilikaudelle
        if( indeksi.data(VientiModel::PvmRooli).toDate() <= kp()->tilitpaatetty() &&
                indeksi.data(VientiModel::IdRooli).toInt() == 0)
        {
            QMessageBox::critical(nullptr, tr("Ei voi kirjata lukitulle tilikaudelle"),
                                  tr("Kirjaus %1 kohdistuu lukitulle tilikaudelle "
                                     "(kirjanpito lukittu %2 saakka)." )
                                  .arg( indeksi.data(VientiModel::PvmRooli).toDate().toString("dd.MM.yyyy"))
                                  .arg( kp()->tilitpaatetty().toString("dd.MM.yyyy")), QMessageBox::Cancel);
            return false;
        }

    }

    // Nyt tarkistetaan, täsmääkö

    if( qAbs( laskennallinenvero - vero) > riveja)
    {
        if( QMessageBox::critical(nullptr, tr("Arvonlisäveron kirjaus virheellinen"),
           tr("Maksettavan arvonlisäveron määrä ei täsmää veron perusteena olevan rahamäärän kanssa.\n"
              "Tallennetaanko tosite silti?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
            return false;
    }

    if( palautus + riveja < laskennallinenpalautus)
    {
        if( QMessageBox::critical(nullptr, tr("Arvonlisäveron kirjaus virheellinen"),
           tr("Arvonlisäveron palautus on pienempi kuin perusteena oleva määrä oikeuttaa.\n"
              "Tallennetaanko tosite silti?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
            return false;
    }

    if( laskennallinenpalautus + mahdollinenpalautus - riveja < palautus)
    {
        if( QMessageBox::critical(nullptr, tr("Arvonlisäveron kirjaus virheellinen"),
           tr("Arvonlisäveron palautus on suurempi kuin perusteena oleva määrä oikeuttaa.\n"
              "Tallennetaanko tosite silti?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
            return false;
    }

    if( alvvaro )
    {
        if( QMessageBox::critical(nullptr, tr("Arvonlisäveroilmoitus annettu"),
           tr("Arvonlisäveroilmoitus on annettu %1 saakka.\n\n"
              "Kirjanpitolaki 2. luku 7§ 2. mom:\n"
              "Tositteen, kirjanpidon tai muun kirjanpitoaineiston sisältöä ei saa muuttaa eikä "
              "poistaa sen jälkeen kuin 6§ tarkoitettu (kirjanpidosta viranomaisille verotusta "
              "tai muuta tarkoitusta varten määräajassa tehtävä) ilmoitus on tehty.\n\n"
              "Tallennetaanko tosite silti?").arg( kp()->asetukset()->pvm("AlvIlmoitus").toString("dd.MM.yyyy") ),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
            return false;
    }



    return true;
}
