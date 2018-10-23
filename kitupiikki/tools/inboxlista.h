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
#ifndef INBOXLISTA_H
#define INBOXLISTA_H

#include <QListWidget>

class QFileSystemWatcher;

class InboxLista : public QListWidget
{
    Q_OBJECT
public:
    InboxLista();

    void alusta();
    void paivita();


signals:
    void nayta(bool naytetaanko);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void aloitaRaahaus();

private:
    QString polku_;
    QFileSystemWatcher *vahti_;
    QPoint alkuPos_;
};

#endif // INBOXLISTA_H
