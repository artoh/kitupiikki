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
#ifndef KOMMENTITWIDGET_H
#define KOMMENTITWIDGET_H

#include <QSplitter>

class Tosite;
class QTextBrowser;
class QPlainTextEdit;
class QPushButton;

class KommentitWidget : public QSplitter
{
    Q_OBJECT
public:
    KommentitWidget(Tosite* tosite, QWidget *parent = nullptr);
    ~KommentitWidget();

public slots:
    void lataa();

protected:
    void paivita();
    void tallenna();
    void tallennettu();

signals:
    void kommentteja(bool onko);

private:
    Tosite* tosite_;

    QTextBrowser* browser_;
    QPlainTextEdit* edit_;
    QPushButton* button_;
};

#endif // KOMMENTITWIDGET_H
