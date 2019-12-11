import os
import statistics
import pandas as pd

test_res = {}

with open("dump.dat", "r") as f:
    rline = f.readline();
    while(rline):
        if rline.strip().startswith("TEST RUN"):
            datasize = f.readline().strip().split(" ")[3]
            testtype = f.readline()
            placetype = f.readline().strip().split(" ")[2]
            ttype = testtype.strip() + " " + placetype.strip() + " " + datasize.strip()
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
        f.write(tname + "\n")
        for cname, cvalues in tdict.items():
            f.write(cname + ": " + format(statistics.mean(cvalues), ".2f") + "\n")
        f.write("\n")

dictlist = []
for tname, tdict in test_res.items():
    ttype = tname.split(" ")[0]
    ptype = tname.split(" ")[1]
    dsize = tname.split(" ")[2]
    tmpdict = {'Test': ttype, 'Placement': ptype, 'Size': int(dsize)}
    for cname, cvalues in tdict.items():
        tmpdict[cname] = float(format(statistics.mean(cvalues)*64/int(dsize), ".2f"))
    dictlist.append(tmpdict)

print("CSV save")
#print(dictlist)
df = pd.DataFrame(dictlist, columns = tmpdict.keys())

df.to_csv("./result.csv", index=False)

cyc_idx = 0
for i, c in enumerate(df.columns):
    if "Cycles" in c:
        cyc_idx = i
        break
test_list = [
    "PRODUCER_CONSUMER",
    "STORE_ON_MODIFIED",
    "STORE_ON_EXCLUSIVE",
    "STORE_ON_SHARED_OR_FORWARD",
    "STORE_ON_INVALID",
    "LOAD_FROM_MODIFIED",
    "LOAD_FROM_EXCLUSIVE",
    "LOAD_FROM_SHARED_OR_FORWARD",
    "LOAD_FROM_INVALID",
    "FLUSH",
    ]
place_list = ["GLOBAL", "SOCKET", "LOCAL"]

for t in test_list:
    joined_df = ""
    for pl in place_list:
        tmp = df[(df['Test'] == t) & (df['Placement'] == pl)]
        if isinstance(tmp, pd.DataFrame):
            res = tmp.iloc[:,[2, cyc_idx]]
            cyc_cols = {c:pl+c for c in list(res.columns) if ("Cycles" in c)}
            res = res.rename(columns=cyc_cols)
            if isinstance(joined_df, pd.DataFrame):
                joined_df = joined_df.merge(res, on='Size', how='left')

            else:
                joined_df = res


    if isinstance(joined_df, pd.DataFrame):
        #print(len(joined_df))
        if len(joined_df) != 0:
            os.system("mkdir -p csv")
            joined_df.to_csv("./csv/result_" + t + ".csv", index=False)
    #res.to_csv("./result_" + t + "_" + pl + ".csv", index=False)

print("Done")
