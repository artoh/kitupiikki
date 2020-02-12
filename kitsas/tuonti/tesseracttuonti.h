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
#ifndef TESSERACTTUONTI_H
#define TESSERACTTUONTI_H

#include <QObject>


namespace Tuonti {

class TesserActTuonti : public QObject
{
    Q_OBJECT
public:
    explicit TesserActTuonti(QObject *parent = nullptr);

    void tuo(const QByteArray& data);

signals:
    void tuotu(const QVariantMap& map);

public slots:

protected slots:
    void kasittele(QVariant* data);

protected:
    QVariantMap analysoi(const QString& teksti);

protected:
};


}
#endif // TESSERACTTUONTI_H
