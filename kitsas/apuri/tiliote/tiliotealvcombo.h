#ifndef TILIOTEALVCOMBO_H
#define TILIOTEALVCOMBO_H

#include "kitsas.h"
#include <QComboBox>

class TilioteAlvCombo : public QComboBox
{
    Q_OBJECT
public:
    TilioteAlvCombo(QWidget* parent = nullptr);

    void aseta(int koodi);
    int koodi() const;

    void alustaTulolle(const QDate &pvm);
    void alustaMenolle(const QDate &pvm);

protected:    
    void lisaa( AlvKoodi::Koodi koodi, int prosentti = 0, const QString& teksti = QString());
};

#endif // TILIOTEALVCOMBO_H
