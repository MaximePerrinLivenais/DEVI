#!/usr/bin/env python3

# import re
import regex as re

# import regex as re
import json
import collections
import csv
import argparse
import concurrent.futures
import enchant
import enchant.tokenize

from pprint import pprint
from operator import attrgetter

import logging
from logging import debug, info, warning, error

logging.basicConfig(level=logging.DEBUG, format="%(levelname)s: %(message)s")


def red(s, colored=True):
    if colored:
        return "\x1b[31m{}\x1b[0m".format(s)
    return s


def green(s, colored=True):
    if colored:
        return "\x1b[32m{}\x1b[0m".format(s)
    return s


def white(s, colored=True):
    return s


def repr_entry(entry, colored=False):
    lines = []

    def print_named_iterable(name):
        iterable = entry[name]
        if len(iterable) == 1:
            lines.append("{:>20}: {}".format(name, green(list(iterable)[0], colored)))
        elif isinstance(iterable, str):
            lines.append("{:>20}: {}".format(name, green(iterable, colored)))
        elif len(iterable) > 1:
            lines.append("{:>20}:".format(name))
            for item in iterable:
                lines.append("{}{}".format(" " * 22, green(item, colored)))

    print_named_iterable("name")
    print_named_iterable("street_number")
    print_named_iterable("street")
    if len(entry["name"]) == 0 or len(entry["street"]) == 0:
        lines.append("{:>20}: {}".format("raw", red(repr(entry["raw"]), colored)))
    else:
        lines.append("{:>20}: {}".format("raw", green(repr(entry["raw"]), colored)))
    lines.append("")
    return "\n".join(lines)


def print_entry(entry, colored=True):
    # print(repr_entry(entry, colored))
    # if len(entry["name"]) == 0 or len(entry["street"]) == 0:
    # pprint(entry)
    # print("{name}|{street_number}|{street}".format(**entry))
    s = "{grade:<6} {name}, {street}, {street_number}.".format(**entry)
    print(entry)
    if entry["perfect_match"]:
        print(green(s, colored))
    else:
        if entry["grade"] == -1:
            print(red(s, colored))
        else:
            print(white(s, colored))
    # print()


def preprocess_line(line):
    line = re.sub("@NOTE ", "", line)

    line = " ".join(line.split("|"))
    line = " ".join(re.split("  +", line))

    return line


def process_name(raw):
    raw = re.sub("@NOTE ", "", raw)
    m = re.search(r"^(?P<name>.*?)((,)|(\. ))", raw)
    if m:
        return m.group("name")
    return raw


dictionary = enchant.Dict("fr")


def join_lines(lines, street_words):
    if len(lines) < 2:
        return lines

    tokenizer = enchant.tokenize.basic_tokenize
    i = 0
    while i < len(lines) - 1:
        word = list(tokenizer(lines[i]))[-1][0]
        word += list(tokenizer(lines[i + 1]))[0][0]
        if dictionary.check(word) or word.lower() in street_words:
            lines[i + 1] = lines[i] + lines[i + 1]
            del lines[i]
        else:
            i += 1
    return lines


def is_street_number(v):
    return True


def is_street(v):
    if len(list(enchant.tokenize.basic_tokenize(v))) > 10:
        return False
    return True


class MyJSONEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, set):
            return list(o)
        return json.JSONEncoder.default(self, o)


def compile_regex(args):
    street_type_regex = set()
    with open(args.streets_type, "r") as fh:
        street_type_raw = fh.read()
        for t in re.split(";", street_type_raw):
            t = t.strip()
            if len(t) > 0:
                t = t.replace(".", "\.")
                street_type_regex.add("({})".format(t))
                street_type_regex.add("({})".format(t.replace("\.", "")))
    street_type_regex = "(" + "|".join(street_type_regex) + ")"

    street_name_regex = set()
    with open(args.streets_name, "r") as fh:
        street_name_raw = fh.read()
        for n in street_name_raw.split("\n"):
            n = " ".join(n.strip().lower().split(" ")[1:])
            n = n.replace(".", "\.")
            street_name_regex.add("({})".format(n))
    street_name_regex = "(" + "|".join(street_name_regex) + ")"

    pre_street_res = []
    pre_street_res.append([0, r", ?"])
    pre_street_res.append([1, r" ?"])
    pre_street_res.append([2, r"^.*?, "])
    pre_street_res.append([3, r"^.*?\. "])
    pre_street_res.append([4, r"^.*?,"])
    pre_street_res.append([5, r"^.*?\."])
    pre_street_res.append([6, r" "])

    street_name_res = []
    street_name_res.append(
        [0, r"(?P<street>" + street_type_regex + " " + street_name_regex + ")"]
    )
    street_name_res.append(
        [10, r"(?P<street>" + street_type_regex + " " + "[^,;()]+?" + ")"]
    )
    street_name_res.append([20, r"(?P<street>" + "[^,;()]+?" + ")"])

    post_street_res = []
    post_street_res.append([0, r", "])
    post_street_res.append([1, r","])
    post_street_res.append([1, r"[;.] "])
    post_street_res.append([2, r"[;.]"])
    post_street_res.append([4, r" "])

    street_number_res = []
    street_number_res.append(
        [
            0,
            r"(?P<street_number>\d+(?: (?:bis)|(?:ter))?(?: et \d+(?: (?:bis)|(?:ter))?)?)(?: \([^\)]*\))?",
        ]
    )

    post_street_number_res = []
    post_street_number_res.append([0, r"[.,;]"])
    post_street_number_res.append([100, r""])

    regex = []
    for pre_street_re in pre_street_res:
        for street_name_re in street_name_res:
            for post_street_re in post_street_res:
                for street_number_re in street_number_res:
                    for post_street_number_re in post_street_number_res:
                        grade = (
                            pre_street_re[0]
                            + street_name_re[0]
                            + post_street_re[0]
                            + street_number_re[0]
                            + post_street_number_re[0]
                        )
                        composed_re = (
                            pre_street_re[1]
                            + street_name_re[1]
                            + post_street_re[1]
                            + street_number_re[1]
                            + post_street_number_re[1]
                        )
                        regex.append([grade, composed_re])

    regex_compiled = []
    with concurrent.futures.ProcessPoolExecutor(max_workers=12) as executor:
        future_to_re = {executor.submit(re.compile, x[1], re.I): x for x in regex}
        for future in concurrent.futures.as_completed(future_to_re):
            x = future_to_re[future]
            regex_compiled.append([x[0], future.result()])
    regex = regex_compiled
    regex.append([-1, re.compile(r"^(?P<name>[^\(]*?(\([^\)]*\))?.*?)((,)|(\. ))")])
    regex = sorted(regex, key=lambda x: x[0])
    print("{} regex initialized".format(len(regex)))

    return regex


def load_street_words(args):
    street_words = set()
    tokenizer = enchant.tokenize.basic_tokenize
    with open(args.streets_name, "r") as fh:
        street_name_raw = fh.read()
        for word, size in tokenizer(street_name_raw):
            street_words.add(word.lower())
    return street_words


def process_entry(raw_entry, regex, street_words):
    entry = {
        "lines": set(),
        "name": "",
        "street": "",
        "street_number": "",
        "grade": -1,
        "perfect_match": False,
    }

    entry["raw"] = raw_entry

    split_entry = entry["raw"].split("-|")
    join_lines(split_entry, street_words)
    lines_count = 2 ** (len(split_entry) - 1)

    # when there is a leading '-' on a line we duplicate the line
    # the result is 2**n lines with n the number of leading '-'
    for i in range(lines_count):
        line = ""
        for j, part in enumerate(split_entry):
            if i & 2 ** j > 0:
                line += part + "-"
            else:
                line += part
        entry["lines"].add(preprocess_line(line))

    # search for street and street number
    for line in entry["lines"]:
        # print(line)
        removed_name = False
        for i, r in regex:
            if not removed_name and len(entry["name"]) > 0:
                line = line.replace(next(iter(entry["name"])), "")
                removed_name = True
            m = r.search(line)
            if m:
                for k, v in m.groupdict().items():
                    if k == "street_number" and not is_street_number(v):
                        continue
                    if k == "street" and not is_street(v):
                        continue

                    if len(entry[k]) == 0:
                        # print(r)
                        entry[k] = "{}".format(v)
                        entry["grade"] = max(entry["grade"], i)
            if len(entry["street"]) > 0 and len(entry["street_number"]) > 0:
                if entry["grade"] == 0:
                    entry["perfect_match"] = True
                break
    return entry


def process_file(json_file, args):
    info("Parsing {}".format(json_file))
    regex = compile_regex(args)
    street_words = load_street_words(args)

    with open(json_file, "r") as fh:
        input_json = json.load(fh)

    process_node(input_json, regex, street_words)

    return input_json


def process_node(node, regex, street_words):
    if node["type"] != "ENTRY":
        if "children" in node:
            for child in node["children"]:
                process_node(child, regex, street_words)

    if "children" not in node:
        return

    # process an entry node
    raw_entry = [c["text"].strip() for c in node["children"] if c["type"] == "LINE"]
    raw_entry = "|".join(raw_entry)
    node["parsed"] = process_entry(raw_entry, regex, street_words)
    # pprint(node["parsed"])
    print_entry(node["parsed"])
    # exit()


def entry_to_dict(entry):
    m = {}
    for k, v in entry.items():
        if isinstance(v, str):
            m[k] = v
        elif isinstance(v, collections.Iterable):
            m[k] = next(iter(v))
        else:
            m[k] = v
    return m


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("json", nargs="+", help="Parse soduco json files")
    parser.add_argument("-q", "--quiet", action="store_false")
    parser.add_argument("-j", "--export-json", metavar="file")
    parser.add_argument("-t", "--export-text", metavar="file")
    parser.add_argument("-c", "--export-csv", metavar="file")
    parser.add_argument(
        "--dico-nom-voie", dest="streets_name", default="./paris_road_name.csv"
    )
    parser.add_argument(
        "--dico-type-voie", dest="streets_type", default="./dico_type_voie"
    )

    args = parser.parse_args()
    # regex.append(re.compile(r"^(.*?), (?P<street>[^,]+)[;., ]"))

    entries = []
    for json_file in args.json:
        entries.append(process_file(json_file, args))
    # with concurrent.futures.ProcessPoolExecutor() as executor:
    #     futures = [executor.submit(process_file, txt_file, args)
    #                for txt_file in args.txts]
    #     for future in concurrent.futures.as_completed(futures):
    #         entries += future.result()
    if args.export_text:
        with open(args.export_text, "w") as fh:
            fh.write(
                "\n".join((print_entry(entry, colored=False) for entry in entries))
            )
    if args.export_json:
        with open(args.export_json, "w") as fh:
            fh.write(MyJSONEncoder(sort_keys=True, indent=4).encode(entries))
    if args.export_csv:
        with open(args.export_csv, "w") as fh:
            fieldnames = [
                "entry_number",
                "grade",
                "perfect_match",
                "lines",
                "name",
                "street",
                "street_number",
                "raw",
            ]
            writer = csv.DictWriter(fh, fieldnames=fieldnames)
            writer.writeheader()
            for serie in entries:
                for entry in serie:
                    writer.writerow(entry_to_dict(entry))


if __name__ == "__main__":
    main()
