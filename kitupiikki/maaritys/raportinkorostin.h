/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef RAPORTINKOROSTIN_H
#define RAPORTINKOROSTIN_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>

/**
 * @brief Syntaksivärjää muokattavan raportin
 */
class RaportinKorostin : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    RaportinKorostin(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text) override;

private:
    QFont lihava;
    QRegularExpression tilinroRe;


};

#endif // RAPORTINKOROSTIN_H
