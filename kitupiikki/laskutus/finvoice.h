/*
   Copyright (C) 2018 Arto Hyvättinen

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
#ifndef FINVOICE_H
#define FINVOICE_H

#include <QObject>

class LaskuModel;


struct HajoitettuOsoite
{
    QString lahiosoite;
    QString postinumero;
    QString postitoimipaikka;
    QString maakoodi;
};

/**
 * @brief Finvoice-laskun käsittely
 */
class Finvoice : public QObject
{
    Q_OBJECT
public:
    explicit Finvoice(QObject *parent = nullptr);

    static bool muodostaFinvoice(LaskuModel *model);
    static QByteArray lasku(LaskuModel* model);

    static HajoitettuOsoite hajoitaOsoite(const QString& osoite);

signals:

public slots:
};

#endif // FINVOICE_H
