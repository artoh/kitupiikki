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
    explicit Tilinavaus(QWidget *parent = nullptr);
    ~Tilinavaus() override;

signals:

public slots:
    void hlostoMuutos();
    void naytaPiilotetut(bool naytetaanko);
    void naytaVainKirjaukset(bool naytetaanko);
    void suodata(const QString& suodatusteksti);
    void erittely(const QModelIndex& index);

    void info(qlonglong vastaavaa, qlonglong vastattavaa, qlonglong tulos);
    void siirry(const QString& minne);

public:
    bool nollaa() override;
    bool tallenna() override;
    bool onkoMuokattu() override;

    QString ohjesivu() override { return "maaritykset/tilinavaus";}

private:
    Ui::Tilinavaus *ui;
    TilinavausModel *model;

    QSortFilterProxyModel *proxy;
    QSortFilterProxyModel *suodatus;
};

#endif // TILINAVAUS_H
