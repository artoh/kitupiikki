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
#ifndef PILVIMODEL_H
#define PILVIMODEL_H

#include <QAbstractListModel>


/**
 * @brief Pilvessä olevien kirjanpitojen luettelo
 *
 * Kirjautuminen Kitupiikin pilveen ja kirjanpitojen luettelo
 *
 */
class PilviModel : public QAbstractListModel
{
public:
    PilviModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    QString kayttajaNimi() const { return kayttajaNimi_;}

public slots:
    void kirjaudu();

private slots:
    void kirjautuminenValmis();

signals:
    void kirjauduttu();



private:
    int kayttajaId_ = 0;
    QString kayttajaNimi_;

    QList<QVariantMap> pilvet_;
};

#endif // PILVIMODEL_H
