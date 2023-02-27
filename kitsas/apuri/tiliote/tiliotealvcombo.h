#ifndef TILIOTEALVCOMBO_H
#define TILIOTEALVCOMBO_H

#include <QComboBox>

class TilioteAlvCombo : public QComboBox
{
    Q_OBJECT
public:
    TilioteAlvCombo(QWidget* parent = nullptr);

    void aseta(int prosentti);
    int prosentti() const;

protected:
    void alusta();
};

#endif // TILIOTEALVCOMBO_H
