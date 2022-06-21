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
#ifndef RYHMALASKUTAB_H
#define RYHMALASKUTAB_H

#include <QSplitter>

class LaskutettavatModel;
class QTableView;
class QListView;
class QComboBox;
class QPushButton;
class QMenu;

class RyhmalaskuTab : public QSplitter
{
    Q_OBJECT
public:
    RyhmalaskuTab(QWidget *parent = nullptr);

    LaskutettavatModel* model() { return laskutettavat_;}

protected slots:
    void lisaaKaikki();
    void uusiAsiakas();
    void poista();
    void suodataRyhma();

protected:
    void luoUi();

private:
    LaskutettavatModel *laskutettavat_;
    QComboBox *ryhmaCombo_;
    QListView* asiakasView_;
    QTableView* laskutettavatView_;
    QPushButton* poistaNappi_;
    QPushButton* tapaNappi_;
    QMenu* tapaMenu_;

};

#endif // RYHMALASKUTAB_H
