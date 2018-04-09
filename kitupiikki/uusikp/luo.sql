PRAGMA foreign_keys = ON;

CREATE TABLE asetus (
    avain   VARCHAR(40) NOT NULL,
    arvo    TEXT,
    muokattu DATETIME
) ;

CREATE TABLE tili (
    id     INTEGER      PRIMARY KEY AUTOINCREMENT,
    nro    INTEGER      NOT NULL,
    nimi   VARCHAR (60) NOT NULL,
    tyyppi VARCHAR (10) NOT NULL,
    tila   INTEGER      DEFAULT (1),
    ysiluku INTEGER    NOT NULL,
    json   TEXT,
    muokattu    DATETIME
) ;

CREATE INDEX tili_nro ON tili(nro);
CREATE INDEX tili_ysiluku ON tili(ysiluku);

CREATE TABLE tilikausi (
    alkaa  DATE PRIMARY KEY  UNIQUE  NOT NULL,
    loppuu DATE UNIQUE  NOT NULL,
    json   TEXT
);

CREATE TABLE tositelaji (
    id       INTEGER         PRIMARY KEY AUTOINCREMENT,
    tunnus   VARCHAR(5)      UNIQUE NOT NULL,
    nimi     VARCHAR (60)    NOT NULL,
    json     TEXT
);

INSERT INTO tositelaji ( id, tunnus,nimi) VALUES
  (0,'*','Järjestelmän tosite'),
  (1,'','Muu tosite' );

CREATE TABLE tosite (
    id        INTEGER      PRIMARY KEY AUTOINCREMENT,
    pvm       DATE,
    otsikko   TEXT,
    kommentti TEXT,
    tunniste  INTEGER,
    tiliote   INTEGER      REFERENCES tili (id) ON UPDATE CASCADE,
    laji      INTEGER         REFERENCES tositelaji (id)
                              DEFAULT (1),
    json      TEXT
);

CREATE INDEX tosite_pvm_index ON tosite(pvm);
CREATE INDEX tosite_tiliote_index on tosite(tiliote);
CREATE INDEX tosite_laji_index on tosite(laji);
CREATE INDEX tosite_tunniste_index on tosite(tunniste);


CREATE TABLE kohdennus (
    id     INTEGER      PRIMARY KEY AUTOINCREMENT,
    nimi   VARCHAR (60) NOT NULL,
    alkaa  DATE,
    loppuu DATE,
    tyyppi INTEGER  NOT NULL,
    json   TEXT
);

INSERT INTO kohdennus(id, nimi, tyyppi) VALUES(0,"Yleinen",0);


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

CREATE INDEX vienti_tosite_index ON vienti(tosite);
CREATE INDEX vienti_pvm_index ON vienti(pvm);
CREATE INDEX vienti_tili_index ON vienti(tili);
CREATE INDEX vienti_kodennus_index ON vienti(kohdennus);
CREATE INDEX vienti_taseera_index ON vienti(eraid);
CREATE INDEX vienti_ibanviite_index ON vienti(iban,viite);
CREATE INDEX vienti_arkisto_index ON vienti(arkistotunnus);

CREATE TABLE liite (
    id       INTEGER      PRIMARY KEY AUTOINCREMENT,
    liiteno  INTEGER      NOT NULL,
    tosite   INTEGER      REFERENCES tosite (id) ON DELETE RESTRICT
                                                 ON UPDATE RESTRICT,
    otsikko  TEXT,
    sha      TEXT         NOT NULL,
    peukku   BLOB
);

CREATE TABLE lasku (
    id          INTEGER        PRIMARY KEY,
    tosite      INTEGER REFERENCES tosite(id) ON DELETE RESTRICT
                                              ON UPDATE CASCADE,
    laskupvm    DATE,
    erapvm      DATE,
    summaSnt    BIGINT,
    avoinSnt    BIGINT,
    asiakas     VARCHAR(128),
    kirjausperuste INTEGER DEFAULT(0),
    json        TEXT

);

CREATE INDEX lasku_viite ON lasku(viite);
CREATE INDEX lasku_pvm ON lasku(laskupvm);
CREATE INDEX lasku_erapvm ON lasku(erapvm);
CREATE INDEX lasku_asiakas ON lasku(asiakas);
CREATE INDEX lasku_tosite ON lasku(tosite);

CREATE TABLE tuote (
    id              INTEGER     PRIMARY KEY AUTOINCREMENT,
    nimike          TEXT,
    yksikko         VARCHAR(16),
    hintaSnt        REAL,
    alvkoodi        INTEGER DEFAULT(0),
    alvprosentti    INTEGER DEFAULT(0),
    tili            INTEGER REFERENCES tili(id) ON DELETE RESTRICT
                                                ON UPDATE CASCADE,
    kohdennus       INTEGER DEFAULT(0)
                            REFERENCES kohdennus(id) ON DELETE RESTRICT
                                                ON UPDATE CASCADE
);

CREATE TABLE merkkaus (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    vienti          INTEGER NOT NULL
                            REFERENCES vienti(id)  ON DELETE CASCADE
                                                   ON UPDATE CASCADE,
    kohdennus       INTEGER NOT NULL
                            REFERENCES kohdennus(id) ON DELETE CASCADE
                                                     ON UPDATE CASCADE
);

CREATE INDEX merkkaus_vienti ON merkkaus(vienti);
CREATE INDEX merkkaus_kohdennus ON merkkaus(kohdennus);


CREATE VIEW vientivw AS
    SELECT vienti.id as vientiId,
           vienti.pvm as pvm,
           vienti.debetsnt as debetsnt,
           vienti.kreditsnt as kreditsnt,
           vienti.selite as selite,
           vienti.kohdennus as kohdennusId,
           vienti.eraid as eraid,
           kohdennus.nimi as kohdennus,
           tositelaji.tunnus as tositelaji,
           tosite.tunniste as tunniste,
           tosite.id as tositeId,
           tositelaji.id as tositelajiId,
           tili.nro as tilinro,
           tili.nimi as tilinimi,
           tili.tyyppi as tilityyppi
      FROM vienti,
           tosite,
           tili,
           tositelaji,
           kohdennus
     WHERE vienti.tosite = tosite.id AND
           vienti.tili = tili.id AND
           tosite.laji = tositelaji.id AND
           vienti.kohdennus = kohdennus.id
