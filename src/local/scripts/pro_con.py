import os
import statistics
import pandas as pd

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

def get_lat_abv(df, s):
    return df[(df["Test"] == tdict[s[:-1]]) & (df["Placement"] == pdict[s[-1]])]["Cycles"].values[0]

def calc_lat(df, s_list):
    lat_list = []
    for s in s_list:
        lat_list.append(get_lat_abv(df, s))
    return lat_list

def get_ping_pong_lat(df, pl):
    trans_list = [
            "SS", "SI", # Producer
            "SS", "LM" # Consumer
                ]
    s_list = [s + pl for s in trans_list]
    lat_list = calc_lat(df, s_list)

def get_one_way_lat(df, pl):
    trans_list = [
            "SI", "SI", # Producer
            "LM", "LM" # Consumer
                ]
    s_list = [s + pl for s in trans_list]
    lat_list = calc_lat(df, s_list)
