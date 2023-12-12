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
#include "taydennysviennit.h"
#include "db/verotyyppimodel.h"
#include "db/kitsasinterface.h"
#include "db/tilimodel.h"
#include "db/tili.h"


TaydennysViennit::TaydennysViennit(KitsasInterface *interface) :
    kitsasInterface_(interface)
{

}

void TaydennysViennit::asetaEra(int eraId, QVariantList alkuperaisviennit)
{
    eraId_ = eraId;
    alkuperaiset_.clear();
    for(const auto & item : alkuperaisviennit)
        alkuperaiset_.append( TositeVienti( item.toMap()) );
}

QList<TositeVienti> TaydennysViennit::viennit(const QVariantList& omatViennit) const
{
    qlonglong omaSentit = 0L;
    QString selite;
    QDate pvm;

    QList<TositeVienti> out;

    for(const auto& vienti : omatViennit) {
        TositeVienti omaVienti(vienti.toMap());

        if(omaVienti.eraId() == eraId()) {
            omaSentit = omaVienti.kreditSnt() - omaVienti.debetSnt();
            selite = omaVienti.selite().isEmpty() ? omaVienti.kumppaniNimi() : omaVienti.selite();
            pvm = omaVienti.pvm();
            break;
        }
    }
    if( !omaSentit) return out;

    double osuusErasta = 0.0;

    for(const auto& avienti : qAsConst( alkuperaiset_)) {
        if( avienti.id() == eraId()) {
            const qlonglong asentit = avienti.debetSnt() - avienti.kreditSnt();
            if( !asentit ) return out;
            osuusErasta = omaSentit / asentit;
            break;
        }
    }
    if( qAbs(osuusErasta) < 1e-5) return out;

    for( const auto& avienti : qAsConst( alkuperaiset_)) {
        if( avienti.alvKoodi() / 100 == AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON / 100) {
            qlonglong sentit = qRound64( osuusErasta * ( avienti.kredit() - avienti.debet()  ) * 100.0 );

            TositeVienti debet;
            debet.setPvm( pvm );
            debet.setTili( avienti.tili() );
            debet.setDebet(sentit);
            debet.setSelite(selite);            
            debet.setEra(avienti.era());
            debet.setAlvKoodi(AlvKoodi::TILITYS);
            debet.setAlvProsentti( avienti.alvProsentti());
            if( taydennysDebetId_)
                debet.setId( taydennysDebetId_ );
            out << debet;

            TositeVienti kredit;
            kredit.setPvm(pvm);
            if( avienti.tili() == kitsasInterface_->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVVELKA).numero() ||
                avienti.alvKoodi() == AlvKoodi::ENNAKKOLASKU_MYYNTI + AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON ) {
                kredit.setTili( kitsasInterface_->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).numero() );
                kredit.setAlvKoodi( avienti.alvKoodi() % 100 + AlvKoodi::ALVKIRJAUS );
            } else if( avienti.tili() == kitsasInterface_->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVSAATAVA).numero() ) {
                kredit.setTili( kitsasInterface_->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA).numero() );
                kredit.setAlvKoodi( avienti.alvKoodi() % 100 + AlvKoodi::ALVVAHENNYS );
            }            
            kredit.setKredit(sentit);
            kredit.setSelite(selite);
            kredit.setAlvProsentti(avienti.alvProsentti());
            if( taydennysKreditId_ )
                kredit.setId( taydennysKreditId_ );
            out << kredit;
        }
    }

    return out;
}




