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
#include "db/verotyyppimodel.h"
#include "db/tositetyyppimodel.h"

#include <QRandomGenerator>

TilioteKirjausRivi::TilioteKirjausRivi() :
    taydennys_(nullptr)
{

}

TilioteKirjausRivi::TilioteKirjausRivi(const QVariantList &data, TilioteModel *model) :
    TilioteRivi(model), taydennys_(model->kitsas())
{
    for(auto const &vienti : data) {
        viennit_.append(TositeVienti(vienti.toMap()));
    }
}

TilioteKirjausRivi::TilioteKirjausRivi(const QVariantMap &tuonti, TilioteModel *model) :
    TilioteRivi(model), tuotu_(true), taydennys_(model->kitsas())
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

    const QString& arkistotunnus = tuonti.value("arkistotunnus").toString();
    pankki.setArkistotunnus( arkistotunnus.length() > 10 ? arkistotunnus : pseudoarkistotunnus() );


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
    paivitaErikoisrivit();
}

TilioteKirjausRivi::TilioteKirjausRivi(const QDate &pvm, TilioteModel *model) :
    TilioteRivi(model), taydennys_(model->kitsas())
{
    TositeVienti pankki;
    TositeVienti tapahtuma;

    pankki.setPvm(pvm);
    tapahtuma.setPvm(pvm);
    pankki.setTili(model->tilinumero());
    pankki.setArkistotunnus(pseudoarkistotunnus());

    viennit_ << pankki << tapahtuma;
}

TilioteKirjausRivi::TilioteKirjausRivi(const QList<TositeVienti> &viennit, TilioteModel *model) :
    TilioteRivi(model), viennit_(viennit), taydennys_(model->kitsas())
{

}

QVariant TilioteKirjausRivi::riviData(int sarake, int role) const
{        
    const TositeVienti& ekavienti = viennit_.value(1);

    switch (role) {
    case Qt::DisplayRole:
    case LajitteluRooli:
        switch (sarake) {
        case PVM:
            if( role == LajitteluRooli) {
                return QString("%1 %2").arg(pankkivienti().pvm().toString(Qt::ISODate))
                                        .arg(lisaysIndeksi(),6,10,QChar('0'));
            } else
                return pankkivienti().pvm();
        case SAAJAMAKSAJA:
            return pankkivienti().kumppaniNimi();
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
                int eraId = eraMap.value("id").toInt();
                if( eraId > 0) {
                    return model()->kitsas()->tositeTunnus(eraMap.value("tunniste").toInt(),
                                                       eraMap.value("pvm").toDate(),
                                                       eraMap.value("sarja").toString());
                } else if( eraMap.contains("huoneisto")) {
                    return eraMap.value("huoneisto").toMap().value("nimi").toString();
                }
            }
            return QVariant();
        }
        case EURO: {            
            const double summa = pankkivienti().debet() - pankkivienti().kredit();
            return qAbs(summa) > 1e-5 ? QString("%L1 €").arg( summa,0,'f',2) : QVariant();
            }
        default:
            return QVariant();
        }

    case Qt::EditRole:       
        switch (sarake) {
        case PVM:           
            return ekavienti.pvm();
        case TILI:
            return ekavienti.tili();
        case EURO:
            return pankkivienti().debet() > 1e-5 ? pankkivienti().debet() : 0 - pankkivienti().kredit();
        case KOHDENNUS:
            return ekavienti.kohdennus();
        case SAAJAMAKSAJA:
            return ekavienti.kumppaniMap();
        case SELITE:
            return ekavienti.selite();
        }

    case Qt::UserRole:
        return sarake == SAAJAMAKSAJA ? ekavienti.kumppaniId() : QVariant();
    case Qt::DecorationRole:
        if(sarake == KOHDENNUS && viennit_.count() == 2) {
            if( ekavienti.kohdennus()) {
                return model()->kitsas()->kohdennukset()->kohdennus( ekavienti.kohdennus() ).tyyppiKuvake();
            } else if(ekavienti.era().contains("huoneisto")) {
                return QIcon(":/pic/talo.png");
            } else if( ekavienti.era().contains("asiakas")) {
                return QIcon(":/pic/mies.png"); // Asiakaskohtainen lasku
            } else if( ekavienti.eraId() > 0) {
                return model()->kitsas()->tositeTyypit()->kuvake( ekavienti.era().value("tositetyyppi").toInt() );
            } else {
                return QIcon(":/pic/tyhja.png");
            }
        } else if( sarake == EURO) {
            const int koodi = pankkivienti().tyyppi() - TositeVienti::VASTAKIRJAUS;
            const qlonglong summa = pankkivienti().debetSnt() - pankkivienti().kreditSnt();
            if( !summa ) return QVariant();
            if( koodi == TositeVienti::MYYNTI ) {
                return summa > 0 ? QIcon(":/pic/lisaa.png") : QIcon(":/pic/edit-undo.png");
            } else if( koodi == TositeVienti::OSTO) {
                return summa < 0 ? QIcon(":/pic/poista.png") : QIcon(":/pic/edit-undo.png");
            } else if( koodi == TositeVienti::SIIRTO) {
                return QIcon(":/pic/siirra.png");
            } else if( koodi == TositeVienti::SUORITUS) {
                return QIcon(":/pic/lasku.png");
            }
        }
        return QVariant();

    case Qt::TextAlignmentRole:
        return sarake == EURO ? QVariant(Qt::AlignRight | Qt::AlignVCenter) : QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    case TilaRooli:
        return peitetty() ? "-" : "AA";
    case LisaysIndeksiRooli:
        return lisaysIndeksi();
    case EraIdRooli:
        return ekavienti.eraId();
    case EuroRooli:
        return pankkivienti().debet() - pankkivienti().kredit();
    default:
        return QVariant();
    }

}

bool TilioteKirjausRivi::setRiviData(int sarake, const QVariant &value)
{
    if( riviData(sarake, Qt::EditRole) == value || viennit_.count() < 2)
        return false;

    switch (sarake) {
    case PVM:
        for(int i=0; i < viennit_.count(); i++)
            viennit_[i].setPvm(value.toDate());
        break;
    case SAAJAMAKSAJA:
        for(int i=0; i < viennit_.count(); i++)
            viennit_[i].setKumppani(value.toMap());
        break;
    case TILI: {
        viennit_[1].setTili(value.toInt());
        const Tili* tili = model()->kitsas()->tilit()->tili(value.toInt());
        if( tili && tili->luku("kohdennus"))
            viennit_[1].setKohdennus(tili->luku("kohdennus"));
        break;
    }
    case KOHDENNUS:
        for(int i=1; i < viennit_.count(); i++)
            viennit_[i].setKohdennus(value.toInt());
        break;
    case SELITE:
        for(int i=0; i < viennit_.count(); i++)
            viennit_[i].setSelite(value.toString());
        break;
    case EURO:
        viennit_[0].setKredit(0.0);
        viennit_[0].setDebet(value.toString());
        viennit_[1].setDebet(0.0);
        viennit_[1].setKredit(value.toString());
        break;
    }
    paivitaTyyppi();
    return true;
}

Qt::ItemFlags TilioteKirjausRivi::riviFlags(int sarake) const
{
    if( viennit_.value(1).eraId() && sarake != PVM && sarake != SAAJAMAKSAJA && sarake != SELITE )
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if( sarake == EURO && viennit_.count() != 2)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    const Tili* tili = model()->kitsas()->tilit()->tili(viennit_.value(1).tili());
    if( sarake == KOHDENNUS && ( (tili && tili->onko(TiliLaji::TASE)) || !model()->kitsas()->kohdennukset()->kohdennuksia()) )
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QList<TositeVienti> TilioteKirjausRivi::viennit() const
{
    return viennit_;
}

TositeVienti TilioteKirjausRivi::pankkivienti() const
{
    return viennit_.value(0);
}

QVariantList TilioteKirjausRivi::tallennettavat(int tyyppi) const
{
    QVariantList ulos;
    for(auto vienti : viennit_) {
        // Meno- ja tulotositteelle siirrettäessä varmistetaan oikea tyyppi        
        if( tyyppi )
            vienti.setTyyppi( vienti.tyyppi() % 100 + tyyppi );
        ulos << vienti;
    }
    return ulos;
}

void TilioteKirjausRivi::asetaPankkitili(int tili)
{
    if(!viennit_.isEmpty())
        viennit_[0].setTili(tili);
}

void TilioteKirjausRivi::asetaViennit(const QList<TositeVienti> &viennit)
{
    viennit_ = viennit;
    paivitaTyyppi();
    paivitaErikoisrivit();
}


void TilioteKirjausRivi::paivitaTyyppi()
{
    int tyyppi = 0;
    const Tili* tili = model()->kitsas()->tilit()->tili( viennit_.value(1).tili() );

    if( viennit_.value(1).eraId() > 0) {
        tyyppi = TositeVienti::SUORITUS;
    } else if( tili && tili->onko(TiliLaji::TASE) ) {
        tyyppi = TositeVienti::SIIRTO;
    } else if( tili && tili->onko(TiliLaji::TULO) ) {
        tyyppi = TositeVienti::MYYNTI;
    } else if( tili && tili->onko(TiliLaji::MENO)) {
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

void TilioteKirjausRivi::paivitaErikoisrivit()
{
    const TositeVienti& vienti = viennit_.value(1);
    const int eraId =  vienti.eraId();
    if( eraId > 0 && qAbs(vienti.kredit() - vienti.debet()) > 1e-5) {
        if( eraId == taydennys_.eraId() )
            sijoitaErikoisrivit();
        else
            model()->tilaaAlkuperaisTosite( lisaysIndeksi(), eraId );
    }
}

void TilioteKirjausRivi::sijoitaErikoisrivit()
{
    const QList<TositeVienti>& taydennys = taydennys_.paivita(viennit_);
    if(!taydennys.isEmpty()) {
        QList<TositeVienti> uudet;
        uudet << viennit_.value(0) << viennit_.value(1);
        uudet << taydennys;
        viennit_ = uudet;
    }
}

QString TilioteKirjausRivi::pseudoarkistotunnus() const
{
    const QString merkit("abcdefghijklmnopqrstuvwxyz0123456789");

    QString tunnus =QString("@%1-%2-")
            .arg(lisaysIndeksi(),4,10,QChar('0'))
            .arg(QDateTime::currentMSecsSinceEpoch(),12,16,QChar('0'));
    for(int i=0; i<16; ++i)
    {
       int index = QRandomGenerator::global()->bounded(merkit.length()-1);
       QChar nextChar = merkit.at(index);
       tunnus.append(nextChar);
    }
    return tunnus;
}

void TilioteKirjausRivi::alkuperaistositeSaapuu(QVariant *data, int eraId)
{
    const QVariantList lista = data->toMap().value("viennit").toList();
    taydennys_.asetaEra(eraId, lista);
    sijoitaErikoisrivit();
}

void TilioteKirjausRivi::asetaLisaysIndeksi(const int indeksi)
{
    TilioteRivi::asetaLisaysIndeksi(indeksi);
    if( !viennit_.isEmpty())
        viennit_[0].setArkistotunnus( pseudoarkistotunnus() );
}


