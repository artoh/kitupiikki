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
#ifndef HUONEISTO_H
#define HUONEISTO_H

#include <QObject>

#include "huoneistolaskutusmodel.h"

class Huoneisto : public QObject
{
    Q_OBJECT
public:
    explicit Huoneisto(QObject *parent = nullptr);

    void lataa(int id);
    void tallenna();

    int id() const;

    int asiakas() const;
    void setAsiakas(int asiakas);

    QString nimi() const;
    void setNimi(const QString &nimi);

    QString muistiinpanot() const;
    void setMuistiinpanot(const QString &muistiinpanot);

    HuoneistoLaskutusModel* laskutus();

protected:
    void lataaData(QVariant* data);
    QVariantMap toMap() const;
    void tallennusValmis();

signals:
    void ladattu();
    void tallennettu();


private:
    int id_ = 0;
    int asiakas_ = 0;
    QString nimi_;
    QString muistiinpanot_;
    HuoneistoLaskutusModel laskutus_;

};

#endif // HUONEISTO_H
