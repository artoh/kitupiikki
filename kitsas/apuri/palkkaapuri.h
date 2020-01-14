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
#ifndef PALKKAAVUSTAJA_H
#define PALKKAAVUSTAJA_H

#include "apuriwidget.h"

#include <QVariantMap>

namespace Ui {
class PalkkaApuri;
}

class PalkkaApuri : public ApuriWidget
{
    Q_OBJECT

public:
    explicit PalkkaApuri(QWidget *parent = nullptr, Tosite* tosite = nullptr);
    virtual ~PalkkaApuri() override;

    void otaFokus() override;

protected:
    void teeReset() override;
    bool teeTositteelle() override;


    void kirjaa(QVariantList& lista, const QString& palkkakoodi,
                double debet = 0.0, double kredit=0.0, const QString& selite = QString(), const QString& tallennuskoodi = QString());

private:
    Ui::PalkkaApuri *ui;
    QVariantMap palkkatilit_;
};

#endif // PALKKAAVUSTAJA_H
