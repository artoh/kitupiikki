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
#include "tuotetuontimodel.h"
#include "db/kirjanpito.h"

TuoteTuontiModel::TuoteTuontiModel(QObject *parent)
    : RekisteriTuontiModel(parent)
{
}


QString TuoteTuontiModel::otsikkoTeksti(int sarake)
{
    switch (sarake) {
        case NIMIKE: return tr("Nimike");
        case YKSIKKO: return tr("Yksikkö");
        case NETTOHINTA: return tr("Hinta á netto");
        case KOHDENNUS: return tr("Kohdennus");
        case TILI: return tr("Tili");
        case ALVKOODI: return tr("Alv-koodi");
        case ALVPROSENTTI: return tr("Alv-prosentti");
        case BRUTTOHINTA: return tr("Bruttohinta");
        default: return QString();    
    }    
}

QVariantList TuoteTuontiModel::lista() const
{
    QVariantList lista;

    bool alvvelvollisuus = kp()->asetukset()->onko(AsetusModel::AlvVelvollinen);
    
    for(int r = otsikkorivi_ ? 1 : 0 ; r < csv_.count(); r++) {
        QVariantMap map;
        QStringList rivi = csv_.value(r);
        
        double alvprosentti =  alvvelvollisuus ? 24.0 : 0;
        double alvkoodi = alvvelvollisuus ? AlvKoodi::MYYNNIT_NETTO : AlvKoodi::EIALV;
        int tili = 3000;
        double nettohinta = 0;
        double bruttohinta = 0;
        
        for(int i = 0; i < rivi.count(); i++) {
            QString txt = rivi.value(i);
            int tyyppi = sarakkeet_.value(i);
            
            switch (tyyppi) {
            case NIMIKE:
                map.insert("nimike", txt); break;
            case YKSIKKO:
                map.insert("yksikko", txt); break;
            case NETTOHINTA:
                nettohinta = txt.toDouble(); break;
            case KOHDENNUS:
                map.insert("kohdennus", txt.toInt()); break;
            case TILI:
                tili = txt.toInt(); break;
            case ALVKOODI:
                alvkoodi = txt.toInt(); break;
            case ALVPROSENTTI:
                alvprosentti = txt.toInt(); break;
            case BRUTTOHINTA:
                bruttohinta = txt.toDouble();
            }
        }
        if(nettohinta == 0.0) {
            if(alvkoodi == AlvKoodi::MYYNNIT_NETTO) {
                nettohinta = bruttohinta / 1.00 + (alvprosentti / 100.0);
            } else {
                nettohinta = bruttohinta;
            }
        }

        map.insert("ahinta", nettohinta);
        map.insert("tili", tili);
        map.insert("alvkoodi", alvkoodi);
        map.insert("alvprosentti", alvprosentti);
        lista.append(map);
    }
    return lista;
}

QString TuoteTuontiModel::otsikkoTekstini(int sarake) const
{
    return TuoteTuontiModel::otsikkoTeksti(sarake);
}

void TuoteTuontiModel::arvaaSarakkeet()
{
    bool otsikkoja = false;
    QStringList rivi = csv_.value(0);
    
    for(int i=0; i < rivi.count(); i++) {
        QString txt = rivi.value(i);
        if(txt.contains("nimike", Qt::CaseInsensitive)) {
            sarakkeet_[i] = NIMIKE;
            otsikkoja = true;
        } else if(txt.contains("yksikk", Qt::CaseInsensitive)) {
            sarakkeet_[i] = YKSIKKO;
            otsikkoja = true;
        } else if(txt.contains("netto", Qt::CaseInsensitive)) {
            sarakkeet_[i] = NETTOHINTA;
            otsikkoja = true;
        } else if(txt.contains("brutto", Qt::CaseInsensitive) || txt.contains("hinta"), Qt::CaseInsensitive) {
            sarakkeet_[i] = BRUTTOHINTA;
            otsikkoja = true;
        } else if(txt.contains("tili", Qt::CaseInsensitive)) {
            sarakkeet_[i] = TILI;
            otsikkoja = true;
        } else if(txt.contains("kohdennus", Qt::CaseInsensitive)) {
            sarakkeet_[i] = KOHDENNUS;
            otsikkoja = true;
        } else if(txt.contains("alvkoodi", Qt::CaseInsensitive)) {
            sarakkeet_[i] = ALVKOODI;
            otsikkoja = true;
        } else if(txt.contains("alvprosentti", Qt::CaseInsensitive)) {
            sarakkeet_[i] = ALVPROSENTTI;
            otsikkoja = true;
        }
    }
    
    if(otsikkoja) {
        asetaOtsikkorivi(true);
        return;
    }
    
    // Ei haistella aineiston perusteella 
}
