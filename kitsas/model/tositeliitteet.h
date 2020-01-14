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
#ifndef TOSITELIITTEET_H
#define TOSITELIITTEET_H

#include <QAbstractListModel>

class TositeLiitteet : public QAbstractListModel
{
    Q_OBJECT

protected:
    class TositeLiite
    {
    public:
        TositeLiite(int id=0, const QString& nimi = QString(),
                    const QByteArray& sisalto = QByteArray(), const QString& rooli = QString());

        int getLiiteId() const;
        void setLiiteId(int value);


        QString getNimi() const;
        void setNimi(const QString &value);
        QByteArray getSisalto() const;

        QString getRooli() const;
        void setRooli(const QString &rooli);

        bool getLiitettava() const;
        void setLiitettava(int id);

    protected:
        int liiteId_ = 0;
        QString nimi_;
        QByteArray sisalto_;
        QString rooli_;
        bool liitettava_ = false;
    };

public:
    explicit TositeLiitteet(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void lataa(QVariantList data);
    void clear();

    bool lisaa(const QByteArray& liite, const QString& tiedostonnimi, const QString& rooli=QString());
    bool lisaaTiedosto(const QString& polku);

    bool lisaaHeti(const QByteArray& liite, const QString& tiedostonnimi);
    bool lisaaHetiTiedosto(const QString& polku);

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    int tallennettaviaLiitteita() const;
    void tallennaLiitteet(int tositeId);
    QVariantList liitettavat() const;

public slots:
    void nayta(int indeksi);
    void poista(int indeksi);

signals:
    void liitteetTallennettu();
    void naytaliite(const QByteArray& data);
    void tuonti(QVariant *data);

private slots:
    void tallennaSeuraava();
    void liitesaapuu(QVariant* data);

protected:
    static QByteArray lueTiedosto(const QString &polku);

private:
    QList<TositeLiite> liitteet_;

    int tositeId_ = -1;
    int tallennuksessa_ = -1;

};

#endif // TOSITELIITTEET_H
