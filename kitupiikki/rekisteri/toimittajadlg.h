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
#ifndef TOIMITTAJADLG_H
#define TOIMITTAJADLG_H

#include <QDialog>
#include <QStringList>

class Toimittaja;

namespace Ui {
class ToimittajaDlg;
}

class ToimittajaDlg : public QDialog
{
    Q_OBJECT

public:
    ToimittajaDlg(QWidget *parent);
    ~ToimittajaDlg() override;

public slots:
    void muokkaa(int id);
    void uusi(const QString& nimi);

signals:
    void toimittajaTallennettu(int id, const QString& nimi);

private slots:    
    void tarkastaTilit();
    void maaMuuttui();
    void haeToimipaikka();

    void accept() override;
    void toimittajaLadattu();
    void tallennettu(int id);
    void haeYTunnarilla();
    void yTietoSaapuu();

private:
    Ui::ToimittajaDlg *ui;
    Toimittaja* toimittaja_;
};

#endif // TOIMITTAJADLG_H
