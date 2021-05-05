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
#ifndef PDFANALYZERTEXT_H
#define PDFANALYZERTEXT_H

#include <QRectF>
#include <QString>
#include <QList>

class PdfAnalyzerWord
{
public:
    PdfAnalyzerWord();
    PdfAnalyzerWord(const QRectF& rect, const QString& text, bool spaceAfter_);

    QRectF boundingRect() const { return boundingRect_;}
    QString text() const { return text_;}
    bool hasSpaceAfter() const { return spaceAfter_;}

private:
    QRectF boundingRect_;
    QString text_;
    bool spaceAfter_;
};


class PdfAnalyzerText
{
public:
    PdfAnalyzerText();

    QRectF boundingRect() const;
    QString text() const;

    void addWord(const QRectF& rect, const QString& text, bool spaceAfter = false);
private:
    QList<PdfAnalyzerWord> words_;
    PdfAnalyzerText* next_;
};

#endif // PDFANALYZERTEXT_H
