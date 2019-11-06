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
#ifndef TOIMITTAJADLG_H
#define TOIMITTAJADLG_H

#include <QDialog>
#include <QStringList>
#include <QVariantMap>

namespace Ui {
class AsiakasToimittajaDlg;
}

class AsiakasToimittajaDlg : public QDialog
{
    Q_OBJECT

public:
    AsiakasToimittajaDlg(QWidget *parent);
    ~AsiakasToimittajaDlg() override;

    static QString yToAlv(QString ytunnus);
    static QString alvToY(QString alvtunnus);

public slots:
    void muokkaa(int id);
    void uusi(const QString& nimi = QString());
    void ytunnuksella(const QString& ytunnus);
    void tuonti(const QVariantMap& map);

signals:
    void tallennettu(int id, const QString& nimi);

protected:
    void lataa(int id);
    void tauluun(QVariantMap map = QVariantMap());


private slots:    
    void tarkastaTilit();
    void maaMuuttui();
    void haeToimipaikka();
    void nimiMuuttuu();

    void accept() override;
    void reject() override;
    void dataSaapuu(QVariant* data);

    void haeYTunnarilla();
    void yTietoSaapuu();
    void tallennusValmis(QVariant* data);


private:
    Ui::AsiakasToimittajaDlg *ui;
    int id_ = 0;
};

#endif // TOIMITTAJADLG_H
