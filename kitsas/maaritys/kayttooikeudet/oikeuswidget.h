#ifndef OIKEUSWIDGET_H
#define OIKEUSWIDGET_H

#include <QWidget>
#include <QSet>

/**
 * @brief Oikeuksien valintaan käytettävä widgetti
 *
 * Tätä voi käyttää kantaluokkana, tai sitten tähän voi
 * asentaa haluamansa ui-luokan
 *
 */

class OikeusWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OikeusWidget(QWidget *parent = nullptr);

    void alusta();

    void aseta(const QStringList& oikeus);
    QSet<QString> oikeudet() const;
    QStringList oikeuslista() const;
    void nakyviin(const QString& oikeus, bool onkoNakyvissa);

    void kaikki();
    void eimitaan();

    bool onkoMuokattu();

signals:
    void muokattu(bool onko);

private:
    bool omistaja_;
    void tarkasta();
    void asetaKaytossaOlevat();

    QSet<QString> alussa_;

};

#endif // OIKEUSWIDGET_H
