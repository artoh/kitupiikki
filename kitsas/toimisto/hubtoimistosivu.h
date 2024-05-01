#ifndef HUBTOIMISTOSIVU_H
#define HUBTOIMISTOSIVU_H

#include <kitupiikkisivu.h>

class QWebEngineView;

class HubToimistoSivu : public KitupiikkiSivu
{
    Q_OBJECT
public:
    enum Jarjestelma { ADMIN, MAJAVA};

    HubToimistoSivu(QWidget *parent, Jarjestelma jarjestelma = ADMIN);

    void siirrySivulle() override;

    void alustaSivu();
    void naytaSivu();

    void alusta();

public slots:
    void naytaToimisto(const QString& id);

signals:
    void toimistoLinkki(const QString& id);

protected:
    QWebEngineView* view_ = nullptr;
    Jarjestelma jarjestelma_;
};

#endif // HUBTOIMISTOSIVU_H
