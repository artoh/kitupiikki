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
#ifndef KPEUROEDIT_H
#define KPEUROEDIT_H

#include <QLineEdit>

/**
 * @brief Euroeditori
 *
 * @author Arto Hyvättinen
 * @since 2.0
 */

class KpEuroEdit : public QLineEdit
{
    Q_OBJECT
public:
    KpEuroEdit(QWidget* parent = nullptr);

    qlonglong asCents() { return miinus_ ? 0 - cents_ : cents_; }
    double value() { return asCents() / 100.0; }

public slots:
    void setCents(qlonglong cents);
    void setValue(double euros);

protected slots:
    void edited(const QString& newtext);

protected:
    void keyPressEvent(QKeyEvent* event) override;

    qlonglong cents_;
    bool miinus_ = false;



};

#endif // KPEUROEDIT_H
