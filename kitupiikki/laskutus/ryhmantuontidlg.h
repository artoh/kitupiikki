/*
   Copyright (C) 2018 Arto Hyvättinen

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
#ifndef RYHMANTUONTIDLG_H
#define RYHMANTUONTIDLG_H

#include <QDialog>
#include <QItemSelection>

class RyhmanTuontiModel;

namespace Ui {
class RyhmanTuontiDlg;
}

class RyhmanTuontiDlg : public QDialog
{
    Q_OBJECT

public:
    RyhmanTuontiDlg(const QString& tiedostonnimi, QWidget *parent = nullptr);
    ~RyhmanTuontiDlg();
    RyhmanTuontiModel* data() { return model;}

public slots:
    void vaihdaSarake();
    void naytaSarake(const QItemSelection& valinta);

private:
    Ui::RyhmanTuontiDlg *ui;
    RyhmanTuontiModel *model;
};

#endif // RYHMANTUONTIDLG_H
