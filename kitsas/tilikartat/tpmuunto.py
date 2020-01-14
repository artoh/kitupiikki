import json
 
with open("yhdistystp.txt") as file:
	data = file.read().splitlines()
	
ulos = {"tilinpaatospohja":{"fi" : data}}
koodattu = json.dumps(ulos, indent=4, ensure_ascii=False)
print(koodattu)
