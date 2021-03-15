import json
import csv

d = {}

with open("tulkki.csv", newline="") as csvfile:
    reader = csv.DictReader(csvfile)
    for row in reader:
        c = {}
        if len(row["fi"]) > 0:
            c["fi"] = row["fi"]
        if len(row["sv"]) > 0:
            c["sv"] = row["sv"]
        if len(row["en"]) > 0:
            c["en"] = row["en"]
        d[row["key"]] = c

with open("ulos.json", "w") as jsonfile:
    json.dump(d, jsonfile, indent=4, ensure_ascii=False)
