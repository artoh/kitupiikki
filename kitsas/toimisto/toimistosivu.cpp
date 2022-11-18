#include "toimistosivu.h"

#include "ui_toimisto.h"


ToimistoSivu::ToimistoSivu(QWidget *parent) :
    KitupiikkiSivu(parent),
    ui(new Ui::Toimisto())
{
    ui->setupUi(this);
}

ToimistoSivu::~ToimistoSivu()
{
    delete ui;
}
