/*
   Copyright (C) 2026 Kitss Oy

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

#ifndef MAKSUVIESTIVALIDATOR_H
#define MAKSUVIESTIVALIDATOR_H

#include <QValidator>

/**
 * @brief Maksun viesti -kentän validointi: Unicode-kirjaimet, desimaalinumerot
 * (QChar::isDigit), välilyönti, väliviiva, piste ja pilkku. Enintään 140 merkkiä.
 */
class MaksuViestiValidator : public QValidator
{
    Q_OBJECT
public:
    explicit MaksuViestiValidator(QObject *parent = nullptr);

    State validate(QString &input, int &pos) const override;
    void fixup(QString &input) const override;

    static constexpr int maxPituus() { return 140; }

    static bool sallittuMerkki(QChar ch);
};

#endif // MAKSUVIESTIVALIDATOR_H
