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

#ifndef ALOITUSSIVU_H
#define ALOITUSSIVU_H

#include <QTextBrowser>

class AloitusSivu : public QTextBrowser
{
    Q_OBJECT
public:
    AloitusSivu();

protected slots:
    void linkkiKlikattu(const QUrl& url);

signals:
    void toiminto(const QString& toiminto);

protected:
    void lisaaTaulu(const QString& otsikko, const QString& sisalto, const QString& linkit, const QString& kuva);

};

#endif // ALOITUSSIVU_H
