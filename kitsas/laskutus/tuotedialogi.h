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
#ifndef TUOTEDIALOGI_H
#define TUOTEDIALOGI_H

#include <QDialog>

namespace Ui {
class TuoteDialogi;
}

class TuoteDialogi : public QDialog
{
    Q_OBJECT

public:
    explicit TuoteDialogi(QWidget *parent = nullptr);
    ~TuoteDialogi() override;

public slots:
    void muokkaa(const QVariantMap& map);
    void uusi();

    void accept() override;

private slots:
    void laskeBrutto();
    void laskeNetto();    

private:
    Ui::TuoteDialogi *ui;
    int muokattavanaId_ = 0;
};

#endif // TUOTEDIALOGI_H
