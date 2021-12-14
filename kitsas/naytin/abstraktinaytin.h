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
#ifndef NAYTINWIDGET_H
#define NAYTINWIDGET_H

#include <QWidget>
#include <QPageLayout>

class QPrinter;

namespace Naytin {

/**
 * @brief Nayttimen abstrakti kantaluokka
 */
class AbstraktiNaytin : public QObject
{
    Q_OBJECT
public:
    explicit AbstraktiNaytin(QObject *parent = nullptr);
    virtual ~AbstraktiNaytin();

    virtual QWidget* widget() = 0;

    virtual QString otsikko() const { return QString();}

    virtual QString tiedostonMuoto() const = 0;
    virtual QString tiedostonPaate() const = 0;

    virtual bool csvMuoto() const { return false;}
    virtual QByteArray csv() const { return QByteArray(); }

    virtual QByteArray data() const = 0;

    virtual bool htmlMuoto() const { return false; }
    virtual QString html() const { return QString();}

    void raidoita(bool raidat = false);
    bool onkoRaidat() const { return raidat_;}
    virtual bool voikoRaidoittaa() const { return false;}

    virtual bool voikoZoomata() const { return false; }    

    virtual void asetaSuunta(QPageLayout::Orientation suunta);

    virtual bool voikoVirkistaa() const { return false;}
    virtual void virkista() {;}

signals:
    void otsikkoVaihtui(const QString& otsikko);

public slots:

    virtual void paivita() const = 0;
    virtual void tulosta(QPrinter* printer) const = 0;
    virtual QPrinter* printer();

    virtual void zoomIn() {}
    virtual void zoomOut() {}
    virtual void zoomFit() {}

private:
    bool raidat_ {false};

};

}

#endif // NAYTINWIDGET_H
