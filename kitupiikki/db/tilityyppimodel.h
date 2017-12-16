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
        TUNTEMATON         =   0,
        OTSIKKO         =   0b0000000000001,
        TASE            =   0b0000000000010,
        VASTAAVAA        =  0b0000000001010,
        POISTETTAVA     =   0b0000010101010,
        TASAERAPOISTO    =  0b0000110101010,
        MENOJAANNOSPOISTO=  0b0001010101010,
        SAATAVA        =    0b0000011001010,
        ALVSAATAVA     =    0b0010111001010,
        RAHAVARAT       =   0b0100111001010,
        VASTATTAVAA      =  0b0000000010010,
        OMAPAAOMA       =   0b0000000110010,
        EDELLISTENTULOS =   0b0001000110010,
        KAUDENTULOS     =   0b0010000110010,
        VIERASPAAOMA    =   0b0000001010010,
        VELKA           =   0b0000011010010,
        ALVVELKA        =   0b0010011010010,
        VEROVELKA       =   0b0100011010010,

        TULOS           =   0b0000000000100,
        TULO            =   0b0000000010100,
        LVTULO          =   0b0000001010100,
        MENO            =   0b0000000100100,
        POISTO          =   0b0000010100100
    };

}

/**
 * @brief Tilityypin kuvaus
 */
class TiliTyyppi
{
public:
    TiliTyyppi(int otsikkotaso);
    TiliTyyppi(QString tyyppikoodi=QString(), QString kuvaus=QString(), TiliLaji::TiliLuonne luonne=TiliLaji::TUNTEMATON);

    QString koodi() const { return tyyppikoodi_; }
    QString kuvaus() const { return kuvaus_; }
    TiliLaji::TiliLuonne luonne() const { return luonne_; }
    int otsikkotaso() const { return otsikkotaso_; }
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
    int otsikkotaso_ = 0;
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
