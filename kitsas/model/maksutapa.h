#ifndef MAKSUTAPA_H
#define MAKSUTAPA_H

#include <QString>
#include <QVariantMap>
#include "kieli/monikielinen.h"

class Maksutapa : public Monikielinen
{
public:
    Maksutapa();
    Maksutapa(const QVariantMap& map);
    
    QString nimi(const QString& kieli = "") const;
    int tili() const;
    bool uusiEra() const;
    QString kuva() const;
    QString muotoehto() const;
    
    QVariantMap toMap() const;

    void asetaTili(const int tili);
    void asetaUusiEra(bool uusiEra);
    void asetaKuva(const QString& kuva);
    
protected:
        
    
};

#endif // MAKSUTAPA_H
