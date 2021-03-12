import json

d = {}

for line in open("laskutekstit.txt"):
    vali = line.find(" ")
    if line.startswith("SV"):
        kieli = "sv"
        avain = line[2:vali]
    elif line.startswith("EN"):
        kieli = "en"
        avain = line[2:vali]
    else:
        kieli = "fi"
        avain = line[0:vali]

    arvo = line[vali + 1:].replace("\n", "")
    if avain in d:
        c = d[avain]
    else:
        c = {}
    c[kieli] = arvo
    d[avain] = c
    print(kieli, " ", avain, " ", arvo)

print json.dumps(d, ensure_ascii=False, indent=4)
