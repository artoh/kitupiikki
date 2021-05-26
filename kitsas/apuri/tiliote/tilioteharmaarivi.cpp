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
#include "tiliotemodel.h"

#include "db/kitsasinterface.h"

#include "db/tilimodel.h"
#include "db/tilikausimodel.h"
#include "db/tositetyyppimodel.h"

#include <QColor>

TilioteHarmaaRivi::TilioteHarmaaRivi()
{

}

TilioteHarmaaRivi::TilioteHarmaaRivi(const QVariantMap &data, TilioteModel *model) :
    TilioteRivi(model), vienti_(data)
{

}

QVariant TilioteHarmaaRivi::riviData(int sarake, int role) const
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
            if(list.count() == 1) {
                return model()->kitsas()->tilit()->tiliNumerolla(list.value(0).toInt()).nimiNumero();
            } else {
                QStringList strlist;
                for(auto const &item : qAsConst(list))
                    strlist << item.toString();
                return strlist.join(", ");
            }
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
        return sarake == EURO ? QVariant(Qt::AlignRight | Qt::AlignVCenter) : QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    case Qt::TextColorRole:
        return QColor(Qt::darkGreen);
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
