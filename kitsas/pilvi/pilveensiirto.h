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
#ifndef PILVEENSIIRTO_H
#define PILVEENSIIRTO_H

#include <QDialog>
#include <QQueue>

class PilviModel;

namespace Ui {
class PilveenSiirto;
}

class PilveenSiirto : public QDialog
{
    Q_OBJECT

public:
    enum {
        ALOITUS, KAYNNISSA, VALMIS
    };

    explicit PilveenSiirto(QWidget *parent = nullptr);
    ~PilveenSiirto() override;

    void accept() override;

private:
    void alustaAlkusivu();
    void initSaapuu(QVariant* data);
    void pilviLuotu(QVariant* data);
    void avaaLuotuPilvi();
    void kumppaniListaSaapuu(QVariant* data);
    void kysySeuraavaKumppani();
    void tallennaKumppani(QVariant* data);
    void haeTositeLista();
    void tositeListaSaapuu(QVariant* data);
    void kysySeuraavaTosite();
    void tallennaTosite(QVariant* data);

private:
    Ui::PilveenSiirto *ui;
    PilviModel* pilviModel_;

    int tositelkm_ = 0;
    int liitelkm_ = 0;
    int pilviId_ = 0;

    QQueue<int> kumppanit;
    QQueue<int> tositteet;

};

#endif // PILVEENSIIRTO_H
