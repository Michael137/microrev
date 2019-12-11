import os
import statistics
import pandas as pd

df=[0,0]
test_res = {}

lat_test_list = [
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

lat_place_list = ["GLOBAL", "SOCKET", "LOCAL"]

easy_tname = [
    "PC",
    "SM",
    "SE",
    "SS",
    "SI",
    "LM",
    "LE",
    "LS",
    "LI",
    "F",
]
easy_pname = ["G", "S", "L"]
tdict = {value:lat_test_list[key] for (key, value) in enumerate(easy_tname)}
pdict = {value:lat_place_list[key] for (key, value) in enumerate(easy_pname)}
def read_dfs():
    df[0] = pd.read_csv("result1.csv")
    df[1] = pd.read_csv("result2.csv")
    print(df[0])
    print(df[1])


def get_lat_abv(dfs, s):
    if s[3] == "1":
        df = dfs[0]
    else:
        df = dfs[1]
    print(tdict, pdict)
    print(s[:2], s[2])
    print(tdict[s[:2]], pdict[s[2]])
    return df[(df["Test"] == tdict[s[:2]]) & (df["Placement"] == pdict[s[2]])]["Cycles"].values[0]

def calc_lat(df, s_list, cnt_list):
    lat_list = []
    for i, s in enumerate(s_list):
        lat_list.append(get_lat_abv(df, s) * cnt_list[i])
    return lat_list

def get_ping_pong_lat(df, pl):
    trans_list = [
            "LML2", "LML1", "SS"+pl+"1", "SML1" , "SM"+pl+"3", "SM"+pl+"3",
            "SM"+pl+"3", "SM"+pl+"3", "SML1", "SML2", "LML1", "LM"+pl+"3"
                ]
    cnt_list = [1,7,1,7,1,1,1,1,7,1,7,1]
    lat_list = calc_lat(df, trans_list, cnt_list)

def get_one_way_lat(df, pl):
    trans_list = [
            "SI", "SI", # Producer
            "LM", "LM" # Consumer
                ]
    s_list = [s + pl for s in trans_list]
    lat_list = calc_lat(df, s_list)

if __name__ == "__main__":
    read_dfs()
    print(get_lat_abv(df, "LML1"))
