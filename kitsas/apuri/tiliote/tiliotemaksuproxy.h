/*
   Copyright (C) 2026 Kitsas Oy

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
#ifndef TILIOTEMAKSUPROXY_H
#define TILIOTEMAKSUPROXY_H

#include <QSortFilterProxyModel>

/**
 * @brief Avointen laskujen suodatus tiliotekirjauksessa
 *
 * Numerolla tai viitteellä (myös RF-viite) haettaessa rivi hyväksytään,
 * jos haettava on laskun numeron tai viitenumeron alkuosa. Muuten
 * suodatetaan asiakkaan tai toimittajan nimellä.
 */
class TilioteMaksuProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit TilioteMaksuProxy(QObject* parent = nullptr);

    void suodata(const QString& teksti);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    QString numeroHaku_;
};

#endif // TILIOTEMAKSUPROXY_H
