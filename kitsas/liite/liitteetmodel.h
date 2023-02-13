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
    explicit LiitteetModel(QObject *parent = nullptr);
    ~LiitteetModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void lataa(const QVariantList& data);
    void clear();
    void asetaInteraktiiviseksi(bool onko = true);

    bool lisaaHeti(const QByteArray& liite, const QString& polku);
    bool lisaaHetiTiedosto(const QString& polku);

    int tallennettaviaLiitteita() const;

    void nayta(int indeksi);
    int naytettavaIndeksi() const { return naytettavaIndeksi_; }
    bool tallennetaanko() const;

    QPdfDocument* pdfDocument() { return pdfDoc_;}
    QByteArray *sisalto();

    void liitteenTilaVaihtui(Liite::LiiteTila uusiTila);
    void ocr(const QVariantMap& data);

protected:
    void valimuistiLiite(int liiteId);
    void naytaKayttajalle();

    void tarkastaKaikkiLiitteet();

signals:
    int valittuVaihtui(int indeksi);
    void naytaPdf();
    void naytaSisalto();
    void kaikkiLiitteetHaettu();
    void liitteetTallennettu();

    void liitettaTallennetaan(bool tallennetaanko);

private:
    QList<Liite*> liitteet_;
    QBuffer* puskuri_;
    QPdfDocument* pdfDoc_;

    bool interaktiivinen_ = false;
    int naytettavaIndeksi_ = -1;

    bool tallennetaanko_ = false;
};

#endif // LIITTEETMODEL_H
