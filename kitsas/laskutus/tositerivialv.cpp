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
#include "tositerivialv.h"
#include "db/verotyyppimodel.h"
#include "model/tositerivit.h"
#include "model/lasku.h"

TositeriviAlv::TositeriviAlv(TositeRivit *rivit) :
    rivit_(rivit)
{
    tiedot_ << AlvTieto(AlvKoodi::MYYNNIT_NETTO, 24.0)
            << AlvTieto(AlvKoodi::MYYNNIT_NETTO, 14.0)
            << AlvTieto(AlvKoodi::MYYNNIT_NETTO, 10.0)
            << AlvTieto(AlvKoodi::EIALV, 0.0)
            << AlvTieto(AlvKoodi::ALV0, 0.0)
            << AlvTieto(AlvKoodi::RAKENNUSPALVELU_MYYNTI, 0.0)
            << AlvTieto(AlvKoodi::YHTEISOMYYNTI_PALVELUT, 0.0)
            << AlvTieto(AlvKoodi::YHTEISOMYYNTI_TAVARAT, 0.0)
            << AlvTieto(Lasku::KAYTETYT, 0.0)
            << AlvTieto(Lasku::TAIDE, 0.0)
            << AlvTieto(Lasku::ANTIIKKI, 0.0);
}

void TositeriviAlv::yhdistaRiveihin(TositeRivit *rivit)
{
    rivit_ = rivit;
}

Euro TositeriviAlv::netto(int indeksi) const
{
    if(indeksi < 0)
        return netto_;
    else
        return tiedot_.at(indeksi).netto();
}

Euro TositeriviAlv::vero(int indeksi) const
{
    if(indeksi < 0)
        return vero_;
    else
        return tiedot_.at(indeksi).vero();
}

Euro TositeriviAlv::brutto(int indeksi) const
{
    if(indeksi < 0)
        return brutto_;
    else
        return tiedot_.at(indeksi).brutto();
}

int TositeriviAlv::alvkoodi(int indeksi) const
{
    return tiedot_.at(indeksi).verokoodi();
}

double TositeriviAlv::veroprosentti(int indeksi) const
{
    return tiedot_.at(indeksi).veroProsentti();
}

void TositeriviAlv::paivita()
{
    for(int i=0; i < tiedot_.count(); i++)
        tiedot_[i].clear();

    for(int i=0; i < rivit_->rowCount(); i++) {
        const TositeRivi& rivi = rivit_->rivi(i);
        int alvKoodi = rivi.alvkoodi();
        double alvProsentti = rivi.alvProsentti();

        for(int j=0; j < tiedot_.count(); j++) {
            if( alvkoodi(j) == alvKoodi &&
                qAbs( veroprosentti(j) - alvProsentti  ) < 1e-5) {
                tiedot_[j].lisaaNettoon( Euro::fromDouble(rivi.nettoYhteensa()) );
                if( bruttoPeruste())
                    tiedot_[j].lisaaBruttoon( rivi.bruttoYhteensa());
                break;
            }
        }
    }



    for(int i=0; i < tiedot_.count(); i++)
        tiedot_[i].paivita(bruttoperuste_);

    netto_ = Euro(0);
    vero_ = Euro(0);
    brutto_ = Euro(0);

    for(const auto& tieto : qAsConst( tiedot_ )) {
        netto_ += tieto.netto();
        vero_ += tieto.vero();
        brutto_ += tieto.brutto();
    }
}

QList<int> TositeriviAlv::indeksitKaytossa() const
{
    QList<int> lista;
    for(int i=0; i < tiedot_.count(); i++)  {
        if( netto(i).cents())
            lista << i;
    }
    return lista;
}

bool TositeriviAlv::veroton() const
{
    for(auto tieto : tiedot_) {
        if( tieto.netto().cents() && tieto.verokoodi() != AlvKoodi::EIALV )
            return false;
    }
    return true;
}

TositeriviAlv::AlvTieto::AlvTieto()
{

}

TositeriviAlv::AlvTieto::AlvTieto(int verokoodi, double veroprosentti) :
    verokoodi_(verokoodi), veroProsentinSadasosat_(qRound64(veroprosentti*100))
{

}

void TositeriviAlv::AlvTieto::paivita(bool bruttoperuste)
{
    if( bruttoperuste )
        vero_ = brutto_ - netto_;
    else if( verokoodi_ == AlvKoodi::MYYNNIT_NETTO ) {
        vero_ = Euro( qRound64(netto_.toDouble() *  veroProsentinSadasosat_ / 100.0) );
        brutto_ = netto_ + vero_;
    } else {
        brutto_ = netto_;
    }

}

void TositeriviAlv::AlvTieto::clear()
{
    netto_ = Euro(0);
    vero_ = Euro(0);
    brutto_ = Euro(0);
}
