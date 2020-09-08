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
#ifndef TULOVERODIALOG_H
#define TULOVERODIALOG_H

#include <QDialog>
#include "db/tilikausi.h"

namespace Ui {
class TuloveroDialog;
}

class TuloveroDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TuloveroDialog(QWidget *parent = nullptr);
    ~TuloveroDialog() override;

    void alusta(const QVariantMap& verolaskelma, const Tilikausi& tilikausi);

    void accept() override;

signals:
    void tallennettu();

protected slots:
    void paivitaYlevero();
    void paivitaVahennys();
    void paivitaTulos();
    void paivitaVero();
    void paivitaJaannos();
    void kirjattu();

private:
    Ui::TuloveroDialog *ui;
    Tilikausi tilikausi_;
};

#endif // TULOVERODIALOG_H
