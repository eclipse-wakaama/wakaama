"""helper functions for integration tests"""
import re

def get_senml_json_record(parsed, urn, label):
    """helper function returning value of associated label

    example SenML input:

        [{'bn': '/1/', 'n': '0/0', 'v': 123}, {'n': '0/1', 'v': 300},
        {'n': '0/2', 'v': 0}, {'n': '0/3', 'v': 0}, {'n': '0/5', 'v': 0},
        {'n': '0/6', 'vb': False}, {'n': '0/7', 'vs': 'U'}]"""

    return next(item for item in parsed if item["n"] == urn)[label]


def get_json_record(parsed, urn, label):
    """helper function returning value of associated label

    example json input:

        {"bn":"/3/0/","e":[{"n":"0","sv":"Open Mobile Alliance"},
        {"n":"1","sv":"Lightweight M2M Client"},{"n":"2","sv":"345000123"},
        {"n":"3","sv":"1.0"},{"n":"6/0","v":1},{"n":"6/1","v":5},{"n":"7/0","v":3800},
        {"n":"7/1","v":5000},{"n":"8/0","v":125},{"n":"8/1","v":900},{"n":"9","v":100},
        {"n":"10","v":15},{"n":"11/0","v":0},{"n":"13","v":2989678747},
        {"n":"14","sv":"+01:00"},{"n":"15","sv":"Europe/Berlin"},{"n":"16","sv":"U"}]}"""

    parsed = parsed['e']
    return next(item for item in parsed if item["n"] == urn)[label]


def get_tlv_record(text, idx):
    """helper function returning value (string) of Resource Value ID

    example tlv input:

    {
        ID: 0 type: Resource Value
        {
            data (20 bytes):
            4F 70 65 6E  20 4D 6F 62  69 6C 65 20  41 6C 6C 69   Open Mobile Alli
            61 6E 63 65                                          ance
        }
    }

    { ..."""

    depth=0
    state = 0
    output = ""
    num = 0
    # 0 - search for "{"" or "}"
    # 1 - ID found search for data "(data: xx) bytes"
    # 2 - "data xx" found parse xx chars of data and then return
    lines = text[text.find("bytes received of type application/vnd.oma.lwm2m+tlv"):].split("\n")
    for line in lines:
        if state == 0:
            if not re.search(r"\w", line) and re.search(r"[{}]", line):
                if line.find("{") > 0:
                    depth+=1
                else:
                    depth-=1
            res = re.findall(r"ID: ([0-9]+) type:", line)
            if depth == 1 and res and res[0] == str(idx):
                state = 1
        elif state == 1:
            res = re.findall(r"data \(([0-9]+) bytes\):", line)
            if res:
                num = int(res[0])
                state = 2
        elif state == 2:
            chars = min(num, 16)
            output += line.rstrip("\r")[-1*chars:]
            num -= chars
            if num < 1:
                return output
    return None
