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
#ifndef ILMOITINTUOTTAJA_H
#define ILMOITINTUOTTAJA_H

#include <QObject>

class IlmoitinTuottaja : public QObject
{
    Q_OBJECT
public:
    explicit IlmoitinTuottaja(QObject *parent = nullptr);

    void tallennaAineisto(int ilmoitusId);
    bool voikoMuodostaa(const QVariantMap& map);

protected:
    /**
     * @brief Verokauden tiedot
     * @param map Alv-tallenne
     * @return Jos kelvollinen, niin lista (kausikirjain, vuosi, [kausi])
     */
    QVariantList kausiTieto(const QVariantMap& map);


    void tositeSaapuu(QVariant* data);

    bool muodosta(const QVariantMap& data);

    void lisaa(int koodi, const QString& arvo);
    void lisaa(int koodi, qlonglong sentit);

    QString txt_;
signals:

};

#endif // ILMOITINTUOTTAJA_H
