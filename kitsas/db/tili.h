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

#ifndef TILI_H
#define TILI_H

#include <QString>
#include <QDate>

#include "tilityyppimodel.h"

#include "kantavariantti.h"
#include "kieli/monikielinen.h"


/**
 * @brief Tilin tai otsikon tiedot
 *
 */
class Tili : public KantaVariantti
{
public:
    Tili();

    Tili(const QVariantMap& data);

    int id() const;
    int numero() const { return numero_; }

    QString nimi(const QString& kieli = QString()) const;
    QString nimiNumero(const QString& kieli = QString()) const;
    QString nimiKaannos(const QString& kieli) const { return nimi_.kaannos(kieli);}

    QString ohje(const QString& kieli=QString()) const;
    QString ohjeKaannos(const QString& kieli) const { return ohje_.kaannos(kieli);}

    TiliTyyppi tyyppi() const { return tyyppi_;}
    QString tyyppiKoodi() const { return tyyppi().koodi(); }

    int tila() const { return tila_; }
    int otsikkotaso() const { return tyyppi().otsikkotaso(); }

    int laajuus() const { return laajuus_;}

    /**
     * @brief Palauttaa tämän tilin tai otsikon yllä olevan otsikon id:n
     * @return
     */
    int ylaotsikkoId() const { return ylaotsikkoId_; }


    void asetaNumero(int numero);
    void asetaNimi(const QString& nimi, const QString& kieli) { nimi_.aseta(nimi, kieli);}
    void asetaTyyppi(const QString& tyyppikoodi);
    void asetaTila(int tila) { tila_ = tila; }
    void asetaOhje(const QString& ohje, const QString& kieli) { ohje_.aseta(ohje, kieli);}
    void asetaLaajuus(int laajuus) { laajuus_ = laajuus;}

    Tili* tamanOtsikko() { return tamanOtsikko_; }
    void asetaOtsikko(Tili* otsikko);

    /**
     * @brief Onko tilillä tarvittavat tiedot, että voi tallettaa
     * @return
     */
    bool onkoValidi() const;

    /**
     * @brief Onko tili kysyttyä tyyppiä
     * @param luonne Tilin luonne
     * @return
     *
     * Luonne voi olla varsinainen tilityyppi tai sen yleistys, esimerkiksi
     * TiliLaji::Tulos kuvaa kaikkia tulo- ja menotilejä.
     *
     */
    bool onko(TiliLaji::TiliLuonne luonne) const;


    enum TaseErittelyTapa
    {
        TASEERITTELY_SALDOT = 0,
        TASEERITTELY_MUUTOKSET = 1,
        TASEERITTELY_LISTA = 2,
        TASEERITTELY_TAYSI = 3
    };

    enum TiliTila
    {
        TILI_PIILOSSA = 0,
        TILI_KAYTOSSA = 1,
        TILI_SUOSIKKI = 2,
        TILI_NORMAALI = 3
    };

    /**
     * @brief Millä tasolla tase-erittely laaditaan
     * @return TaseErittelyTapa
     */
    int taseErittelyTapa();
    /**
     * @brief Pidetäänkö tase-eristä kirjaa
     *
     * Jos tase-erittely laaditaan täydellisenä tai listana, tarvitaan tase-erien kirjaus
     *
     * @return
     */
    bool eritellaankoTase()  { return taseErittelyTapa() == TASEERITTELY_TAYSI ||
                                  taseErittelyTapa() == TASEERITTELY_LISTA;  }

    QVariantMap data() const;


protected:
    int numero_;

    TiliTyyppi tyyppi_;
    int tila_;
    int ylaotsikkoId_;

    Monikielinen nimi_;
    Monikielinen ohje_;
    int laajuus_ = 0;

    Tili* tamanOtsikko_ = nullptr;

};

#endif // TILI_H
