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
#ifndef RYHMAVALINTALISTWIDGET_H
#define RYHMAVALINTALISTWIDGET_H

#include <QListWidget>

/**
 * @brief ListWidget ryhmien valintaan kumppanidialogissa
 */
class RyhmaValintaListWidget : public QListWidget
{
    Q_OBJECT
public:
    RyhmaValintaListWidget(QWidget* parent = nullptr);

    QVariantList valitutRyhmat() const;
    void valitseRyhmat(const QVariantList& lista);

private:
    void lataaRyhmat();
};

#endif // RYHMAVALINTALISTWIDGET_H
