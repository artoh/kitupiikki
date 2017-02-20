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

#ifndef KIRJAUSAPURIDIALOG_H
#define KIRJAUSAPURIDIALOG_H

#include "db/tositemodel.h"

#include <QDialog>

namespace Ui {
class KirjausApuriDialog;
}

class KirjausApuriDialog : public QDialog
{
    Q_OBJECT

public:
    KirjausApuriDialog(TositeModel *tositeModel, QWidget *parent = 0);
    ~KirjausApuriDialog();


public slots:
    void tiliTaytetty();
    void laskeNetto();
    void laskeBrutto();
    void laskeVerolla();
    void alvLajiMuuttui();

    void tarkasta();

    void accept();

protected:
    void teeEhdotus(const QString& teksti, bool tiliOnDebet, const QIcon& kuvake = QIcon());

private:
    Ui::KirjausApuriDialog *ui;
    TositeModel *model;

    double bruttoEur = 0.0;
    double nettoEur = 0.0;

};

#endif // KIRJAUSAPURIDIALOG_H
