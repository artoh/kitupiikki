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
#ifndef TULOSTUSRUUDUKKO_H
#define TULOSTUSRUUDUKKO_H

#include <QString>
#include <QList>
#include <QSizeF>

class QPainter;
class QPagedPaintDevice;

class SivunVaihtaja
{
public:
    virtual qreal vaihdaSivua(QPainter* painter, QPagedPaintDevice* device) = 0;
};


class TulostusRuudukko
{
public:
    TulostusRuudukko();

    void asetaLeveys(qreal vahintaan, qreal enintaan = -1) { vahimmaisLeveys_ = vahintaan; enimmaisLeveys_ = enintaan;}
    void asetaPistekoko(int pistekoko) { pistekoko_ = pistekoko;}

    void lisaaSarake(const QString& otsikko,
           Qt::AlignmentFlag tasaus = Qt::AlignLeft,
           qreal vahimmaisleveys = 0.0);

    void lisaaRivi(const QStringList& tekstit, bool lihava = false);

    void lisaaSummaRivi(const QString& otsikko, const QString& summa);

    void laske(QPainter* painter);

    qreal piirra(QPainter* painter, QPagedPaintDevice* device,
                qreal alaMarginaali = -1, SivunVaihtaja* vaihtaja = nullptr);        

    QSizeF koko() const { return koko_;}
    QSizeF summaKoko() const { return summakoko_;}

protected:

    class Sarake {
    public:
        Sarake();
        Sarake(const QString& otsikko,
               Qt::AlignmentFlag tasaus = Qt::AlignLeft,
               qreal vahimmaisleveys = 0.0);

        QString otsikko() const { return otsikko_; }
        qreal leveys() const { return leveys_; }
        Qt::AlignmentFlag tasaus() const { return tasaus_;}

        void asetaLeveys(const qreal leveys) { leveys_= leveys;}

    private:
        QString otsikko_;
        qreal leveys_ = 0;
        Qt::AlignmentFlag tasaus_ = Qt::AlignLeft;
    };

    class Rivi {
    public:
        Rivi();
        Rivi(const QStringList& tekstit, bool lihava = false);

        QString teksti(int indeksi) const { return tekstit_.value(indeksi);}
        bool lihava() const { return lihava_;}
        void asetaKorkeus(qreal korkeus) { korkeus_ = korkeus; }
        qreal korkeus() const { return korkeus_;}

    private:
        qreal korkeus_ = 0.0;
        QStringList tekstit_;
        bool lihava_ = false;
    };

protected:
    void piirraOtsikko(QPainter* painter);
    void piirraRivi(const Rivi& rivi, QPainter* painter);
    void piiraSummaRivit(QPainter* painter);


private:
    QList<Sarake> sarakkeet_;
    QList<Rivi> rivit_;
    QList<QPair<QString,QString>> summarivit_;

    qreal vahimmaisLeveys_ = -1;
    qreal enimmaisLeveys_ = -1;

    int pistekoko_ = 9;

    qreal sarakevali_;
    qreal ivali_;

    QSizeF summakoko_;
    QSizeF koko_;        

};

#endif // TULOSTUSRUUDUKKO_H
