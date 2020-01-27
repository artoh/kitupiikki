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
#ifndef ARKISTOIJA_H
#define ARKISTOIJA_H

#include "db/tilikausi.h"
#include "raportti/raportinkirjoittaja.h"

#include <QObject>
#include <QDir>
#include <QList>
#include <QMap>
#include <QQueue>


class QProgressDialog;

class Arkistoija : public QObject
{
    Q_OBJECT
public:
    explicit Arkistoija(const Tilikausi& tilikausi, QObject *parent = nullptr);

    void arkistoi();
    QByteArray tosite(const QVariantMap &tosite, int indeksi);


signals:
    void arkistoValmis(QString hakemisto);

protected:
    bool luoHakemistot();    
    void arkistoiTositteet();
    void arkistoiRaportit();
    void arkistoiTilinpaatos();
    void arkistoiByteArray(const QString& tiedostonnimi, const QByteArray& array);
    void kirjoitaHash() const;
    void merkitseArkistoiduksi();
    void tositeLuetteloSaapuu(QVariant* data);
    void arkistoiSeuraava();
    void arkistoiSeuraavaTosite();
    void arkistoiTosite(QVariant* data, int indeksi);
    void arkistoiSeuraavaLiite();
    void arkistoiLiite(QVariant* data, const QString tiedosto);
    void arkistoiRaportti(RaportinKirjoittaja rk, const QString& tiedosto);
    void viimeistele();

protected:
    static QString tiedostonnimi(const QDate& pvm, const QString& sarja, int tunniste);

    QString navipalkki(int indeksi = -10) const;

protected:
    class JonoTosite {
    public:
        explicit JonoTosite();
        JonoTosite(const QString& sarja, int tunniste, int id, const QDate& pvm);
        JonoTosite(const QVariantMap &map);
        QString sarja() const { return sarja_;}
        int tunniste() const { return tunniste_;}
        int id() const { return id_;}
        QDate pvm() const { return pvm_;}
        QString tiedostonnimi();
    protected:
        QString sarja_;
        int tunniste_;
        int id_;
        QDate pvm_;
    };

private:
    Tilikausi tilikausi_;
    QString hakemistoPolku_;

    QProgressDialog *progressDlg_ = nullptr;

    QList<JonoTosite> tositeJono_;
    QHash<int,QString> liiteNimet_;
    QQueue<int> liiteJono_;
    QList<QPair<QString,QString>> raporttiNimet_;

    QByteArray shaBytes;

    int arkistoitavaTosite_ = 0;
    bool logo_ = false;
    int raporttilaskuri_=0;
    int liitelaskuri_ = 0;
    bool keskeytetty_ = false;

};

#endif // ARKISTOIJA_H
