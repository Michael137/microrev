import os
import statistics
import pandas as pd
import math

test_res = {}

with open("dump.dat", "r") as f:
    rline = f.readline();
    while(rline):
        if rline.strip().startswith("~~"):
            datasize = rline.strip().split("~~")[1]
            ttype = str(datasize)
            if ttype in test_res.keys():
                res_dict = test_res[ttype]
            else:
                res_dict = {}
            rline = f.readline()
            while rline and rline.strip() != "":
                counter_name = rline.strip().split(" ")[0][:-1]
                counter_value = int(float(rline.strip().split(" ")[1]))
                if counter_name in res_dict.keys():
                    res_dict[counter_name].append(counter_value)
                else:
                    res_dict[counter_name] = [counter_value]
                rline = f.readline()
            test_res[ttype] = res_dict
            if not rline:
                break
        rline = f.readline()

with open("result.dat", "w") as f:
    for tname, tdict in test_res.items():
        dsize = tname
        f.write(tname + "\n")
        for cname, cvalues in tdict.items():
            if "Cycle" in cname or "CYC" in cname:
                f.write(cname + ": " + format(statistics.mean(cvalues)/int(dsize), ".2f") + "\n")
            else:
                f.write(cname + ": " + format(statistics.mean(cvalues), ".2f") + "\n")
        f.write("\n")

dictlist = []
for tname, tdict in test_res.items():
    dsize = tname
    tmpdict = {'Size': int(dsize)}
    for cname, cvalues in tdict.items():
        if "Cycle" in cname or "CYC" in cname:
            tmpdict[cname] = float(format(statistics.mean(cvalues)/int(dsize), ".2f"))
        else:
            tmpdict[cname] = float(format(statistics.mean(cvalues), ".2f"))
    dictlist.append(tmpdict)

print("CSV save")
#print(dictlist)
df = pd.DataFrame(dictlist, columns = tmpdict.keys())

df.to_csv("./result.csv", index=False)

print("Done")
