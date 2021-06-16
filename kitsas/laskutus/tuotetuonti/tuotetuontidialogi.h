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
#ifndef TUOTETUONTIDIALOGI_H
#define TUOTETUONTIDIALOGI_H

#include <QDialog>

class TuoteTuontiModel;
class QDialogButtonBox;

class TuoteTuontiDialogi : public QDialog
{

public:
    TuoteTuontiDialogi(const QString& tiedosto, QWidget* parent = nullptr);

    void accept() override;

protected:
    void tallennaSeuraava();

private:
    TuoteTuontiModel *model_;
    QVariantList tallennusLista_;
    QDialogButtonBox *bbox;
};

#endif // TUOTETUONTIDIALOGI_H
