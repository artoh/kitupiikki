#ifndef LASKUTEKSTITMODEL_H
#define LASKUTEKSTITMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include "kieli/monikielinen.h"


class LaskuTekstitModel : public QAbstractListModel
{
    Q_OBJECT

public:
    LaskuTekstitModel(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;


    QString kuvaus(int rivi) const;
    QString otsikko(int rivi, const QString& kieli) const;
    QString sisalto(int rivi, const QString& kieli) const;

    // Lippujen kysymykset
    bool onkoOtsikkoa(int rivi) const;
    bool onkoKorostusta(int rivi) const;

    void asetaOtsikko(int rivi, const QString& kieli, const QString& otsikko);
    void asetaSisalto(int rivi, const QString& kieli, const QString& sisalto);

    bool muokattu();
    void nollaa();
    void tallenna();


protected:
    void lisaa(const QString& tunnus, const QString& nimi, const QString& kuvaus, bool otsikot = false, bool korostus = false);

private:      
    class LaskuTeksti {
    public:
        enum {
            OTSIKKO = 0b1,
            EMAILKOROSTUS = 0b10
        };

        LaskuTeksti();
        LaskuTeksti(const QString& tunnus, const QString& nimi, const QString& kuvaus, int liput);

        void nollaa();
        void tallenna();

        QString tunnus() const { return tunnus_;};
        QString nimi() const { return nimi_;}
        QString kuvaus() const { return kuvaus_;};
        int liput() const { return liput_;};
        QString otsikko(const QString& kieli) const {return otsikko_muokattu_.kaannos(kieli);}
        QString sisalto(const QString& kieli) const {return sisalto_muokattu_.kaannos(kieli);};
        void asetaOtsikko(const QString& kieli, const QString& otsikko) { otsikko_muokattu_.aseta(otsikko, kieli); };
        void asetaSisalto(const QString& kieli, const QString& sisalto) { sisalto_muokattu_.aseta(sisalto, kieli);};
        bool muokattu() const { return otsikko_ladattu_.toString() != otsikko_muokattu_.toString()
                    || sisalto_ladattu_.toString() != sisalto_muokattu_.toString();}

    protected:
        QString tunnus_;
        QString nimi_;
        QString kuvaus_;
        int liput_;
        Monikielinen otsikko_ladattu_;
        Monikielinen otsikko_muokattu_;
        Monikielinen sisalto_ladattu_;
        Monikielinen sisalto_muokattu_;
    };

    QVector<LaskuTeksti> tekstit_;


};

#endif // LASKUTEKSTITMODEL_H
