/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef TILITYYPPIMODEL_H
#define TILITYYPPIMODEL_H
#include <QAbstractListModel>
#include <QList>

namespace TiliLaji {
    enum TiliLuonne
    {
        EITILIA         =   0,
        TASE            =   0b000000000001,
        VASTAAVA        =   0b000000000101,
        POISTETTAVA     =   0b000001010101,
        TASAERAPOISTO    =  0b000011010101,
        MENOJAANNOSPOISTO=  0b000101010101,
        SAATAVA        =   0b000001100101,
        ALVSAATAVA     =   0b001011100101,
        RAHAVARAT       =   0b010011100101,
        VASTATTAVA      =   0b000000001001,
        OMAPAAOMA       =   0b000000011001,
        EDELLISTENTULOS =   0b000100011001,
        VIERASPAAOMA    =   0b000000101001,
        VELKA           =   0b000001101001,
        ALVVELKA        =   0b001001101001,
        VEROVELKA       =   0b010001101001,

        TULOS           =   0b000000000010,
        TULO            =   0b000000001010,
        MENO            =   0b000000010010,
        POISTO          =   0b000001010010
    };

}

/**
 * @brief Tilityypin kuvaus
 */
class TiliTyyppi
{
public:
    TiliTyyppi(QString tyyppikoodi=QString(), QString kuvaus=QString(), TiliLaji::TiliLuonne luonne=TiliLaji::EITILIA);

    QString koodi() const { return tyyppikoodi_; }
    QString kuvaus() const { return kuvaus_; }
    TiliLaji::TiliLuonne luonne() const { return luonne_; }
    /**
     * @brief Edustaako tämä tili kysyttyä "luonnetta"
     * @param luonnetta Luonne
     *
     * Esimerkiksi ALV-saatavien tili edustaa edustaa myös luonteita
     * SAAMINEN, VASTAAVA ja TASE
     *
     * @return
     */
    bool onko(TiliLaji::TiliLuonne luonnetta) const;

protected:
    QString tyyppikoodi_;
    QString kuvaus_;
    TiliLaji::TiliLuonne     luonne_;
};

/**
 * @brief Valittavissa olevien tilityyppien model
 */
class TilityyppiModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum
    {
        KoodiRooli = Qt::UserRole,
        KuvausRooli = Qt::UserRole + 2,
        LuonneRooli = Qt::UserRole + 3
    };

    TilityyppiModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    TiliTyyppi tyyppiKoodilla(QString koodi);
protected:
    void lisaa(TiliTyyppi tyyppi);
    QList<TiliTyyppi> tyypit;
};

#endif // TILITYYPPIMODEL_H
