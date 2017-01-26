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

#ifndef RAPORTTI_H
#define RAPORTTI_H

#include <QObject>
#include <QPrinter>
#include <QWidget>
#include <QIcon>
#include <QPainter>


/**
 * @brief The Raportin kantaluokka
 *
 *
 */
class Raportti : public QObject
{
    Q_OBJECT
public:
    Raportti(QObject *parent = 0);

    virtual QString raporttinimi() const = 0;
    virtual QString lomake() const = 0;
    virtual QIcon kuvake() const;

    virtual void alustaLomake(QWidget *kehys);

    virtual void tulosta(QPrinter *printer, QWidget *kehys);

signals:

public slots:
};

#endif // RAPORTTI_H
