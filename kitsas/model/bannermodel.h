#ifndef BANNERMODEL_H
#define BANNERMODEL_H

#include <QAbstractListModel>
#include <QImage>

class BannerModel : public QAbstractListModel
{
    Q_OBJECT

public:

    enum {
        IdRooli = Qt::UserRole,
        NimiRooli = Qt::DisplayRole,
        KuvaRooli = Qt::DecorationRole,
        IndeksiRooli = Qt::UserRole + 1
    };

    explicit BannerModel(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void lisaa(const QString& nimi, const QImage& kuva);
    void muuta(const QString id, const QString& nimi, const QImage& kuva);
    void poista(const QString id);

    void lataa();

    QImage kuva(const QString uuid) const;

protected:
    class Banneri {
    public:
        Banneri();
        Banneri(const QString& id, const QString &nimi, const QImage& kuva = QImage());

        QString id() const { return id_;}
        QString nimi() const { return nimi_;}
        QImage kuva() const { return kuva_;}
        QPixmap kuvake() const;

        void asetaKuva(const QImage& image);
        void asetaNimi(const QString& nimi);

    protected:
        QString id_;
        QString nimi_;
        QImage kuva_;
    };


private:
    void lataaKuva();
    void kuvaSaapuu(const QString& uuid, QVariant* reply);

    QStringList latausLista_;
    QList<Banneri> bannerit_;



    static const QString ASETUSPOLKU;
    static const QString KYSELYPOLKU;
};

#endif // BANNERMODEL_H
