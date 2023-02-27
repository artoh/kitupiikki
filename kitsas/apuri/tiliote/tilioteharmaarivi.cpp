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
#include "tilioteharmaarivi.h"
#include "db/kirjanpito.h"
#include "tiliotemodel.h"

#include "db/kitsasinterface.h"

#include "db/tilimodel.h"
#include "db/tilikausimodel.h"
#include "db/tositetyyppimodel.h"
#include "db/verotyyppimodel.h"

#include <QColor>
#include <QBrush>
#include <QPalette>

TilioteHarmaaRivi::TilioteHarmaaRivi()
{

}

TilioteHarmaaRivi::TilioteHarmaaRivi(const QVariantMap &data, TilioteModel *model) :
    TilioteRivi(model), vienti_(data)
{

}

QVariant TilioteHarmaaRivi::riviData(int sarake, int role, bool alternateColor ) const
{
    switch (role) {
    case Qt::DisplayRole:    
    case LajitteluRooli:
        switch (sarake) {
        case PVM:
            if(role == LajitteluRooli) {
                return QString("%1 %2").arg(vienti_.pvm().toString(Qt::ISODate))
                                        .arg(lisaysIndeksi(),6,10,QChar('0'));
            } else
                return vienti_.pvm();
        case SAAJAMAKSAJA:
            return vienti_.kumppaniNimi();
        case SELITE:
            return vienti_.selite();
        case TILI:
        {
            QVariantList list = vienti_.value("vastatilit").toList();
            QList<int> suodatettu;

            for(const auto& item: list) {
                int numero = item.toInt();
                if(suodatettu.contains(numero)) continue;
                Tili tili = model()->kitsas()->tilit()->tiliNumerolla(numero);
                if( tili.onko(TiliLaji::ALVVELKA) || tili.onko(TiliLaji::ALVSAATAVA)) continue;
                suodatettu.append(numero);
            }

            if(suodatettu.count() == 1) {
                return model()->kitsas()->tilit()->tiliNumerolla(suodatettu.value(0)).nimiNumero();
            } else {
                QStringList strlist;
                for(auto const &item : qAsConst(suodatettu))
                    strlist << QString::number(item);
                return strlist.join(", ");
            }
        }
        case ALV: {
            QVariantList list = vienti_.value("alv").toList();            
            if( list.count() == 1) {
                const QVariantMap& map = list.at(0).toMap();
                if( map.value("koodi").toInt() == AlvKoodi::EIALV) return QString();
                int prossa = (int) map.value("prosentti").toString().toDouble();
                if(prossa) return QString("%1 %").arg(prossa);
            }
            else if(list.count() > 1) {
                QStringList prossat;
                for(const auto& item : list) {
                    const QVariantMap map = item.toMap();
                    const QString txt = QString("%1 %").arg((int) map.value("prosentti").toString().toDouble());
                    if(!prossat.contains(txt)) prossat.append(txt);
                }
                prossat.sort();
                return prossat.join(", ");
            }
            return QVariant();
        }
        case KOHDENNUS: {
            QVariantMap tosite = vienti_.value("tosite").toMap();
            return model()->kitsas()->tositeTunnus(tosite.value("tunniste").toInt(),
                                                           tosite.value("pvm").toDate(),
                                                           tosite.value("sarja").toString());
        }
        case EURO: {
            double euro = vienti_.contains("debet") ? vienti_.debet() : 0 - vienti_.kredit();
            return qAbs(euro) > 1e-5 ? QString("%L1 €").arg( euro ,0,'f',2) : QVariant();
        }
        default: return QVariant();
    }
    case Qt::TextAlignmentRole:
        return sarake == EURO || sarake == ALV ? QVariant(Qt::AlignRight | Qt::AlignVCenter) : QVariant(Qt::AlignLeft | Qt::AlignVCenter);
/*    case Qt::ForegroundRole:
        if( QPalette().base().color().lightness() > 128)
            return QBrush(QColor(0,77,0));
        else
            return QBrush(QColor(200,255,200));
*/
    case Qt::BackgroundRole:
        if( QPalette().base().color().lightness() > 128) {
            return alternateColor ? QBrush(QColor(173, 255, 153)) : QBrush(QColor(194,255,179));
        } else {
            return alternateColor ? QBrush(QColor(26, 102, 0)) : QBrush(QColor(38,153,0));
        }
    case TilaRooli:
        return "AH";
    case TositeIdRooli:
        return vienti_.value("tosite").toMap().value("id").toInt();
    case LisaysIndeksiRooli:
        return lisaysIndeksi();
    case Qt::DecorationRole:        
        if( sarake == KOHDENNUS) {
            const QVariantMap& tosite = vienti_.value("tosite").toMap();
            return model()->kitsas()->tositeTyypit()->kuvake( tosite.value("tyyppi").toInt() );
        } else if(sarake == ALV) {

            QVariantList list = vienti_.value("alv").toList();
            if( list.count() == 1) {
                int koodi = list.at(0).toMap().value("koodi").toInt();
                return model()->kitsas()->alvTyypit()->kuvakeKoodilla(koodi);
            }
            return QVariant();
        } else if( sarake == TILI) {
            QVariantList list = vienti_.value("vastatilit").toList();
            for(const auto& item: list) {
                Tili tili = model()->kitsas()->tilit()->tiliNumerolla(item.toInt());
                if( tili.nimi("fi").contains("selvittelytili", Qt::CaseInsensitive)) {
                    return QIcon(":/pic/varoitus.png");
                }
            }
        } else {
            return QVariant();
        }
    }



    return QVariant();
}

TositeVienti TilioteHarmaaRivi::vienti() const
{
    return vienti_;
}
