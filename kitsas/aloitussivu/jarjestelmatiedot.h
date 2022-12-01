#ifndef JARJESTELMATIEDOT_H
#define JARJESTELMATIEDOT_H

#include <QAbstractTableModel>

class JarjestelmaTiedot : public QAbstractTableModel
{
    Q_OBJECT

protected:
    class Tieto {
    public:
        explicit Tieto();
        Tieto(const QString& avain, const QString& arvo);

        QString avain() const { return avain_;}
        QString arvo() const { return arvo_;}

    protected:
        QString avain_;
        QString arvo_;
    };

public:
    explicit JarjestelmaTiedot(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void lisaa(const QString& avain, const QString& arvo);

    void lataaSysteemiTiedot();
    QVariantList asList() const;
protected:
    void pyydaInfo();
    void infoSaapuu(QVariant* info);

private:
    QList<Tieto> tiedot_;
};

#endif // JARJESTELMATIEDOT_H
