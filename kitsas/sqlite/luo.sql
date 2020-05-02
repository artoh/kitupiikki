CREATE TABLE Asetus
(
	avain varchar(128) PRIMARY KEY NOT NULL,
	arvo text,
	muokattu timestamp
);

CREATE TABLE Tili
(
	numero integer PRIMARY KEY NOT NULL,
	tyyppi varchar(10) NOT NULL,
	iban VARCHAR(32),
	json text,
	muokattu TIMESTAMP
);

CREATE TABLE Otsikko
(
	numero integer NOT NULL,
	taso integer NOT NULL,
	json text,
	muokattu TIMESTAMP,
	PRIMARY KEY (numero,taso)
);

CREATE TABLE Tilikausi
(
	alkaa date PRIMARY KEY NOT NULL,
	loppuu date UNIQUE NOT NULL,
	json text
);

CREATE TABLE Kohdennus
(
	id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	tyyppi INTEGER NOT NULL,
	kuuluu INTEGER REFERENCES Kohdennus(id) ON DELETE RESTRICT,
	json text,
	CHECK (tyyppi IN (0,1,2,3))
);

INSERT INTO Kohdennus (id, tyyppi, json ) VALUES
( 0, 0, '{"nimi":{"fi":"Yleinen","se":"Allmän", "en":"General"}}' );

CREATE TABLE Budjetti
(
	tilikausi DATE REFERENCES Tilikausi(alkaa),
	kohdennus INTEGER REFERENCES Kohdennus(id) ON DELETE CASCADE,
	tili INTEGER REFERENCES Tili(numero) ON DELETE CASCADE,
	sentti BIGINT,
	PRIMARY KEY (tilikausi, kohdennus, tili)
);

CREATE TABLE Kumppani
(
	id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	nimi VARCHAR(255),
	alvtunnus VARCHAR(20),
	json text
);

CREATE INDEX kumppani_nimi_index ON Kumppani(nimi);

CREATE TABLE KumppaniIban
(
	iban VARCHAR(30) PRIMARY KEY NOT NULL,
	kumppani INTEGER REFERENCES Kumppani(id) ON DELETE CASCADE
);

INSERT INTO Kumppani(nimi,alvtunnus,json)
	VALUES ('Verohallinto','FI02454583','{"osoite":"PL 325","postinumero":"00510","kaupunki":"VERO"}');
INSERT INTO KumppaniIban(iban,kumppani)	VALUES
	('FI6416603000117625',1),
	('FI5689199710000724',1),
	('FI3550000120253504',1);


CREATE TABLE Ryhma
(
	id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	nimi VARCHAR(255),
	json TEXT
);

CREATE TABLE KumppaniRyhmassa
(
	kumppani INTEGER REFERENCES Kumppani(id) ON DELETE CASCADE,
	ryhma INTEGER REFERENCES Ryhma(id) ON DELETE CASCADE,
	PRIMARY KEY(kumppani,ryhma)
);

CREATE TABLE Tosite
(
	id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	pvm date,
	tyyppi integer,
	tila integer DEFAULT 100,
	tunniste integer,
	sarja VARCHAR(10),
	otsikko TEXT,
	kumppani integer REFERENCES Kumppani(id),
	laskupvm DATE,
	erapvm DATE,
	viite varchar(64),
	json text
);

CREATE INDEX tosite_pvm ON Tosite (pvm);
CREATE INDEX tosite_tyyppi ON Tosite (tyyppi);
CREATE INDEX tosite_tila ON Tosite (tila);

CREATE TABLE Tositeloki
(
	id integer PRIMARY KEY AUTOINCREMENT NOT NULL,
	tosite integer REFERENCES Tosite(id),
	aika timestamp DEFAULT current_timestamp,
	data jsonb,
	userid integer,
	tila integer
);

CREATE INDEX tositeloki_tosite ON Tositeloki (tosite);

CREATE TABLE Vienti
(
	id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	rivi integer NOT NULL,
	tosite integer REFERENCES Tosite(id) ON DELETE CASCADE,
	tyyppi integer DEFAULT (0),
	pvm date,
	tili integer REFERENCES Tili(numero) ON DELETE RESTRICT,
	kohdennus integer DEFAULT(0) REFERENCES Kohdennus(id) ON DELETE RESTRICT,
	selite text,
	debetsnt BIGINT,
	kreditsnt BIGINT,
	eraid integer,
	alvprosentti numeric(5,2),
	alvkoodi integer,
	kumppani integer REFERENCES Kumppani(id),
	jaksoalkaa DATE,
	jaksoloppuu DATE,
	arkistotunnus VARCHAR(32),
	json text,
	CHECK (debetsnt = 0 OR kreditsnt = 0)
);

CREATE TABLE Merkkaus
(
	vienti INTEGER REFERENCES Vienti(id) ON DELETE CASCADE,
	kohdennus INTEGER REFERENCES Kohdennus(id) ON DELETE CASCADE,
	PRIMARY KEY (vienti, kohdennus)
);

CREATE INDEX vienti_tosite ON Vienti (tosite);
CREATE INDEX vienti_pvm ON Vienti (pvm);
CREATE INDEX vienti_tili ON Vienti (tili);
CREATE INDEX vienti_kohdennus ON Vienti (kohdennus);

CREATE TABLE Liite
(
	id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	tosite integer REFERENCES Tosite (id) ON DELETE CASCADE,
	nimi text,
	roolinimi varchar(16),
	tyyppi text,
	sha text,
	data bytea,
	luotu timestamp DEFAULT current_timestamp,
	json text,
	UNIQUE(tosite,roolinimi)
);

CREATE INDEX liite_tosite ON Liite (tosite);

CREATE TABLE Tuote
(
	id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	nimike VARCHAR(255),
	json text
);

CREATE TABLE Rivi
(
	tosite integer REFERENCES Tosite(id) ON DELETE CASCADE,
	rivi integer,
	tuote integer,
	myyntikpl real,
	ostokpl real,
	ahinta real DEFAULT(0.0),
	json text,
	PRIMARY KEY(tosite, rivi)
);

CREATE INDEX rivi_tosite ON Rivi (tosite);


CREATE TABLE Vakioviite
(
	viite integer PRIMARY KEY NOT NULL,
	tili INTEGER REFERENCES Tili(numero) ON DELETE CASCADE,
	kohdennus INTEGER REFERENCES Kohdennus(id) ON DELETE CASCADE,
	otsikko TEXT,
	alkaen DATE,
	paattyen DATE,
	json TEXT
);
