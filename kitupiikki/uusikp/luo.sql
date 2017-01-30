CREATE TABLE asetus (
 avain VARCHAR(40) NOT NULL,
 arvo TEXT) ;

CREATE TABLE tili (
    nro    INTEGER      PRIMARY KEY ASC  NOT NULL UNIQUE,
    nimi   VARCHAR (60) NOT NULL,
    ohje   TEXT,
    tyyppi VARCHAR (10) NOT NULL,
    tila   INTEGER      DEFAULT (1),
    otsikkotaso INTEGER DEFAULT (0),
    json   TEXT
) ;

CREATE TABLE tiliotsikko (
    tilista       INTEGER      NOT NULL,
    tiliin                     NOT NULL,
    otsikko       VARCHAR (60) NOT NULL,
    tyyppi VARCHAR (10) DEFAULT O
);

CREATE TABLE tilikausi (
    alkaa  DATE PRIMARY KEY  UNIQUE  NOT NULL,
    loppuu DATE UNIQUE  NOT NULL,
    lukittu DATE ,
    json   TEXT
);

CREATE TABLE tositetyyppi (
    tunnus   VARCHAR(5)      PRIMARY KEY ASC,
    nimi VARCHAR (60) NOT NULL
);

INSERT INTO tositetyyppi ( tunnus,nimi) VALUES
  ('*','Järjestelmän tosite'),
  ('','Muu tosite' );

CREATE TABLE tosite (
    id        INTEGER      PRIMARY KEY AUTOINCREMENT,
    pvm       DATE         NOT NULL,
    otsikko   TEXT,
    kommentti TEXT,
    tunniste  INTEGER,
    tiliote   INTEGER      REFERENCES tili (nro) ON UPDATE CASCADE,
    tiedosto  VARCHAR (60),
    sha       TEXT,
    tyyppi    VARCHAR(6)         REFERENCES tositetyyppi (tunnus)
                                 DEFAULT (''),
    json      TEXT
);

CREATE TABLE kustannuspaikka (
    nro  INTEGER      PRIMARY KEY,
    kustannuspaikkanimi VARCHAR (60) NOT NULL
);

CREATE TABLE projekti (
    id     INTEGER      PRIMARY KEY AUTOINCREMENT,
    nimi   VARCHAR (60) NOT NULL,
    alkaa  DATE,
    loppuu              NOT NULL
);

CREATE TABLE vienti (
    id        INTEGER PRIMARY KEY AUTOINCREMENT,
    tosite          INTEGER NOT NULL
                            REFERENCES tosite (id),
    pvm       DATE    NOT NULL,
    tili            INTEGER REFERENCES tili (tilinro) ON DELETE RESTRICT
                                                      ON UPDATE RESTRICT,
    debetsnt        BIGINT,
    kreditsnt       BIGINT,
    selite          TEXT,
    kustannuspaikka         REFERENCES kustannuspaikka (nro) ON DELETE RESTRICT
                                                                            ON UPDATE CASCADE,
    projekti        INTEGER REFERENCES projekti (id) ON DELETE RESTRICT
                                                             ON UPDATE CASCADE,
    json            TEXT
);

CREATE TABLE liite (
    id       INTEGER      PRIMARY KEY AUTOINCREMENT,
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
           tosite.tunniste,
           tili.nimi,
           tili.tyyppi
      FROM vienti,
           tosite,
           tili
     WHERE vienti.tosite = tosite.id AND
           vienti.tili = tili.nro
