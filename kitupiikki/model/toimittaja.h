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
#ifndef TOIMITTAJA_H
#define TOIMITTAJA_H

#include "kantaasiakastoimittaja.h"


class Toimittaja : public KantaAsiakasToimittaja
{
    Q_OBJECT
public:
    explicit Toimittaja(QObject *parent = nullptr);


    QStringList tilit() const { return tilit_;}
    void setTilit(const QStringList& lista);

    AsiakasToimittajaTaydentaja* taydentaja() override;

signals:

public slots:
    void lataa(QVariantMap data);
    void valitse(const QString& nimi);
    void clear() { data_.clear(); tilit_.clear();}

protected:
    QStringList tilit_;

};

#endif // TOIMITTAJA_H
