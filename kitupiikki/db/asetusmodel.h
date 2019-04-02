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

#ifndef ASETUSMODEL_H
#define ASETUSMODEL_H

#include <QObject>
#include <QHash>
#include <QSqlDatabase>
#include <QDateTime>

/**
 * @brief Asetusten käsittely
 *
 * Asetukset tallennetaan välittömästi tietokantaan
 * Yleisimmille tietotyypeille on valmiiksi muunnosfunktiot
 *
 * Tyhjä arvo (tai nolla) tarkoittaa, että kyseinen asetus poistetaan
 *
 */
class AsetusModel : public QObject
{
    Q_OBJECT
public:
    explicit AsetusModel(QSqlDatabase *tietokanta, QObject *parent = 0, bool uusikirjanpito=false);

    /**
     * @brief Palauttaa asetuksen annetulla avaimella
     * @param avain Haettava avain
     * @return asetus, tai String() jos asetusta ei ole
     */
    QString asetus(const QString& avain, const QString oletus = QString()) const { return asetukset_.value(avain, oletus); }
    void aseta(const QString& avain, const QString& arvo);
    /**
     * @brief Poistaa asetuksen annetulla avaimella
     * @param avain
     */
    void poista(const QString& avain);

    QDate pvm(const QString& avain, const QDate oletus = QDate()) const;
    void aseta(const QString &avain, const QDate& pvm);

    bool onko(const QString& avain) const;
    void aseta(const QString& avain, bool totuusarvo);

    void asetaVar(const QString& avain, const QVariant& arvo);

    QStringList lista(const QString& avain) const;
    void aseta(const QString& avain, const QStringList& arvo);

    int luku(const QString& avain, int oletusarvo = 0) const;
    qulonglong isoluku(const QString &avain, qulonglong oletusarvo = 0) const;
    void aseta(const QString &avain, int luku);
    void aseta(const QString& avain, qulonglong luku);

    /**
     * @brief Palauttaa listan avaimista, jotka alkavat annetulla alulla
     * @param avaimenAlku Teksti, jolla haettavat avaimet alkavat, tyhjä hakee kaikki
     * @return
     */
    QStringList avaimet(const QString& avaimenAlku = QString()) const;

    /**
     * @brief Koska tätä asetusta on muokattu
     * @param avain
     * @return null, jos peräisin ktp-tiedostosta
     */
    QDateTime muokattu(const QString& avain) const;

    /**
     * @brief Siirtää modelin moodiin, jossa muutospäiväykseksi merkitään null
     * @param onko
     */
    void tilikarttaMoodiin(bool onko);

    void lataa();

    void lataa(const QVariantList& lista);

    void tyhjenna() { asetukset_.clear(); }

signals:



protected:
    QHash<QString,QString> asetukset_;
    QHash<QString,QDateTime> muokatut_;

    QSqlDatabase *tietokanta_;

    bool alustetaanTietokantaa_;    /** tosi, jos tietokantaa vasta luodaan */

};

#endif // ASETUSMODEL_H
