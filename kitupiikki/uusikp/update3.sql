PRAGMA foreign_keys = OFF;

ALTER TABLE vienti ADD COLUMN viite VARCHAR(60);
ALTER TABLE vienti ADD COLUMN iban VARCHAR(60);
ALTER TABLE vienti ADD COLUMN erapvm DATE;
ALTER TABLE vienti ADD COLUMN arkistotunnus VARCHAR(60);

CREATE INDEX vienti_ibanviite_index ON vienti(iban,viite);
CREATE INDEX vienti_arkisto_index ON vienti(arkistotunnus);

ALTER TABLE tosite ADD COLUMN luotu DATETIME;
ALTER TABLE tosite ADD COLUMN muokattu DATETIME;

ALTER TABLE kohdennus ADD COLUMN json TEXT;

ALTER TABLE liite ADD COLUMN data BLOB;
ALTER TABLE liite ADD COLUMN liitetty DATETIME;

CREATE INDEX liite_tosite_index ON liite(tosite);

ALTER TABLE vienti RENAME TO vienti_old;

CREATE TABLE vienti (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    tosite          INTEGER REFERENCES tosite (id),
    vientirivi      INTEGER NOT NULL,
    pvm             DATE,
    tili            INTEGER REFERENCES tili (id) ON DELETE RESTRICT
                                                 ON UPDATE RESTRICT,
    debetsnt        BIGINT,
    kreditsnt       BIGINT,
    selite          TEXT,
    alvkoodi        INTEGER DEFAULT(0),
    alvprosentti    INTEGER DEFAULT(0),
    kohdennus        INTEGER DEFAULT(0)
                             REFERENCES kohdennus (id) ON DELETE RESTRICT
                                                      ON UPDATE CASCADE,
    eraid           INTEGER REFERENCES vienti(id) ON DELETE SET NULL
                                                  ON UPDATE CASCADE,
    viite           VARCHAR(60),
    iban            VARCHAR(60),
    laskupvm        DATE,
    erapvm          DATE,
    arkistotunnus   VARCHAR(60),
    asiakas         VARCHAR(60),
    json            TEXT,
    luotu           DATETIME,
    muokattu        DATETIME
);


INSERT INTO vienti(id, tosite, vientirivi, pvm, tili, debetsnt, kreditsnt, selite,
                   alvkoodi, alvprosentti, kohdennus, eraid, viite, iban, erapvm, arkistotunnus,
                   json, luotu, muokattu)
SELECT id, tosite, vientirivi, pvm, tili, debetsnt, kreditsnt, selite, alvkoodi, alvprosentti,
       kohdennus, eraid, viite, iban, erapvm, arkistotunnus, json, luotu, muokattu
FROM vienti_old;

UPDATE vienti SET eraid=id WHERE eraid IS NULL;


PRAGMA foreign_keys = ON;
