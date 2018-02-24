ALTER TABLE vienti ADD COLUMN viite VARCHAR(60);
ALTER TABLE vienti ADD COLUMN iban VARCHAR(60);
ALTER TABLE vienti ADD COLUMN erapvm DATE;
ALTER TABLE vienti ADD COLUMN arkistotunnus VARCHAR(60);

CREATE INDEX vienti_ibanviite_index ON vienti(iban,viite);
CREATE INDEX vienti_arkisto_index ON vienti(arkistotunnus);
