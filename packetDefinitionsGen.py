#!/usr/bin/env python3
from bs4 import BeautifulSoup
import requests

#Python script for generating packetDefinitions.h from the wiki

if __name__ == "__main__":
    url = "https://wiki.vg/Protocol"
    page = requests.get(url).content
    soup = BeautifulSoup(page, "html.parser")
    main = soup.body.find(id="content").find(id='bodyContent').find(id='mw-content-text').div
    result = {}
    notice = main.find("a", href="/Protocol_version_numbers", title="Protocol version numbers").string
    packets = main.find_all("h4")
    for pack in packets:
        name = pack.span["id"]
        if(name != None):
            link = url + '#' + name
            name = name.replace(" ", "_").upper().replace("(", "").replace(")", "")
            table = pack.find_next_sibling("table", class_="wikitable")
            if table != None:
                cells = table.tbody.find_all("tr")[1].find_all("td")
                num = cells[0].string[:-1]
                state = cells[1].string[:-1]
                bound = cells[2].string[:-1]
                if state not in result.keys():
                    result[state] = []
                result[state].append([name, num, bound, state.lower(), link])
    with open("packetDefinitions.h", "w") as f:
        f.write("//Created for %s\n" % notice)
        f.write("#ifndef PACKET_DEFINITIONS\n#define PACKET_DEFINITIONS")
        for key, val in result.items():
            f.write("\n//%s packets\n\n" % key)
            for item in val:
                f.write("#define %s %s //Bound to %s during %s. %s\n" % tuple(item))
        f.write("\n#endif\n")

