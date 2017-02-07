/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef ASETUSMODEL_H
#define ASETUSMODEL_H

#include <QObject>
#include <QHash>
#include <QSqlDatabase>

/**
 * @brief Asetusten käsittely
 *
 * Asetukset tallennetaan välittömästi tietokantaan
 * Yleisimmille tietotyypeille on valmiiksi muunnosfunktiot
 *
 */
class AsetusModel : public QObject
{
    Q_OBJECT
public:
    explicit AsetusModel(QSqlDatabase tietokanta, QObject *parent = 0);

    QString asetus(const QString& avain) const { return asetukset_.value(avain, QString()); }
    void aseta(const QString& avain, const QString& arvo);

    QDate pvm(const QString& avain) const;
    void aseta(const QString &avain, const QDate& pvm);

    bool onko(const QString& avain) const;
    void aseta(const QString& avain, bool totuusarvo);

    void asetaVar(const QString& avain, const QVariant& arvo);

    QStringList lista(const QString& avain) const;
    void aseta(const QString& avain, const QStringList& arvo);

    int luku(const QString& avain) const;
    void aseta(const QString &avain, int luku);

signals:

public slots:
    void lataa();


protected:
    QHash<QString,QString> asetukset_;
    QSqlDatabase tietokanta_;

};

#endif // ASETUSMODEL_H
