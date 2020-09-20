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
#ifndef TILIVALINTADIALOGIFILTTERI_H
#define TILIVALINTADIALOGIFILTTERI_H

#include <QSortFilterProxyModel>
#include <QObject>
#include <QSet>

class TilivalintaDialogiFiltteri : public QSortFilterProxyModel
{
    Q_OBJECT
public:

    enum Suodatus {
        KAIKKI = 0,
        KAYTOSSA = 1,
        SUOSIKIT = 2,
        KIRJATTU = 3
    };

    TilivalintaDialogiFiltteri(QObject* parent = nullptr);

    void suodataTyypilla(const QString& tilityyppi);
    void suodataTekstilla(const QString& teksti);
    void suodataTilalla(int tila);
    void naytaOtsikot(bool nayta);

    int suodatusTila() const { return tilaSuodatus_; }

protected:
    void doFiltering();

    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    QString nimiSuodatus_;
    QString numeroSuodatus_;
    QRegularExpression tyyppiSuodatus_;

    int tilaSuodatus_ = KAYTOSSA;
    bool otsikot_ = true;

    QSet<int> suodatusSet_;


};

#endif // TILIVALINTADIALOGIFILTTERI_H
