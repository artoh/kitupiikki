#ifndef TOIMINIMIMODEL_H
#define TOIMINIMIMODEL_H

#include <QAbstractListModel>
#include <QImage>

class Kirjanpito;

class ToiminimiModel : public QAbstractListModel
{
    Q_OBJECT    

public:

    enum ToiminimiRoolit {
        Nimi = Qt::DisplayRole,
        Katuosoite = Qt::UserRole + 2,
        Postinumero = Qt::UserRole + 3,
        Kaupunki = Qt::UserRole + 4,
        Puhelin = Qt::UserRole + 5,
        Sahkoposti = Qt::UserRole + 6,
        Kotisivu = Qt::UserRole + 7,
        LogonSijainti = Qt::UserRole + 10,
        LogonKorkeus = Qt::UserRole + 11,
        VariKehys = Qt::UserRole + 12,
        VariVarjo = Qt::UserRole + 13,

        Piilossa = Qt::UserRole + 100,
        Nakyva = Qt::UserRole + 101,
        Logo = Qt::DecorationRole,
        Indeksi = Qt::UserRole + 200,
    };




    ToiminimiModel(Kirjanpito *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;



    void lataa();
    void tallenna();

    int lisaaToiminimi(const QString& toiminimi);

    QString tieto(int rooli = Nimi, int indeksi = 0, const QString &oletus = QString()) const;
    void aseta(int indeksi, int rooli, const QString& tieto);
    QColor vari(int rooli = VariKehys, int indeksi=0, const QString &oletus="128,128,128");

    QImage logo(int indeksi = 0) const;
    void asetaLogo(int indeksi, const QImage& logo);

protected:
    void logoSaapui(int indeksi, QVariant* reply);
    QString toString() const;

private:
    QVector<QVariantMap> lista_;
    QVector<QImage> logo_;

    static std::map<int,QString> avaimet__;

};

#endif // TOIMINIMIMODEL_H
