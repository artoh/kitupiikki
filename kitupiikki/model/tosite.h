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
#ifndef TOSITE_H
#define TOSITE_H


#include <QObject>
#include <QVariant>
#include <map>

class TositeViennit;
class TositeLiitteet;
class TositeLoki;


/**
 * @brief Kirjanpitotosite
 *
 * @author Arto Hyvättinen
 * @since 2.0
 */
class Tosite : public QObject
{
    Q_OBJECT
public:
    enum Avain {
        ID,
        PVM,
        TYYPPI,
        TILA,
        TUNNISTE,
        OTSIKKO,
        VIITE,
        ERAPVM,
        TOIMITTAJA,
        ASIAKAS,
        KOMMENTIT
    };

    enum Virheet {
        PVMPUUTTUU      = 0b1,
        PVMLUKITTU      = 0b10,
        PVMALV          = 0b100,
        EITASMAA        = 0b1000,
        NOLLA           = 0b10000,
        TILIPUUTTUU     = 0b100000
    };

    explicit Tosite(QObject *parent = nullptr);

    QVariant data(int kentta) const;
    void setData(int kentta, QVariant arvo);

    TositeViennit* viennit() { return viennit_; }
    TositeLiitteet* liitteet() { return liitteet_;}
    TositeLoki* loki() { return loki_;}

signals:
    void ladattu();
    void talletettu(int id, int tunniste, const QDate& pvm);
    void tallennusvirhe(int virhe);
    void tila(bool muokattu, int virheet, double debet, double kredit);
    void pvmMuuttui(const QDate& pvm);

public slots:
    void lataa(int tositeid);
    void lataaData(QVariant *variant);
    void tallenna();
    void tarkasta();
    void nollaa(const QDate& pvm, int tyyppi);

protected slots:
    void tallennusValmis(QVariant *variant);
    void tallennuksessaVirhe(int virhe);
    void liitteetTallennettu();

private:
    /**
     * @brief Tiedot tallennettavassa muodossa
     * @return
     */
    QVariantMap tallennettava();

private:
    QVariantMap data_;
    QVariantMap tallennettu_;

    TositeViennit* viennit_;
    TositeLiitteet* liitteet_;
    TositeLoki* loki_;

    bool resetointiKaynnissa_ = false;

    static std::map<int,QString> avaimet__;
};

#endif // TOSITE_H
