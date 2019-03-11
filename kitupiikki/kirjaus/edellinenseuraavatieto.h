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
#ifndef EDELLINENSEURAAVATIETO_H
#define EDELLINENSEURAAVATIETO_H

#include <QObject>

class TositeModel;

/**
 * @brief Tieto edellisestä ja seuraavasta tositteesta
 *
 * Jotta seuraavaan ja edelliseen tositteeseen voitaisiin siirtyä, sisältää
 * tiedon edellisestä ja seuraavasta tositteesta
 *
 */
class EdellinenSeuraavaTieto : public QObject
{
    Q_OBJECT
public:
    EdellinenSeuraavaTieto(TositeModel* model, QObject *parent = nullptr);

    int edellinenId() const { return edellinenId_;}
    int seuraavaId() const { return seuraavaId_ ;}


signals:
    void edellinenOlemassa(bool onko);
    void seuraavaOlemassa(bool onko);

public slots:
    void paivita();

private:
    TositeModel* model_;
    int edellinenId_ = -1;
    int seuraavaId_ = -1;
};

#endif // EDELLINENSEURAAVATIETO_H
