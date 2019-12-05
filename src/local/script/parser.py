import os
import statistics

test_res = {}

with open("dump.dat", "r") as f:
    rline = f.readline();
    while(rline):
        if rline.strip().startswith("TEST RUN"):
            testtype = f.readline()
            placetype = f.readline().strip().split(" ")[2]
            ttype = testtype.strip() + " " + placetype.strip()
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

