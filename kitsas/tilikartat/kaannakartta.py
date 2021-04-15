import json
import sys

asetukset = {}

with open(sys.argv[1] + "/tilikartta.json") as f:
    t = json.load(f)
    asetukset.update(t)

with open(sys.argv[1] + "/asetukset.json") as f:
    t = json.load(f)
    asetukset.update(t)

with open(sys.argv[1] + "/raportit.json") as f:
    t = json.load(f)
    asetukset.update(t)

kieli = ""
pohja = ""
with open(sys.argv[1] + "/tilinpaatos.txt") as f:
    for line in f:
        if line.startswith("["):
            if(len(pohja) > 10):
                asetukset["tppohja/" + kieli] = pohja
                pohja = ""
            kieli = line[1:3]
        else:
            pohja = pohja + line
    asetukset["tppohja/" + kieli] = pohja


with open(sys.argv[1] + "/tilit.json") as f:
    tilit = json.load(f)

kartta = {
    "asetukset": asetukset,
    "tilit": tilit
}

with open(sys.argv[1] + ".kitsaskartta", "w") as out:
    json.dump(kartta, out, ensure_ascii=False)
