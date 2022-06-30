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
#ifndef LASKUNALAOSA_H
#define LASKUNALAOSA_H

#include "model/lasku.h"

#include "laskumaksulaatikko.h"
#include "laskuinfolaatikko.h"

#include "model/toiminimimodel.h"

class KitsasInterface;
class QPainter;



class LaskunAlaosa
{
public:
    LaskunAlaosa(KitsasInterface* interface);

    void lataa(const Lasku& lasku, const QString& vastaanottaja);

    qreal laske(QPainter* painter);
    void piirra(QPainter* painter, const Lasku &lasku);

    qreal alatunniste(QPainter* painter);

    static QString code128(const QString& koodattava);
private:
    void lataaIbanit();
    QString kaanna(const QString& avain) const;
    void lataaYhteystiedot();
    void lataaMaksutiedot(const Lasku& lasku);

    void lisaaYhteys(const int &asetusavain, const QString& kaannosavain);
    QString toimiNimiTieto(ToiminimiModel::ToiminimiRoolit rooli) const;
    void toiminimiYhteys(ToiminimiModel::ToiminimiRoolit rooli, const QString& kaannosavain);
    void lisaaTunnukseen(const QString &avain, const QString& teksti);
    void piirraTilisiirto(QPainter* painter, const Lasku& lasku);
    void piirraViivakoodi(QPainter* painter, const QRectF& rect, const Lasku& lasku);

    static QChar code128c(int koodattava);

    LaskuMaksuLaatikko maksulaatikko_;
    LaskuInfoLaatikko osoitelaatikko_;
    LaskuInfoLaatikko yhteyslaatikko_;
    LaskuInfoLaatikko tunnuslaatikko_;

    KitsasInterface *interface_;
    QString kieli_;

    QStringList pankit_;
    QStringList ibanit_;
    QStringList bicit_;
    QString lahettaja_;
    QString vastaanottaja_;

    qreal alaosaKorkeus_ = 0.0;
    qreal maksuKorkeus_ = 0.0;
    qreal yhteysKorkeus_ = 0.0;

    bool tilisiirto_;
    bool viivakoodi_;
    bool virtuaaliviivakoodi_;

    int toiminimiIndeksi_ = 0;
};

#endif // LASKUNALAOSA_H
