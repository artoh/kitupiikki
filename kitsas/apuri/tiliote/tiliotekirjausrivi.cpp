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
#include "tiliotekirjausrivi.h"

#include "db/kitsasinterface.h"
#include "tiliotemodel.h"
#include "db/tilimodel.h"
#include "db/tilikausimodel.h"
#include "db/kohdennusmodel.h"
#include "db/asetusmodel.h"
#include "db/tili.h"

TilioteKirjausRivi::TilioteKirjausRivi()
{

}

TilioteKirjausRivi::TilioteKirjausRivi(const QVariantList &data, TilioteModel *model) :
    TilioteRivi(model)
{
    for(auto const &vienti : data) {
        viennit_.append(TositeVienti(vienti.toMap()));
    }
}

TilioteKirjausRivi::TilioteKirjausRivi(const QVariantMap &tuonti, TilioteModel *model) :
    TilioteRivi(model), tuotu_(true)
{
    TositeVienti pankki;
    TositeVienti tapahtuma;

    pankki.setTili(model->tilinumero());

    const QDate& pvm = tuonti.value("pvm").toDate();
    pankki.setPvm(pvm);
    tapahtuma.setPvm(pvm);

    const QString& euroa = tuonti.value("euro").toString();
    pankki.setDebet(euroa);
    tapahtuma.setKredit(euroa);

    const QString& selite = tuonti.value("selite").toString();
    pankki.setSelite(selite);
    tapahtuma.setSelite(selite);

    tapahtuma.setKohdennus( tuonti.value("kohdennus").toInt());

    const QString& kumppaniNimi = tuonti.value("saajamaksaja").toString();
    int kumppaniId = tuonti.value("saajamaksajaid").toInt();
    const QString& iban = tuonti.value("iban").toString();

    QVariantMap kumppani;
    if(kumppaniId) kumppani.insert("id", kumppaniId);
    if(!kumppaniNimi.isEmpty()) kumppani.insert("nimi", kumppaniNimi);
    if(!iban.isEmpty()) kumppani.insert("iban", QVariantList() << iban);

    pankki.setKumppani(kumppani);
    tapahtuma.setKumppani(kumppani);

    pankki.setArkistotunnus(tuonti.value("arkistotunnus").toString());
    tapahtuma.setEra(tuonti.value("era").toMap());

    int tilinumero = tuonti.value("tili").toInt();
    if(!tilinumero) {
        if(pankki.debet() > 1e-5 && model->kitsas()->asetukset()->onko("TilioteTuloKaytossa") ) {
            tilinumero = model->kitsas()->asetukset()->luku("TilioteTulotili");
        } else if( pankki.kredit() > 1e-5 && model->kitsas()->asetukset()->onko("TilioteMenoKaytossa")) {
            tilinumero = model->kitsas()->asetukset()->luku("TilioteMenotili");
        }
    }
    tapahtuma.setTili(tilinumero);

    const QString& viite = tuonti.value("viite").toString();
    if( !viite.isEmpty() )
        pankki.setViite(viite);

    viennit_ << pankki << tapahtuma;
    paivitaTyyppi();
}

QVariant TilioteKirjausRivi::riviData(int sarake, int role) const
{    
    const TositeVienti& ekavienti = viennit_.value(1);

    switch (role) {
    case Qt::DisplayRole:
        switch (sarake) {
        case PVM:
            return ekavienti.pvm();
        case SAAJAMAKSAJA:
            return ekavienti.kumppaniNimi();
        case SELITE:
            if(ekavienti.selite().isEmpty())
                return pankkivienti().viite();
            return ekavienti.selite();
        case TILI:
        {
            if( viennit_.count() == 2) {
                return model()->kitsas()->tilit()->tiliNumerolla(ekavienti.tili()).nimiNumero();
            } else {
                QStringList strlist;
                for(int i=1; i < viennit_.count(); i++) {
                    strlist << QString::number(viennit_.at(i).tili());
                }
                return strlist.join(", ");
            }
        }
        case KOHDENNUS:
        {
            if( viennit_.count() > 2) {
                return QString("...");
            } else if(ekavienti.kohdennus()) {
                return model()->kitsas()->kohdennukset()->kohdennus( ekavienti.kohdennus() ).nimi();
            } else if(ekavienti.eraId()) {
                const QVariantMap& eraMap = ekavienti.era();
                return model()->kitsas()->tositeTunnus(eraMap.value("tunniste").toInt(),
                                                       eraMap.value("pvm").toDate(),
                                                       eraMap.value("sarja").toString());
            } else {
                return QVariant();
            }

        }
        case EURO: {
            const TositeVienti& vasta = viennit_.value(0);
            double summa = vasta.debet() - vasta.kredit();
            return qAbs(summa) > 1e-5 ? QString("%L1 €").arg( summa,0,'f',2) : QVariant();
            }
        default:
            return QVariant();
        }

    case LajitteluRooli:
        return QString("%1 %2").arg(ekavienti.pvm().toString(Qt::ISODate))
                                .arg(lisaysIndeksi(),6,10,QChar('0'));
    case Qt::TextAlignmentRole:
        return sarake == EURO ? QVariant(Qt::AlignRight | Qt::AlignVCenter) : QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    case TilaRooli:
        return peitetty() ? "-" : "AA";
    case Qt::TextColorRole:
        return peitetty() ? QColor(Qt::magenta) : QVariant();
    default:
        return QVariant();
    }

}

QList<TositeVienti> TilioteKirjausRivi::viennit() const
{
    return viennit_;
}

TositeVienti TilioteKirjausRivi::pankkivienti() const
{
    return viennit_.value(0);
}

QVariantList TilioteKirjausRivi::tallennettavat() const
{
    QVariantList ulos;
    for(auto const& vienti : viennit_) {
        ulos << vienti;
    }
    return ulos;
}


void TilioteKirjausRivi::paivitaTyyppi()
{
    int tyyppi = 0;
    const Tili* tili = model()->kitsas()->tilit()->tili( pankkivienti().tili() );

    if( viennit_.value(0).eraId() > 0) {
        tyyppi = TositeVienti::SUORITUS;
    } else if( tili && tili->onko(TiliLaji::TASE) ) {
        tyyppi = TositeVienti::SIIRTO;
    } else if( pankkivienti().debet() > 1e-5 ) {
        tyyppi = TositeVienti::MYYNTI;
    } else if( pankkivienti().kredit() > 1e-5) {
        tyyppi = TositeVienti::OSTO;
    }

    if(!viennit_.isEmpty())
        viennit_[0].setTyyppi(TositeVienti::VASTAKIRJAUS + tyyppi);
    for(int i=1; i < viennit_.count(); i++) {
        if( viennit_[i].tyyppi() % 100 < TositeVienti::ALVKIRJAUS) {
            viennit_[i].setTyyppi(TositeVienti::KIRJAUS + tyyppi);
        }
    }
}


