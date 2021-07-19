/*
   Copyright (C) 2019 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef ASIAKASTOIMITTAJAVALINTA_H
#define ASIAKASTOIMITTAJAVALINTA_H

#include <QWidget>
#include <QVariantMap>

class QLineEdit;
class QPushButton;
class AsiakasToimittajaListaModel;

class QComboBox;

class AsiakasToimittajaDlg;

class AsiakasToimittajaValinta : public QWidget
{
    Q_OBJECT
public:
    AsiakasToimittajaValinta(QWidget *parent = nullptr);

    int id() const;
    QString nimi() const;
    QStringList ibanit() const;
    QVariantMap map() const;

signals:
    void muuttui(const QVariantMap& map);

public slots:
    void clear();
    void tuonti(const QVariantMap &data);
    void valitse(const QVariantMap& map);
    void valitse(int kumppaniId);
    void lataa(QVariant* data);

private slots:
    void nimiMuuttui();
    void syotettyNimi();
    void muokkaa();
    void modelLadattu();

    void ibanLoytyi(const QVariantMap& tuontiData, QVariant* data);

    void tallennettu(const QVariantMap& map);

protected:
    void setId(int id);
    bool eventFilter(QObject *watched, QEvent *event) override;

    QComboBox* combo_;
    QPushButton* button_;

    AsiakasToimittajaListaModel* model_;
    AsiakasToimittajaDlg *dlg_ = nullptr;

    int lataa_ = 0;
    bool modelLataa_ = false;
    QVariantMap map_;

};

#endif // ASIAKASTOIMITTAJAVALINTA_H
