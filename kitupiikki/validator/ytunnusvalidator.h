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

#ifndef YTUNNUSVALIDATOR_H
#define YTUNNUSVALIDATOR_H

#include <QValidator>

/**
 * @brief Y-tunnuksen muodon tarkastus
 */
class YTunnusValidator : public QValidator
{
public:
    YTunnusValidator(bool alvtunnuksia = false);

    State validate(QString &input, int &pos) const;

    static QValidator::State kelpo(const QString &input, bool alvkelpaa = false);
    static bool kelpaako(const QString& input, bool alvtunnuksia = false);
private:
    bool alvtunnuskelpaa = false;
};

#endif // YTUNNUSVALIDATOR_H
