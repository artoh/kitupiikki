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

#ifndef EHDOTUSMODEL_H
#define EHDOTUSMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include "db/vientimodel.h"


/**
 * @brief Kirjausapurin kirjausehdotukset
 *
 * KirjausApuriDialogi muodostaa kirjausehdotukset VientiRivi-tietueilla ja säilöö ne EhdotusModeliin.
 * Kun dialogi vahvistetaan, tallenna() tallentaa apurin muodostamat viennit VientiModel:iin
 *
 */
class EhdotusModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    enum EhdotusSarake
    {
        TILI, DEBET, KREDIT
    };

    EhdotusModel();

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    void tyhjaa();
    void lisaaVienti(VientiRivi rivi);
    /**
     * @brief Tallentaa ehdotetun viennin
     * @param model VientiModel, minne tallennetaan
     */
     void tallenna(VientiModel *model, int yhdistettavaVastatiliNumero = 0, QDate yhdistettavaPvm = QDate());

    /**
     * @brief Onko kirjausehdotus valmis
     *
     * Kirjauksen voi tehdä, jos debet ja kredit täsmää ja summa > 0
     * Toispuolinen kelpaa, vaikkei täsmää
     *
     * @return tosi, jos ehdotus on valmis
     */
    bool onkoKelpo(bool toispuolinen = false) const;

private:
    QList<VientiRivi> viennit_;

};

#endif // EHDOTUSMODEL_H
