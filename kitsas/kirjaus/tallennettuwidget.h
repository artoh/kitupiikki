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
#ifndef TALLENNETTUWIDGET_H
#define TALLENNETTUWIDGET_H

#include <QWidget>

class QTimer;

namespace Ui {
class TallennettuWidget;
}

class TallennettuWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TallennettuWidget(QWidget *parent = nullptr);
    ~TallennettuWidget() override;


public slots:
    void nayta(int tunnus, const QDate& paiva, const QString& sarja = QString());
    void piiloon();

private:
    Ui::TallennettuWidget *ui;

    void mousePressEvent(QMouseEvent* event) override;

    QTimer* timer_;
};

#endif // TALLENNETTUWIDGET_H
