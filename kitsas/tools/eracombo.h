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
#ifndef ERACOMBO_H
#define ERACOMBO_H

#include <QComboBox>
#include <QDate>

class EraCombo : public QComboBox
{
    Q_OBJECT
public:
    EraCombo(QWidget *parent = nullptr);

    enum {
        AvoinnaRooli = Qt::UserRole + 1,
        SeliteRooli = Qt::UserRole + 2
    };

    int valittuEra() const;
    QVariantMap eraMap() const;

public slots:
    void lataa(int tili);
    void valitse(int eraid);

signals:
    void valittu(int eraid, double avoinna, const QString& selite);

private slots:
    void dataSaapuu(QVariant* data);
    void vientiSaapuu(QVariant* data);
    void valintaMuuttui();

private:
    int valittuna_;    
    QVariantList data_;
    bool latauksessa_ = false;
    int tili_ = 0;

};

#endif // ERACOMBO_H
