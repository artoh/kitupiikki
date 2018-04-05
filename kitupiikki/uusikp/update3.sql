ALTER TABLE kohdennus ADD COLUMN json TEXT;

CREATE TABLE merkkaus (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    vienti          INTEGER NOT NULL
                            REFERENCES vienti(id)  ON DELETE CASCADE
                                                   ON UPDATE CASCADE,
    kohdennus       INTEGER NOT NULL
                            REFERENCES kohdennus(id) ON DELETE CASCADE,
                                                     ON UPDATE CASCADE
);

CREATE INDEX merkkaus_vienti ON merkkaus(vienti);
CREATE INDEX merkkaus_kohdennus ON merkkaus(kohdennus);
