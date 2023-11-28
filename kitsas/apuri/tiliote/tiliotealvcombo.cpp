#include "tiliotealvcombo.h"

TilioteAlvCombo::TilioteAlvCombo(QWidget *parent) :
    QComboBox(parent)
{
    alusta();
}

void TilioteAlvCombo::aseta(int prosentti)
{
    int indeksi = findData( prosentti );
    if( indeksi > -1)
        setCurrentIndex(indeksi);
}

int TilioteAlvCombo::prosentti() const
{
    return currentData().toInt();
}

void TilioteAlvCombo::alusta()
{
    addItem(QIcon(":/pic/tyhja.png"),tr("Veroton"), 0);
    addItem(QIcon(":/pic/netto.png"),"10%", 10);
    addItem(QIcon(":/pic/netto.png"),"14%", 14);
    addItem(QIcon(":/pic/netto.png"),"24%", 24);

}
