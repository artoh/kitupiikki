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

#ifndef TILINPAATOSEDITORI_H
#define TILINPAATOSEDITORI_H

#include <QMainWindow>
#include <QTextEdit>
#include <QAction>
#include <QToolBar>
#include <QPrinter>

#include "db/tilikausi.h"


class TilinpaatosEditori : public QMainWindow
{
    Q_OBJECT
public:
    explicit TilinpaatosEditori(Tilikausi tilikausi);

signals:

public slots:
    void esikatsele();

protected:
    void luoAktiot();
    void luoPalkit();

    void lataa();

protected:
    QTextEdit *editori_;
    Tilikausi tilikausi_;

    QAction *esikatseleAction_;
    QAction *vahvistaAction_;
    QAction *liitaAction_;

    QToolBar *tilinpaatosTb_;

    QPrinter printer_;
};

#endif // TILINPAATOSEDITORI_H
