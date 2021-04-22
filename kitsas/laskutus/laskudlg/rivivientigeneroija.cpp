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
#include "rivivientigeneroija.h"

#include "model/tosite.h"
#include "model/tositevienti.h"
#include "model/tositeviennit.h"
#include "model/tositerivi.h"
#include "model/tositerivit.h"

#include "db/kitsasinterface.h"
#include "db/asetusmodel.h"
#include "db/tilimodel.h"


#include "kitsas.h"


RiviVientiGeneroija::RiviVientiGeneroija(KitsasInterface *kitsas) :
    kitsas_(kitsas)
{

}

void RiviVientiGeneroija::generoiViennit(Tosite *tosite, int eraId)
{
    tosite_ = tosite;

    asetaEraId(eraId);
    tosite->viennit()->tyhjenna();

    Lasku& lasku = tosite_->lasku();
    if( lasku.valvonta() == Lasku::VAKIOVIITE ||
        lasku.valvonta() == Lasku::VALVOMATON )
        return;     // Ei vientejä!
                    // Pitäisikö tehdä nollavienti jotta tulee listaan?

    alv_.yhdistaRiveihin( tosite->rivit() );
    alv_.asetaBruttoPeruste( lasku.riviTyyppi() == Lasku::BRUTTORIVIT );
    alv_.paivita();

    if( lasku.maksutapa() == Lasku::KUUKAUSITTAINEN ) {
        generoiKuukausittaisetLaskut();
    } else {
        generoiViennit(tosite->pvm());
    }


    lasku.setSumma( alv_.brutto() );

}

void RiviVientiGeneroija::asetaEraId(int eraId)
{
    if(eraId) {
        eraId_ = eraId;
        return;
    }

    const Lasku& lasku = tosite_->constLasku();

    if( tosite_->viennit()->rowCount())
        vastaVientiId_ = tosite_->viennit()->vienti(0).id();

    int valvonta = lasku.valvonta();

    if( valvonta == Lasku::ASIAKAS || valvonta == Lasku::HUONEISTO) {
        eraId_ = lasku.viite().eraId();
    } else if( lasku.maksutapa() == Lasku::KATEINEN ) {
        eraId_ = 0;
    } else if( tosite_->viennit()->rowCount() &&
               tosite_->viennit()->vienti(0).eraId()) {
        eraId_ = tosite_->viennit()->vienti(0).eraId();
    } else {
        eraId_ = Kitsas::UUSI_ERA;
    }
}

void RiviVientiGeneroija::generoiKuukausittaisetLaskut()
{
    const Lasku& lasku = tosite_->constLasku();
    QDate pvm = lasku.toimituspvm();
    const QDate& loppupvm = lasku.jaksopvm();
    const int erapvm = lasku.toistuvanErapaiva();

    if( !pvm.isValid() || !loppupvm.isValid() )
        return;

    if( pvm.day() > erapvm)
        pvm = pvm.addMonths(1);

    while( pvm < loppupvm) {
        QDate kkpvm = QDate( pvm.year(), pvm.month(), erapvm);
        generoiViennit(kkpvm);
        pvm  = pvm.addMonths(1);
    }
}

void RiviVientiGeneroija::generoiViennit(const QDate &pvm)
{
    generoiVastavienti(pvm);
    generoiTiliviennit(pvm);
    generoiVeroviennit(pvm);
}

void RiviVientiGeneroija::generoiVastavienti(const QDate &pvm)
{
    TositeVienti vienti;
    vienti.setPvm( pvm );
    if(eraId_ > 0)
        vienti.setId(vastaVientiId_);

    const Lasku& lasku = tosite_->constLasku();
    const AsetusModel* asetukset = kitsas_->asetukset();
    const int maksutapa = lasku.maksutapa();

    if( maksutapa == Lasku::KATEINEN) {
        vienti.setTili( asetukset->luku(AsetusModel::LaskuKateistili) );
    } else if (maksutapa == Lasku::ENNAKKOLASKU) {
        vienti.setTili( asetukset->luku(AsetusModel::LaskuEnnakkosaatavaTili) );
    }  else {
        vienti.setTili( asetukset->luku(AsetusModel::LaskuSaatavaTili));
    }

    vienti.setDebet( alv_.brutto() );

    vienti.setKumppani( tosite_->kumppani() );
    vienti.setTyyppi( TositeVienti::MYYNTI + TositeVienti::VASTAKIRJAUS );
    vienti.setSelite( lasku.otsikko() );
    vienti.setEra(eraId_);
    tosite_->viennit()->lisaa(vienti);
}

void RiviVientiGeneroija::generoiTiliviennit(const QDate &pvm)
{
    const Lasku& lasku = tosite_->constLasku();

    // Tehdään viennit jokaista tilikohdennusyhdistelmää varten
    QMap<QString,Euro> eurot;
    for(int i=0; i < tosite_->rivit()->rowCount(); i++) {
        const TositeRivi& rivi = tosite_->rivit()->rivi(i);
        Euro rivisumma = Euro::fromDouble(rivi.nettoYhteensa());
        if( !rivisumma)
            continue;

        int tili = lasku.maksutapa() == Lasku::ENNAKKOLASKU ? kitsas_->asetukset()->luku(AsetusModel::LaskuEnnakkoTili) :  rivi.tili();
        int kohdennus = rivi.ennakkoEra() ? 0 - rivi.ennakkoEra() : rivi.kohdennus();
        QString str = QString("%1/%2/%3/%4")
                .arg(tili)
                .arg(kohdennus)
                .arg(rivi.alvkoodi())
                .arg(rivi.alvProsentti(),0,'f',2);
        for( auto& merkkaus : rivi.merkkaukset()) {
            str.append(QString("/%1").arg(merkkaus.toString()));
        }
        Euro aiempi = eurot.value(str);
        eurot.insert(str, aiempi + rivisumma);
    }

    // Sitten puretaan nämä kirjauksiksi
    QMapIterator<QString,Euro> iter(eurot);
    while( iter.hasNext()) {
        iter.next();
        QStringList strlist = iter.key().split("/");

        TositeVienti vienti;
        vienti.setTyyppi(TositeVienti::MYYNTI + TositeVienti::KIRJAUS);
        vienti.setPvm( pvm );
        vienti.setTili( strlist.takeFirst().toInt() );
        int kohdennus = strlist.takeFirst().toInt();
        if( kohdennus < 0) {
            vienti.setEra( 0 - kohdennus);
        } else {
            vienti.setKohdennus( kohdennus );
        }

        int alvkoodi = strlist.takeFirst().toInt();
        int alvprosentti = strlist.takeFirst().toDouble();

        if(alvkoodi >= Lasku::KAYTETYT) {
            alvkoodi = AlvKoodi::MYYNNIT_MARGINAALI;
            alvprosentti = 24.0;
        } else if( alvkoodi == AlvKoodi::MYYNNIT_NETTO &&
                   lasku.maksutapa() == Lasku::ENNAKKOLASKU) {
            alvkoodi = AlvKoodi::ENNAKKOLASKU_MYYNTI;
            vienti.setEra(-1);  // Ennakkolaskujen erät seurannassa
        } else if( alvkoodi == AlvKoodi::MYYNNIT_NETTO &&
                 kitsas_->onkoMaksuperusteinenAlv(pvm)) {
            alvkoodi = AlvKoodi::MAKSUPERUSTEINEN_MYYNTI;            
        }

        vienti.setAlvKoodi(alvkoodi);
        vienti.setAlvProsentti(alvprosentti);

        QVariantList merkkauslista;
        for(auto& merkkaus : strlist) {
            merkkauslista << merkkaus;
        }
        if( !merkkauslista.isEmpty() )
            vienti.setMerkkaukset(merkkauslista);

        vienti.setKredit( iter.value() );
        vienti.setSelite( lasku.otsikko() );
        vienti.setKumppani( tosite_->kumppani() );


        if( lasku.maksutapa() != Lasku::KUUKAUSITTAINEN) {
            vienti.setJaksoalkaa( lasku.toimituspvm() );
            vienti.setJaksoloppuu( lasku.jaksopvm());
        }

        tosite_->viennit()->lisaa(vienti);
    }

}

void RiviVientiGeneroija::generoiVeroviennit(const QDate &pvm)
{
    QList<int> indeksit = alv_.indeksitKaytossa();
    for(int indeksi : indeksit) {
        int verokoodi = alv_.alvkoodi(indeksi);
        if( verokoodi == AlvKoodi::MYYNNIT_NETTO && alv_.vero(indeksi).cents()) {
            generoiVeroVienti( alv_.veroprosentti(indeksi), alv_.vero(indeksi), pvm );
        }
    }
}

void RiviVientiGeneroija::generoiVeroVienti(const double prosentti, const Euro& vero, const QDate& pvm)
{
    TositeVienti vienti;
    vienti.setPvm(pvm);
    vienti.setTyyppi(TositeVienti::MYYNTI + TositeVienti::ALVKIRJAUS);

    const Lasku& lasku = tosite_->lasku();

    if( lasku.maksutapa() != Lasku::KATEINEN &&
        kitsas_->onkoMaksuperusteinenAlv(pvm) ) {
        vienti.setAlvKoodi( AlvKoodi::MAKSUPERUSTEINEN_MYYNTI + AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON );
        vienti.setTili( kitsas_->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVVELKA).numero() );
    } else if( lasku.maksutapa() == Lasku::ENNAKKOLASKU ) {
        vienti.setAlvKoodi( AlvKoodi::ENNAKKOLASKU_MYYNTI + AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON);
        vienti.setTili( kitsas_->asetukset()->luku(AsetusModel::EnnakkoAlvTili) );
    } else {
        vienti.setAlvKoodi( AlvKoodi::MYYNNIT_NETTO + AlvKoodi::ALVKIRJAUS);
        vienti.setTili( kitsas_->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).numero() );
    }

    vienti.setAlvProsentti( prosentti );
    vienti.setKredit( vero );
    vienti.setKumppani( tosite_->kumppani() );
    vienti.setSelite( QString("%1 %2 %L3").arg(lasku.otsikko())
                        .arg(kitsas_->kaanna("alv"))
                        .arg(prosentti,0,'f',2));

    tosite_->viennit()->lisaa(vienti);

}


