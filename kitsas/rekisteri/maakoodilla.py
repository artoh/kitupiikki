import csv, json
taulu = dict()
with open("maat.csv", mode="r") as csv_file :
    csv_reader = csv.reader( csv_file, delimiter=";")
    for row in csv_reader:
        print( row[2], row[1])
        taulu[ row[2] ] = row[1]
json_object = json.dumps(taulu, indent=2)
print(json_object)
