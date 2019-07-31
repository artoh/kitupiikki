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
#ifndef ASIAKASTOIMITTAJAVALINTA_H
#define ASIAKASTOIMITTAJAVALINTA_H

#include <QWidget>

class QLineEdit;
class QPushButton;
class AsiakasToimittajaTaydentaja;
class QCompleter;

class AsiakasDlg;
class ToimittajaDlg;

class AsiakasToimittajaValinta : public QWidget
{
    Q_OBJECT
public:
    AsiakasToimittajaValinta(QWidget *parent = nullptr);

    int id() const { return id_;}
    QString nimi() const;

signals:

public slots:
    void set(int id, const QString& nimi, bool toimittaja = false);
    void clear();
    void alusta(bool toimittaja);


private slots:
    void valitseAsiakas();
    void muokkaa();
    void talletettu(int id, const QString &nimi);

protected:
    QLineEdit* edit_;
    QPushButton* button_;
    bool toimittaja_ = false;
    AsiakasToimittajaTaydentaja* taydentajaModel_;
    QCompleter *completer_;

    AsiakasDlg *asiakasDlg_ = nullptr;
    ToimittajaDlg *toimittajaDlg_ = nullptr;


    int id_=0;
};

#endif // ASIAKASTOIMITTAJAVALINTA_H
