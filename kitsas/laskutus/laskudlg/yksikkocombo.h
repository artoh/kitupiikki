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
#ifndef YKSIKKOCOMBO_H
#define YKSIKKOCOMBO_H

#include <QComboBox>
#include "../yksikkomodel.h"

class YksikkoCombo : public QComboBox
{
    Q_OBJECT
public:
    YksikkoCombo(QWidget* parent = nullptr, bool editable = true);

    void setYksikko(const QString& yksikko);
    void setUNkoodi(const QString& koodi);

    QString yksikko() const;
    QString unKoodi() const;

protected:
    void focusOutEvent(QFocusEvent *e) override;

    void vaihtui(int indeksi);
    void syotetty(const QString& teksti);

    YksikkoModel yksikot_;

    QString yksikko_;
    QString unKoodi_;

};

#endif // YKSIKKOCOMBO_H
