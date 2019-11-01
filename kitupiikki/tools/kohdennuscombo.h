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
#ifndef KOHDENNUSCOMBO_H
#define KOHDENNUSCOMBO_H

#include <QComboBox>

class KohdennusProxyModel;

class KohdennusCombo : public QComboBox
{
    Q_OBJECT
public:
    KohdennusCombo(QWidget* parent = nullptr);
    int kohdennus() const;

public slots:
    void valitseKohdennus(int kohdennus);
    void suodataPaivalla(const QDate& pvm);
    void suodataValilla(const QDate& alkaa, const QDate& paattyy);

private slots:
    void vaihtuu();

signals:
    void kohdennusVaihtui(int kohdennus);

protected:
    KohdennusProxyModel *proxy_;
};

#endif // KOHDENNUSCOMBO_H
