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
#ifndef BUDJETTIDLG_H
#define BUDJETTIDLG_H

#include <QDialog>

namespace Ui {
class BudjettiDlg;
}

class BudjettiModel;
class KohdennusProxyModel;

/**
 * @brief Budjetointidialogi
 *
 * Budjetti laaditaan tilikausittain kohdennuksille
 *
 * @since 1.1
 */
class BudjettiDlg : public QDialog
{
    Q_OBJECT

public:
    explicit BudjettiDlg(QWidget *parent = nullptr);
    ~BudjettiDlg() override;

public slots:
    void lataa(const QString &kausi);
    void kausivaihtuu();
    void paivita();
    void muokattu(qlonglong summa);

    void kysyTallennus();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::BudjettiDlg *ui;

    BudjettiModel* model_;
    KohdennusProxyModel* kohdennukset_;
};

#endif // BUDJETTIDLG_H
