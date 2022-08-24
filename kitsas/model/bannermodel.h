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
    void muuta(int indeksi, const QString& nimi, const QImage& kuva);
    void poista(int indeksi);

    void lataa();

    QImage kuva(const QString uuid) const;

private:
    void lataaKuva();
    void kuvaSaapuu(const QString& uuid, QVariant* reply);

    QStringList idt_;
    QStringList latausLista_;

    QMap<QString,QString> nimet_;
    QMap<QString,QImage> kuvat_;


    static const QString ASETUSPOLKU;
    static const QString KYSELYPOLKU;
};

#endif // BANNERMODEL_H
