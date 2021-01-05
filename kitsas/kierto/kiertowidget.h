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
#ifndef KIERTOWIDGET_H
#define KIERTOWIDGET_H

#include <QWidget>

class Tosite;
namespace Ui {
    class Kierto;
}

class KiertoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KiertoWidget(Tosite* tosite, QWidget *parent = nullptr);
    ~KiertoWidget();

    QString kululaskuVirtuaalikoodi() const;
    QByteArray kululaskuQr() const;
    QString luotuviite() const;

public slots:
    void lataaTosite();    
    void paivitaViivakoodi();

signals:
    void tallenna(int tilaan);

protected:
    void valmis(int tilaan);        

private:
    Ui::Kierto* ui;
    Tosite* tosite_;
    QString iban_;

};

#endif // KIERTOWIDGET_H
