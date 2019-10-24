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

#ifndef JSONKENTTA_H
#define JSONKENTTA_H

#include <QDate>
#include <QMap>
#include <QVariant>

/**
 * @brief json-muotoisten kenttien käsittely
 *
 * Laajennettavuutta ja yksinkertaisempaa tietokantaa silmällä pitäen käytetään
 * json-muotoisia kenttiä, joita käsitellään tämän luokan kautta
 *
 */
class [[deprecated]] JsonKentta
{
public:
    JsonKentta();
    JsonKentta(const QByteArray &json);

    void set(const QString& avain, const QString& arvo);
    void set(const QString& avain, const QDate& pvm);
    void set(const QString& avain, int arvo=1);
    void set(const QString& avain, qulonglong arvo );
    void set(const QString& avain, qlonglong arvo);
    void unset(const QString &avain);
    void setVar(const QString& avain, const QVariant& arvo);

    QString str(const QString& avain);
    QDate date(const QString& avain);
    int luku(const QString& avain, int oletus = 0);
    qlonglong pitkaluku(QString& avain);
    qulonglong isoluku(const QString& avain);
    QVariant variant(const QString& avain);
    QStringList avaimet() const { return map_.keys(); }

    QByteArray toJson();
    QVariant toSqlJson();
    void fromJson(const QByteArray& json);

    /**
     * @brief Onko muokattu
     *
     * Muokkaus nollautuu, kun tiedot haetaan fromJson -funktiolla tai
     * viedään toSqlJson -funktiolla
     *
     * @return tosi, jos muokattu
     */
    bool onkoMuokattu() const { return muokattu_; }

protected:
    QVariantMap map_;
    bool muokattu_;
};

#endif // JSONKENTTA_H
