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

#ifndef TILINAVAUS_H
#define TILINAVAUS_H

#include <QWidget>

#include "ui_tilinavaus.h"
#include "tilinavausmodel.h"

#include "maarityswidget.h"

class QSortFilterProxyModel;

/**
 * @brief Määritywidget kirjanpidon tilinavauksen kirjaamiseen
 *
 * Näyttää taulukon, johon voi tilien kohdalle kirjata avaavat summat
 * edellisestä tilinpäätöksestä
 *
 */
class Tilinavaus : public MaaritysWidget
{
    Q_OBJECT
public:
    explicit Tilinavaus(QWidget *parent = 0);
    ~Tilinavaus();

signals:

public slots:
    void naytaInfo(const QString &info);
    void hlostoMuutos();
    void tosite();
    void naytaPiilotetut(bool naytetaanko);

public:
    bool nollaa() override;
    bool tallenna() override;
    bool onkoMuokattu() override;

    QString ohjesivu() override { return "maaritykset/tilinavaus";}

private:
    Ui::Tilinavaus *ui;
    TilinavausModel *model;

    QSortFilterProxyModel *proxy;
};

#endif // TILINAVAUS_H
