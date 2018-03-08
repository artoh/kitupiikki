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

#include "kaavankorostin.h"

KaavanKorostin::KaavanKorostin(QTextDocument *parent) :
    QSyntaxHighlighter( parent )
{
    lihava.setFontWeight(QFont::Bold);
    lihava.setForeground(QBrush(Qt::magenta));

    raporttiRe.setPattern("^@.+[\\*!].+@$");
    raporttiRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
}

void KaavanKorostin::highlightBlock(const QString &text)
{
    // Korostettavia ##otsikko #ehtomääre @raportti!teksti@ @määre@ <html>
    if( text.startsWith("##"))
    {
        setFormat(0, text.length(), lihava);
    }
    else if( text.startsWith("# "))
    {
        setFormat(0, text.length(), QColor(Qt::darkGray));
    }
    else if( text.startsWith("#"))
    {
        setFormat(0, text.length(), QColor(Qt::magenta));
    }
    else if( text.startsWith("?"))
    {
        setFormat(0, text.length(), QColor(Qt::darkCyan));
    }
    else if( text.startsWith("@"))
    {
        if( text == "@sha@" || text == "@henkilosto@")
            setFormat(0, text.length(), QColor(Qt::green));
        else if( raporttiRe.match(text).hasMatch() )
            setFormat(0, text.length(), QColor(Qt::green));
    }
    else
    {
        int tagialkaa = -1;
        for(int i=0; i < text.length(); i++)
        {
            QChar merkki = text.at(i);

            if( merkki == '<')
                tagialkaa = i;
            else if( merkki == '>' && tagialkaa > -1)
            {
                setFormat(tagialkaa, i - tagialkaa + 1, QColor(Qt::blue));
                tagialkaa = -1;
            }
        }
    }
}
