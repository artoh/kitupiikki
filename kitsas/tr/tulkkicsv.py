import json
import csv


with open("tulkki.json") as f:
    data = json.load(f)

    with open("tulkki.csv", "w", newline="") as csvfile:
        fieldnames = ["key", "fi", "sv", "en"]
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()

        for key in data:
            l = data[key]
            if "fi" not in l:
                l["fi"] = key
            if "sv" not in l:
                l["sv"] = ""
            if "en" not in l:
                l["en"] = ""

            writer.writerow(
                {"key": key, "fi": l["fi"], "sv": l["sv"], "en": l["en"]})
