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
#ifndef EIPDFNAYTIN_H
#define EIPDFNAYTIN_H

#include "abstraktinaytin.h"

class QWidget;

namespace Naytin {

/**
 * @brief Korvaa pdf-näyttimen silloin, kun pdf-toiminnot poistettu käytöstä
 */
class EiPdfNaytin : public AbstraktiNaytin
{
    Q_OBJECT
public:
    EiPdfNaytin(const QByteArray& pdf, QObject *parent = nullptr);

    virtual QWidget* widget() override { return widget_;}

    virtual QString tiedostonMuoto() const override { return tr("pdf-tiedosto (*.pdf)");}
    virtual QString tiedostonPaate() const override { return "pdf"; }

    virtual QByteArray data() const override;

public slots:

    void paivita() const override;
    void tulosta(QPrinter* printer) const override;

    void avaaTiedostolla();

protected:
    QByteArray data_;
    QWidget *widget_ = nullptr;
};


}


#endif // EIPDFNAYTIN_H
