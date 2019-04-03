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

#ifndef DEVTOOL_H
#define DEVTOOL_H


#include <QDialog>
#include <QMap>

namespace Ui {
class DevTool;
}

/**
 * @brief Kehittäjän työkaluja
 */
class DevTool : public QDialog
{
    Q_OBJECT

public:
    explicit DevTool(QWidget *parent = nullptr);
    ~DevTool();

protected slots:
    void haeAsetus(const QString& asetus);
    void tallennaAsetus();
    void poistaAsetus();
    void tabMuuttui(int tab);

    void kysely();
    void vastausSaapui(QVariantMap *vastaus, int status);

    void uusiPeli();
    void peliNapautus(int ruutu);

protected:
    /**
     * @brief Tarkastaa voiton ja ilmoittaa tuloksen
     * @return Voittaja
     */
    int tarkastaVoitto();
    /**
     * @brief Tekee voiton tarkastuksen
     * @return Voittaja
     */
    int voitonTarkastaja(const QVector<int> &taulu);
    int voittajaRivilla(const QVector<int> &taulu, int a, int b, int c) const;

protected:
    void alustaRistinolla();
    QMap<int,QPushButton*> pelinapit_;
    QVector<int> peliRuudut_;
    bool pelissa_ = true;


private:
    Ui::DevTool *ui;
};

#endif // DEVTOOL_H
