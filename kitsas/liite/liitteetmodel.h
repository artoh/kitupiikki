#ifndef LIITTEETMODEL_H
#define LIITTEETMODEL_H

#include "cacheliite.h"
#include <QPixmap>
#include <QAbstractListModel>
#include <QBuffer>
#include <QPdfDocument>

#include "liite.h"

class LiitteetModel : public QAbstractListModel
{
    Q_OBJECT


public:

    enum {
        SisaltoRooli = Qt::UserRole + 1,
        NimiRooli = Qt::UserRole + 2,
        TyyppiRooli = Qt::UserRole + 3,
        RooliRooli = Qt::UserRole + 4,
        IdRooli = Qt::UserRole + 5
    };

    explicit LiitteetModel(QObject *parent = nullptr);
    ~LiitteetModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;


    void lataa(const QVariantList& data);
    void clear();
    void asetaInteraktiiviseksi(bool onko = true);

    bool lisaa(QByteArray liite, const QString& tiedostonnimi, const QString& rooli=QString());
    bool lisaaHeti(QByteArray liite, const QString& polku);
    bool lisaaHetiTiedosto(const QString& polku);

    int tallennettaviaLiitteita() const;
    void tallennaLiitteet(int tositeId);
    void poistaInboxistaLisattyjenTiedostot();

    void nayta(int indeksi);
    int naytettavaIndeksi() const { return naytettavaIndeksi_; }
    QModelIndex naytettava() const;
    bool tallennetaanko() const;
    QVariantList liitettavat() const;

    void poista(int indeksi);

    QPdfDocument* pdfDocument() { return pdfDoc_;}
    QByteArray *sisalto();

    void liitteenTilaVaihtui(Liite::LiiteTila uusiTila);
    void ocr(const QVariantMap& data);

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

protected:
    void valimuistiLiite(int liiteId);
    void naytaKayttajalle();

    void pdfTilaVaihtui(QPdfDocument::Status status);
    void tuoLiite(const QString &tyyppi, const QByteArray &sisalto);

    void tarkastaKaikkiLiitteet();

    QByteArray esikasittely(QByteArray sisalto, const QString &tiedostonnimi = QString());

signals:
    int valittuVaihtui(int indeksi);
    void naytaPdf();
    void naytaSisalto();
    void kaikkiLiitteetHaettu();
    void liitteetTallennettu();    

    void liitettaTallennetaan(bool tallennetaanko);

    void tuonti(const QVariantMap& data);
    void ocrKaynnissa(bool onko);
    void hakuVirhe(int virhe, int liiteId);

private:
    QList<Liite*> liitteet_;
    QBuffer* puskuri_;
    QPdfDocument* pdfDoc_;

    bool interaktiivinen_ = false;
    int naytettavaIndeksi_ = -1;
    int pdfTuontiIndeksi_ = -1;

    bool tallennetaanko_ = false;
};

#endif // LIITTEETMODEL_H
