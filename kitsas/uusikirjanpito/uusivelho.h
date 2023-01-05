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
#ifndef UUSIVELHO_H
#define UUSIVELHO_H

#include <QObject>
#include <QWizard>
#include <QVariantMap>

namespace Ui {
    class Varmista;
    class UusiHarjoitus;
    class Uusivastuu;
    class UusiAlustus;
    class UusiTilikartta;
    class UusiTilikausi;
    class UusiNumerointi;
    class UusiLoppu;
}

class GroupData;

class UusiVelho : public QWizard
{
    Q_OBJECT
public:
    UusiVelho(QWidget *parent = nullptr);
    enum Sivut { ALOITUS, VARMISTA, HARJOITUS, VASTUU, ALUSTUS, TILIKARTTA, TIEDOT, TILIKAUSI, NUMEROINTI, SIJAINTI, LOPPU };
    bool lataaKartta(const QString& polku);
    static QVariantMap kartta(const QString& polku);
    bool ladattu() const { return !tilit_.isEmpty();}

    QVariantMap data() const;
    QString polku() const;

    int nextId() const override;

    QVariantMap asetukset_;
    QVariantList tilit_;
    QVariantList tilikaudet_;

    static QVariantMap asetukset( const QString& polku);

    QVariantMap alustusVelho(const QString& ytunnus, const QString& nimi, bool harjoitus);

protected:
    void veroViiteTulos(QVariant* data);
    QVariantMap veroViiteMap_;
    void help();

protected:


    class VarmistaSivu : public QWizardPage {
    public:
        VarmistaSivu();
    protected:
        Ui::Varmista *ui;
    };


    class Harjoitussivu : public QWizardPage {
    public:
        Harjoitussivu(UusiVelho *wizard);
    protected:        
        Ui::UusiHarjoitus *ui;
        UusiVelho *velho;
    };

    class VastuuSivu : public QWizardPage {
    public:
        VastuuSivu();
    protected:
        Ui::Uusivastuu *ui;
    };

    class UusiAlustus : public QWizardPage {
    public:
        UusiAlustus();
        void initializePage() override;
    protected:
        Ui::UusiAlustus *ui;
    };

    class Tilikarttasivu : public QWizardPage {
    public:
        Tilikarttasivu(UusiVelho *wizard);
        bool validatePage() override;
    protected:
        void tiedostosta();
        Ui::UusiTilikartta *ui;
        UusiVelho *velho;
    };

    class NumerointiSivu : public QWizardPage {
    public:
        NumerointiSivu();
    protected:
        Ui::UusiNumerointi *ui;
    };

    class LoppuSivu : public QWizardPage {
    public:
        LoppuSivu();
        void initializePage() override;
    protected:
        Ui::UusiLoppu* ui;
    };

};

#endif // UUSIVELHO_H
