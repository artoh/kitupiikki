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
#include "tulomenorivi.h"
#include "db/verotyyppimodel.h"
#include "model/tositevienti.h"
#include "db/kirjanpito.h"
#include "model/tosite.h"
#include "db/tositetyyppimodel.h"

TulomenoRivi::TulomenoRivi()
{

}

TulomenoRivi::TulomenoRivi(const QVariantMap &data)
{
    TositeVienti vienti( data );

    vientiId_ = vienti.id();
    tilinumero_ = vienti.tili();
    selite_ = vienti.selite();
    alvkoodi_ = vienti.alvKoodi();
    veroprosentti_ = vienti.alvProsentti();
    kohdennus_ = vienti.kohdennus();
    merkkaukset_ = vienti.merkkaukset();
    jaksoalkaa_ = vienti.jaksoalkaa();
    jaksoloppuu_ = vienti.jaksoloppuu();
    poistoaika_ = vienti.tasaerapoisto();

    // Sitten netto/brutto tilanteen mukaan

    int verokoodi = alvkoodi();
    bool nettoon = verokoodi == AlvKoodi::MYYNNIT_NETTO  || verokoodi == AlvKoodi::OSTOT_NETTO ||
            verokoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI || verokoodi== AlvKoodi::MAKSUPERUSTEINEN_OSTO ||
            verokoodi == AlvKoodi::RAKENNUSPALVELU_OSTO || verokoodi== AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
            verokoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verokoodi == AlvKoodi::MAAHANTUONTI ;

    qlonglong maara = vienti.tyyppi() == TositeVienti::MYYNTI + TositeVienti::KIRJAUS ?
                qRound64(vienti.kredit() * 100.0) - qRound64(vienti.debet() * 100.0) :
                qRound64(vienti.debet() * 100.0) - qRound64(vienti.kredit() * 100.0);

    if( nettoon )
        setNetto( maara);
    else
        setBrutto( maara );

}

void TulomenoRivi::setAlvkoodi(int koodi)
{
    alvkoodi_ = koodi;
}

void TulomenoRivi::setAlvvahennys(bool vahennys)
{
   alvvahennys_ = vahennys;
}

qlonglong TulomenoRivi::brutto() const
{
    if( netto_ ) {
        return qRound64( ( 100 + veroprosentti_ ) * netto_ / 100.0);
    } else
        return brutto_;
}

qlonglong TulomenoRivi::netto() const
{
    if( brutto_ ) {
        qlonglong vero = qRound64( brutto_ * veroprosentti_ / ( 100 + veroprosentti_) );
        return brutto_ - vero;
    } else
        return netto_;
}

void TulomenoRivi::setBrutto(qlonglong sentit)
{
    brutto_ = sentit;
    netto_ = 0;
}

void TulomenoRivi::setNetto(qlonglong sentit)
{
    netto_ = sentit;
    brutto_ = 0;
}

bool TulomenoRivi::naytaBrutto() const
{
    return !( alvkoodi() == AlvKoodi::RAKENNUSPALVELU_OSTO || alvkoodi() == AlvKoodi::YHTEISOHANKINNAT_TAVARAT
                                 || alvkoodi() == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || alvkoodi() == AlvKoodi::MAAHANTUONTI
                                 || alvkoodi() == AlvKoodi::MAAHANTUONTI_VERO)  ;

}

bool TulomenoRivi::naytaNetto() const
{
    return  alvkoodi() == AlvKoodi::OSTOT_NETTO || alvkoodi() == AlvKoodi::MYYNNIT_NETTO ||
                                 alvkoodi() == AlvKoodi::OSTOT_BRUTTO || alvkoodi() == AlvKoodi::MYYNNIT_BRUTTO ||
                                 alvkoodi() == AlvKoodi::MAKSUPERUSTEINEN_OSTO || alvkoodi() == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI ||
            !naytaBrutto() ;
}

bool TulomenoRivi::naytaVahennysvalinta() const
{
    return alvkoodi() == AlvKoodi::RAKENNUSPALVELU_OSTO ||
            alvkoodi() == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
            alvkoodi() == AlvKoodi::YHTEISOHANKINNAT_PALVELUT ||
            alvkoodi() == AlvKoodi::MAAHANTUONTI ||
            alvkoodi() == AlvKoodi::MAAHANTUONTI_VERO;
}

QVariantList TulomenoRivi::viennit(Tosite* tosite) const
{
    QVariantList vientilista;
    // Ensin varsinainen vienti

    if( !tilinumero() || qAbs(this->netto()) < 1e-5)
        return vientilista;

    double maara = this->brutto() / 100.0 ;
    double netto = this->netto() / 100.0;
    double vero = (this->brutto() - this->netto() ) / 100.0;

    bool menoa = tosite->tyyppi() == TositeTyyppi::MENO ||
                 tosite->tyyppi() == TositeTyyppi::KULULASKU;
    QDate pvm = tosite->data(Tosite::PVM).toDate();

    QString otsikko = selite().isEmpty() ?
                tosite->otsikko() :
                selite();

    TositeVienti vienti;

    int verokoodi = alvkoodi();
    bool maahantuonninvero = false;
    if( verokoodi == AlvKoodi::MAAHANTUONTI_VERO) {
        maahantuonninvero = true;
        verokoodi = AlvKoodi::MAAHANTUONTI;
    }


    vienti.setTyyppi( ( menoa ? TositeVienti::OSTO : TositeVienti::MYYNTI) + TositeVienti::KIRJAUS );
    if( vientiId_)
        vienti.setId( vientiId_);

    vienti.setTili( tilinumero() );
    vienti.setPvm(pvm);
    vienti.setSelite( otsikko );
    vienti.setKohdennus( kohdennus() );
    vienti.setMerkkaukset( merkkaukset() );

    if( jaksoalkaa().isValid())
        vienti.setJaksoalkaa( jaksoalkaa() );
    if( jaksoalkaa().isValid() && jaksopaattyy().isValid())
    vienti.setJaksoloppuu( jaksopaattyy());

    vienti.setAlvKoodi( verokoodi);
    if( verokoodi != AlvKoodi::EIALV && verokoodi != AlvKoodi::RAKENNUSPALVELU_MYYNTI &&
        verokoodi != AlvKoodi::YHTEISOMYYNTI_TAVARAT && verokoodi != AlvKoodi::YHTEISOMYYNTI_PALVELUT)
        vienti.setAlvProsentti( alvprosentti());

    if( tosite->kumppani())
        vienti.setKumppani( tosite->kumppani() );

    if( poistoaika())
        vienti.setTasaerapoisto( poistoaika());


    double kirjattava = ( verokoodi == AlvKoodi::MYYNNIT_NETTO  || verokoodi == AlvKoodi::OSTOT_NETTO ||
                             verokoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI || verokoodi== AlvKoodi::MAKSUPERUSTEINEN_OSTO ||
                             verokoodi == AlvKoodi::RAKENNUSPALVELU_OSTO || verokoodi== AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
                             verokoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verokoodi == AlvKoodi::MAAHANTUONTI
                        ) ? netto : maara;

    if( menoa ) {
        if( kirjattava > 1e-5)
            vienti.setDebet(kirjattava);
        else
            vienti.setKredit(0-kirjattava);
    } else {
        if( kirjattava > 1e-5)
            vienti.setKredit( kirjattava );
        else
            vienti.setDebet( 0-kirjattava );
    }

    vientilista.append( vienti );

    // ALV-SAAMINEN
    if( verokoodi == AlvKoodi::OSTOT_NETTO || verokoodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO ||
          ((verokoodi == AlvKoodi::RAKENNUSPALVELU_OSTO || verokoodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
            verokoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verokoodi == AlvKoodi::MAAHANTUONTI )
           &&  alvvahennys() ) ) {

        TositeVienti palautus;
        palautus.setTyyppi( TositeVienti::OSTO + TositeVienti::ALVKIRJAUS );

        palautus.setPvm(pvm);
        if( verokoodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO) {
            palautus.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVSAATAVA).numero() );
            palautus.setAlvKoodi( AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_OSTO );
        } else {
            palautus.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA).numero());
            palautus.setAlvKoodi( AlvKoodi::ALVVAHENNYS + verokoodi );
        }
        if( vero > 0)
            palautus.setDebet( vero );
        else
            palautus.setKredit( 0 - vero);

        palautus.setAlvProsentti( alvprosentti() );
        palautus.setSelite( otsikko );
        vientilista.append(palautus);
    }

    // Alv-veron kirjaaminen
    if( verokoodi == AlvKoodi::MYYNNIT_NETTO || verokoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI ||
            verokoodi == AlvKoodi::RAKENNUSPALVELU_OSTO || verokoodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
            verokoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verokoodi == AlvKoodi::MAAHANTUONTI )
    {
        TositeVienti verorivi;
        verorivi.setTyyppi( TositeVienti::MYYNTI + TositeVienti::ALVKIRJAUS );
        verorivi.setPvm(pvm);
        if( verokoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI) {
            verorivi.setTili( kp()->tilit()->tiliTyypilla( TiliLaji::KOHDENTAMATONALVVELKA ).numero() );
            verorivi.setAlvKoodi( AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_MYYNTI);
        } else {
            verorivi.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).numero());
            verorivi.setAlvKoodi( AlvKoodi::ALVKIRJAUS + verokoodi);
        }

        if( vero > 0)
            verorivi.setKredit( vero );
        else
            verorivi.setDebet( 0 - vero);

        verorivi.setAlvProsentti( alvprosentti());
        verorivi.setSelite(otsikko);
        vientilista.append(verorivi);
    }

    // Mahdollinen maahantuonnin veron kirjaamisen vastakirjaaminen
    if( maahantuonninvero ) {
        TositeVienti tuonti;
        tuonti.setTyyppi(TositeVienti::OSTO + TositeVienti::MAAHANTUONTIVASTAKIRJAUS);
        tuonti.setPvm(pvm);
        tuonti.setTili( tilinumero() );
        if( netto > 0)
            tuonti.setKredit(netto);
        else
            tuonti.setDebet( 0 - netto);

        tuonti.setSelite(otsikko);
        tuonti.setAlvKoodi(AlvKoodi::MAAHANTUONTI_VERO);
        vientilista.append(tuonti);
    }
    // Jos ei oikeuta alv-vähennykseen, kirjataan myös tämä osuus menoksi
    if( (verokoodi == AlvKoodi::RAKENNUSPALVELU_OSTO || verokoodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
            verokoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verokoodi == AlvKoodi::MAAHANTUONTI )
           &&  !alvvahennys()  ) {

        TositeVienti palautus;
        palautus.setPvm( pvm );
        palautus.setTyyppi( TositeVienti::OSTO + TositeVienti::VAHENNYSKELVOTON );
        palautus.setTili( tilinumero() );
        palautus.setAlvKoodi( AlvKoodi::VAHENNYSKELVOTON);
        palautus.setAlvProsentti( alvprosentti() );

        if( vero > 0)
            palautus.setDebet( vero );
        else
            palautus.setKredit( 0 - vero);

        palautus.setSelite( otsikko );
        vientilista.append(palautus);
    }

    return vientilista;
}


