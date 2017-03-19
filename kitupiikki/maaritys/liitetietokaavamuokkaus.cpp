/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include "liitetietokaavamuokkaus.h"

#include "db/kirjanpito.h"
#include <QVBoxLayout>
#include <QLabel>

LiitetietokaavaMuokkaus::LiitetietokaavaMuokkaus()
{
    editor = new QPlainTextEdit;

    QLabel* otsikko = new QLabel( tr("<b>Liitetietojen kaavan muokkaus</b>"));
    QVBoxLayout *leiska = new QVBoxLayout;
    leiska->addWidget(otsikko);
    leiska->addWidget(editor);
    setLayout(leiska);

    connect( editor, SIGNAL(textChanged()), this, SLOT(ilmoitaOnkoMuokattu()));
}

bool LiitetietokaavaMuokkaus::nollaa()
{
    editor->setPlainText( kp()->asetukset()->asetus("LiitetietoKaava"));
    return true;
}

bool LiitetietokaavaMuokkaus::tallenna()
{
    kp()->asetukset()->aseta("LiitetietoKaava", editor->toPlainText() );
    editor->document()->setModified(false);
    ilmoitaOnkoMuokattu();
    return true;
}

bool LiitetietokaavaMuokkaus::onkoMuokattu()
{
    return editor->document()->isModified();
}

void LiitetietokaavaMuokkaus::ilmoitaOnkoMuokattu()
{
    emit tallennaKaytossa(onkoMuokattu());
}
