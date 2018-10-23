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

#ifndef TASEERAVALINTADIALOGI_H
#define TASEERAVALINTADIALOGI_H

#include <QDialog>
#include <QSortFilterProxyModel>

#include "db/tili.h"
#include "db/eranvalintamodel.h"

#include "db/vientimodel.h"

namespace Ui {
class TaseEraValintaDialogi;
}

/**
 * @brief Kirjausnäytön dialogi käsinkirjauksen tase-erän valintaan
 */
class TaseEraValintaDialogi : public QDialog
{
    Q_OBJECT

    enum Tabs
    {
        ERA_TAB = 0,
        OSTO_TAB = 1
    };

public:
    explicit TaseEraValintaDialogi(QWidget *parent = 0);
    ~TaseEraValintaDialogi();

    bool nayta(VientiModel *model, QModelIndex &index);

    int eraId();
    int poistoKk();

public slots:
    void eraValintaVaihtuu();
    void sntSuodatusVaihtuu();

private:
    Ui::TaseEraValintaDialogi *ui;

    QSortFilterProxyModel *proxy_;
    QSortFilterProxyModel *sntProxy_;
    EranValintaModel model_;

    Tili tili_;
    int taseEra_ = 0;
    int vientiId_ = 0;

};

#endif // TASEERAVALINTADIALOGI_H
