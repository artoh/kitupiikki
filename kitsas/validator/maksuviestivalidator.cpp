/*
   Copyright (C) 2026 Kitsas Oy

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

#include "maksuviestivalidator.h"

bool MaksuViestiValidator::sallittuMerkki(QChar ch)
{
    if (ch == u' ' || ch == u'-' || ch == u'.' || ch == u',')
        return true;
    return ch.isLetter() || ch.isDigit();
}

MaksuViestiValidator::MaksuViestiValidator(QObject *parent)
    : QValidator(parent)
{
}

void MaksuViestiValidator::fixup(QString &input) const
{
    QString out;
    const int maxLen = maxPituus();
    const int inLen = static_cast<int>(input.length());
    out.reserve(inLen > maxLen ? maxLen : inLen);
    for (QChar ch : input) {
        if (!sallittuMerkki(ch))
            continue;
        if (out.length() >= maxLen)
            break;
        out.append(ch);
    }
    input = out;
}

QValidator::State MaksuViestiValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos);
    if (input.isEmpty())
        return Intermediate;
    if (input.length() > maxPituus())
        return Invalid;
    for (QChar ch : input) {
        if (!sallittuMerkki(ch))
            return Invalid;
    }
    return Acceptable;
}
