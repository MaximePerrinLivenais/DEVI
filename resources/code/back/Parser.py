import enchant
import enchant.tokenize
import regex as re
from itertools import product
import concurrent.futures

import logging
from logging import info, debug
logging.basicConfig(level=logging.DEBUG, format="%(levelname)s: %(message)s")


class Parser:
    '''
    Parser object to decode text entry
    '''


    def decode(self, text):
        '''
        return a pair (text, score) with the decoded entry
        '''

        if not self.regexps:
            logging.info("Setting up regexps (please wait...)");
            self.regexps = compute_regexps(self.street_types, self.street_names)

        entry = process_entry(text, self.regexps, self.street_tokens)
        return entry


    def __init__(self, streets_types_uri, streets_names_uri = None):
        self.street_names = None
        self.street_tokens = None
        self.street_types = None
        self.regexps = None


        if streets_names_uri:
            self.street_tokens = load_street_tokens(streets_names_uri)
            self.street_names = load_street_names(streets_names_uri)
        self.street_types = load_street_types(streets_types_uri)



def load_street_types(uri):
    out = set()
    with open(uri, "r") as fh:
        street_type_raw = fh.read()
        for t in re.split(";", street_type_raw):
            t = t.strip()
            if t:
                out.add(t)
    return out

def load_street_tokens(uri):
    street_words = set()
    tokenizer = enchant.tokenize.basic_tokenize
    with open(uri, "r") as fh:
        street_name_raw = fh.read()
        for word, size in tokenizer(street_name_raw):
            street_words.add(word.lower())
    return street_words


def load_street_names(uri):
    streets = set()
    with open(uri, "r") as fh:
        for street in fh:
            streets.add(street.lower().strip())
    return streets




def compute_regexps(street_types, street_names):
    street_type_regexp = street_types and r"\L<StreetTypes>"
    street_name_regexp = street_names and r"\L<StreetNames>"


    R1 = pre_street_regexps = [
        (0, r", ?"),
        (1, r" ?"),
        (2, r"^.*?, "),
        (3, r"^.*?\. "),
        (4, r"^.*?,"),
        (5, r"^.*?\."),
        (6, r" ")
    ]

    R2 = street_name_regexps = [
        (0, r"(?P<street>" + street_type_regexp + " " + street_name_regexp + ")"),
        (10, r"(?P<street>" + street_type_regexp + " " + "[^,;()]+?" + ")"),
        (20, r"(?P<street>" + "[^,;()]+?" + ")")
    ]

    R3 = post_street_regexps = [
        (0, r", "),
        (1, r","),
        (1, r"[;.] "),
        (2, r"[;.]"),
        (4, r" "),
    ]

    R4 = street_number_regexps = [
        (0, r"(?P<street_number>\d+(?: (?:bis)|(?:ter))?(?: et \d+(?: (?:bis)|(?:ter))?)?)(?: \([^\)]*\))?")
    ]

    R5 = post_street_number_regexps = [
        (0, r"[.,;]"),
        (100, r""),
    ]


    regexps = [
        (-1, r"^(?P<name>[^\(]*?(\([^\)]*\))?.*?)((,)|(\. ))") # Catch all regexp
    ]
    for x in product(R1, R2, R3, R4, R5):
        (a, pre_street_re), (b,street_name_re), (c,post_street_re), (d,street_number_re), (e,post_street_number_re) = x
        score = a + b + c + d + e
        all_re = pre_street_re + street_name_re + post_street_re + street_number_re + post_street_number_re
        regexps.append((score,all_re))




    executor = concurrent.futures.ThreadPoolExecutor(max_workers=12)
    regex_compiled = executor.map(lambda arg: (arg[0], re.compile(arg[1], re.I, StreetTypes = street_types, StreetNames = street_names,
                                                                  ignore_unused=True)), regexps)
    executor.shutdown(True) #wait
    regex_compiled = list(regex_compiled)
    regex_compiled.sort(key = lambda x: x[0])
    logging.info("{} regex initialized".format(len(regex_compiled)))
    return regex_compiled

def process_entry(raw_entry, regex, street_words):
    entry = {
        "raw" : None,
        "lines" : set(),
        "name": "",
        "street": "",
        "street_number": "",
        "grade": -1,
        "perfect_match": False,
    }

    entry["raw"] = raw_entry

    split_entry = entry["raw"].splitlines()
    #join_lines(split_entry, street_words)
    split_entry = [ " ".join(split_entry) ]
    lines_count = int(2 ** (len(split_entry) - 1))

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
        removed_name = False
        for score, r in regex:
            if not removed_name and len(entry["name"]) > 0:
                line = line.replace(next(iter(entry["name"])), "")
                removed_name = True
            m = r.search(line)
            if not m:
                continue
            groups = m.groupdict()
            street_number = groups.get("street_number")
            street = groups.get("street")
            name = groups.get("name")

            if name:                                entry["name"] = name
            if street_number:                       entry["street_number"] = street_number
            if street and is_street(street):        entry["street"] = street
            entry["grade"] = max(entry["grade"], score)

            if street and street_number and entry["grade"] == 0:
                entry["perfect_match"] = True
                break
    del entry["lines"] # Temp hack because of json writer
    return entry

def preprocess_line(line):
    line = re.sub("@NOTE ", "", line)
    line = " ".join(line.split("|"))
    line = " ".join(re.split("  +", line))

    return line

def is_street(v):
    if len(list(enchant.tokenize.basic_tokenize(v))) > 10:
        return False
    return True



def red(s, colored=True):
    if colored:
        return "\x1b[31m{}\x1b[0m".format(s)
    return s


def green(s, colored=True):
    if colored:
        return "\x1b[32m{}\x1b[0m".format(s)
    return s


def print_entry(entry, colored=True):
    s = "{grade:<6} <NAME>:{name}, <STREET>:{street}, <NUM>:{street_number}.".format(**entry)
    if entry["perfect_match"]:
        info(green(s, colored))
    elif entry["grade"] == -1:
        info(red(s, colored))
    else:
        info(s)

dictionary = enchant.Dict("fr")
def join_lines(lines, street_words):
    lines = list(filter(lambda x: len(x.strip()) > 0, lines))
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
