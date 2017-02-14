CREATE TABLE asetus (
 avain VARCHAR(40) NOT NULL,
 arvo TEXT) ;

CREATE TABLE tili (
    id     INTEGER      PRIMARY KEY AUTOINCREMENT,
    nro    INTEGER      NOT NULL,
    nimi   VARCHAR (60) NOT NULL,
    tyyppi VARCHAR (10) NOT NULL,
    tila   INTEGER      DEFAULT (1),
    otsikkotaso INTEGER DEFAULT (0),
    ysiluku INTEGER    NOT NULL,
    json   TEXT
) ;


CREATE TABLE tilikausi (
    alkaa  DATE PRIMARY KEY  UNIQUE  NOT NULL,
    loppuu DATE UNIQUE  NOT NULL,
    json   TEXT
);

CREATE TABLE tositelaji (
    id       INTEGER         PRIMARY KEY AUTOINCREMENT,
    tunnus   VARCHAR(5)      UNIQUE NOT NULL,
    nimi VARCHAR (60) NOT NULL
);

INSERT INTO tositelaji ( id, tunnus,nimi) VALUES
  (0,'***','Järjestelmän tosite'),
  (1,'','Muu tosite' );

CREATE TABLE tosite (
    id        INTEGER      PRIMARY KEY AUTOINCREMENT,
    pvm       DATE         NOT NULL,
    otsikko   TEXT,
    kommentti TEXT,
    tunniste  INTEGER,
    tiliote   INTEGER      REFERENCES tili (id) ON UPDATE CASCADE,
    laji      INTEGER         REFERENCES tositelaji (id)
                                 DEFAULT (1),
    json      TEXT
);


CREATE TABLE kohdennus (
    id     INTEGER      PRIMARY KEY AUTOINCREMENT,
    nimi   VARCHAR (60) NOT NULL,
    alkaa  DATE,
    loppuu DATE,
    tyyppi INTEGER  NOT NULL
);

INSERT INTO kohdennus(id, nimi, tyyppi) VALUES(0,"Ei kohdenneta",0);


CREATE TABLE vienti (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    tosite          INTEGER NOT NULL
                            REFERENCES tosite (id),
    vientirivi      INTEGER NOT NULL,
    pvm             DATE    NOT NULL,
    tili            INTEGER REFERENCES tili (id) ON DELETE RESTRICT
                                                 ON UPDATE RESTRICT,
    debetsnt        BIGINT,
    kreditsnt       BIGINT,
    selite          TEXT,
    alvkoodi        INTEGER DEFAULT(0),
    alvprosentti    INTEGER DEFAULT(0),
    kustannuspaikka         REFERENCES kohdennus (id) ON DELETE RESTRICT
                                                      ON UPDATE CASCADE,
    projekti        INTEGER REFERENCES kohdennus (id) ON DELETE RESTRICT
                                                      ON UPDATE CASCADE,
    json            TEXT,
    luotu           DATETIME,
    muokattu        DATETIME
);

CREATE TABLE liite (
    id       INTEGER      PRIMARY KEY AUTOINCREMENT,
    liiteno  INTEGER      NOT NULL,
    tosite   INTEGER      REFERENCES tosite (id) ON DELETE RESTRICT
                                                 ON UPDATE RESTRICT,
    otsikko  TEXT,
    tiedosto VARCHAR (60) NOT NULL,
    sha      TEXT         NOT NULL
);

CREATE VIEW vientivw AS
    SELECT vienti.pvm,
           vienti.tili,
           vienti.debetsnt,
           vienti.kreditsnt,
           vienti.selite,
           vienti.kustannuspaikka,
           vienti.projekti,
           vienti.json,
           vienti.tosite,
           tositelaji.tunnus as tositelaji,
           tosite.tunniste,
           tili.nro,
           tili.nimi,
           tili.tyyppi
      FROM vienti,
           tosite,
           tili,
           tositelaji
     WHERE vienti.tosite = tosite.id AND
           vienti.tili = tili.id AND
           tosite.laji = tositelaji.id
