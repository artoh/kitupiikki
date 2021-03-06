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
#ifndef REKISTERITUONTIDLG_H
#define REKISTERITUONTIDLG_H

#include <QDialog>

class RekisteriTuontiModel;
class QComboBox;

namespace Ui {
class RekisteriTuontiDlg;
}

class RekisteriTuontiDlg : public QDialog
{
    Q_OBJECT

public:
    explicit RekisteriTuontiDlg(const QString& tiedosto, QWidget *parent = nullptr);
    ~RekisteriTuontiDlg() override;

    void accept() override;

protected:
    void alustaMaksutavat(QComboBox* combo);
    void tallennaSeuraava();
    int maksutapa(const QVariantMap& map);

private:
    Ui::RekisteriTuontiDlg *ui;
    RekisteriTuontiModel* model_;
    QVariantList tallennusLista_;
};

#endif // REKISTERITUONTIDLG_H
