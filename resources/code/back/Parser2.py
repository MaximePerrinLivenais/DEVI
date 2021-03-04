import regex as re
import logging
import dataclasses
import csv
from FastSpellChecker import Dictionary
from dataclasses import dataclass
from unidecode import unidecode
from typing import List, Dict, Iterable



class Parser:
    '''
    Parser object to decode text entry
    '''


    def decode(self, text):
        '''
        return a pair (text, score) with the decoded entry
        '''
        entry = self._process_entry(text)
        if entry is None:
            entry = EntryParseResult()
        entry.raw = text
        if entry.street in self.street_fullnames_rev:
            entry.street = self.street_fullnames_rev.get(entry.street)
        elif entry.street in self.street_names_rev:
            entry.street = self.street_names_rev.get(entry.street)
        logging.debug("Detected entry: %s", entry)
        return dataclasses.asdict(entry)


    def __init__(self, street_names_uri):
        self.street_fullnames = None
        self.street_names = None
        self.street_fullnames_rev = {}
        self.street_names_rev = {}



        with open(street_names_uri) as fp:
            reader = csv.DictReader(fp, delimiter=';')
            street_fullnames = set()
            street_names = set()
            for row in reader:
                x = row["typo_min"]
                y = text_normalize(row["nomvoie"], force_ascii=True)
                street_names.add(y)
                if y not in self.street_names_rev or row["typvoie"] == "rue":
                    self.street_names_rev[y] = x

                y = text_normalize(row["typo"])
                street_fullnames.add(y)
                self.street_fullnames_rev[y] = x



        self.street_fullnames = Dictionary(street_fullnames)
        self.street_names = Dictionary(street_names)


    def _process_entry(self, text: str):
        '''
        '''
        # 1. Text normalization
        text = text_normalize(text, force_ascii = True, block = True, remove_abbrv = True)

        logging.debug(text)
        # 3. Split in fields
        fields = _split_fields(text)

        if len(fields) == 0:
            return None

        # 4. Parse fields
        name_field = fields[0]
        street_number_fields = [ _match_street_number(f) for f in fields ]
        street_name_fields = [ _match_street_name(f, self.street_names, self.street_fullnames) for f in fields ]

        logging.debug("Fields = {}".format(fields))
        logging.debug("Score StreetName = {}".format(street_name_fields))
        logging.debug("Score Numbers = {}".format(street_number_fields))

     
        if len(fields) >= 2:
            street_number_fields.append(("",0)) # case where we end with a street
            rstreet, rnum = max(zip(street_name_fields[1:], street_number_fields[2:]), key=lambda p: p[0][1]+p[1][1])
            num, score_num = rnum
            street, score_street = rstreet
            return EntryParseResult(name = name_field,
                                     street_number = num,
                                     street = street,
                                     score = 0.5 * (score_num + score_street))
                                    

        return None


class _bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


@dataclass
class EntryParseResult:
    name : str = ""
    street : str = ""
    street_number : str = ""
    raw : str = ""
    score : int = -1

    def __repr__(self):
        if self.score == -1:
            return _bcolors.FAIL + "Failure" +_bcolors.ENDC

        if self.score < 10: color = _bcolors.OKGREEN
        elif self.score < 50: color = _bcolors.OKBLUE
        else: color = _bcolors.WARNING

        return f"{color}##{self.score}##{_bcolors.ENDC} {_bcolors.BOLD}{self.name}{_bcolors.ENDC}, {self.street}, {_bcolors.UNDERLINE}{self.street_number}{_bcolors.ENDC}"


def _remove_linebreaks(text: str) -> str:
    '''
    Remove linebreaks from a text
    Fixme, use a dictionny to replace "-\n"
    '''
    text = text.replace("-\n", "-")
    text = text.replace("\n", " ")
    return text


__abbrv = {
    "av"      : "avenue",
    "barr"    : "barrière",
    "b"       : "boulevard",
    "boul"    : "boulevard",
    "boulev"  : "boulevard",
    "carref"  : "carrefour",
    "chem"    : "chemin",
    "ch"      : "chemin",
    "cloit"   : "cloitre",
    "faub"    : "faubourg",
    "fb"      : "faubourg",
    "germ"    : "germain",
    "imp"     : "impasse",
    "impas"   : "impasse",
    "pas"     : "passage",
    "pass"    : "passage",
    "p"       : "place",
    "pl"      : "place",
    "s"       : "saint",
    "st"      : "saint",
    "ste"     : "sainte",
    "nve"     : "neuve",
}


__abbrv_re1 = re.compile(r"(\([^)]+\))|\b(\L<abrv>)([.,;:-])", abrv = set(__abbrv.keys()), flags = re.IGNORECASE)
__abbrv_re2 = re.compile(r"(\([^)]+\))|(?:\b([[:upper:]][[:lower:]]{0,2})[.;:])")

def _remove_abbrv(text: str) -> str:
    '''
    Remove all abbreviations (not in parenthesis) from the text and returns a new string
    Two kinds of abbreviations are handled:
    1) Abbreviations of places/rues from a whitelist
    2) Abbreviations of names in the form "Ch." "M."

    (1) is replaced by the full name
    (2) remains but the punct is replaced by "\u2022"

    Eg.
    Lagaffe (G.), av. G. De Gaulle. 12. =>
    Lagaffe (G.), avenue G• De Gaulle. 12.
    '''
    text = __abbrv_re1.sub(lambda x: __abbrv[x.group(2).lower()] + ("-" if x.group(3) == '-' else " ")
                            if x.group(2) else x.group(0), text)
    text = __abbrv_re2.sub(lambda x: x.group(2) + "\u2022" if x.group(2) else x.group(0), text)
    return text


__field_re = re.compile(r"((?:[^(.,;:]|\([^)]*\))+)")

def _split_fields(text: str) -> List[str]:
    '''
    Split an entry into fields (comma-separeted) that are not in groups '()'

    Ex:

    Kurtz (E.) fils, successeur de son père, mécanicien constructeur, fait balanciers, découpoirs, lami- noirs et
    rouleaux en acier fondu, trempés et polis à l’usage des monnaies, des bijoutiers, des or- févres, des fabricants de
    plaqué, enfin pour Jaminer l'acier et le fer; outils pour ferblantiers et boutonniers ; transinission de mouvement
    pour machine à va Peur et manége, lisse pour pa- pier carton ; machines à chocolat et à broyer les couleurs, machi-
    nes à moirer et à gaufrer, pres- ses à cornes, M. H. 1844, Gravil-liers, 11 et 15.

    Gives:
    [ "Kurtz (E.) fils", "successeur de son père", ..., " Gravil-liers", "11 et 15" ]
    '''
    fields =  __field_re.findall(text)
    return [f.strip() for f in fields]

__trans_table = str.maketrans("-", " ")

def text_normalize(text: str, force_ascii = False, block = False, remove_abbrv = False, strip_dashes = True):
    '''
    Trim trailing/leading/extra spaces and convert to lowercase.

    Args:

        block (bool): if enabled, linebreaks are removed (and trailing hyphens may be droped)
        remove_abbrv (bool): if enabled, abbreviations are replaced
        force_ascii (bool): if enabled, accents and other non-ascci chars are replaced



    "  Rue du   Marché Saint-Honoré " -> "rue du marché saint-honoré" (if force_ascii = False)
    "  Rue du   Marché Saint-Honoré " -> "rue du marche saint-honore" (if force_ascii = True)

    Abbreviations removal
    ---------------------
    Remove all abbreviations (not in parenthesis) from the text and returns a new string
    Two kinds of abbreviations are handled:
    1) Abbreviations of places/rues from a whitelist
    2) Abbreviations of names in the form "Ch." "M."

    (1) is replaced by the full name
    (2) remains but the punct is replaced by "\u2022"

    Eg.
    "Lagaffe (G.), av. G. De Gaulle. 12." -> "Lagaffe (G.), avenue G• De Gaulle. 12."
    '''
    if block:
        text = _remove_linebreaks(text)

    # Must be before lowering conversion (because case-dependant)
    if remove_abbrv:
        text = _remove_abbrv(text)

    text = text.strip().lower()

    if strip_dashes:
        text = text.translate(__trans_table)
    
    text = re.sub(r'\s+', ' ', text)

    if force_ascii:
        text = unidecode(text)
    return text




'''
Must match [Prefix]? [Type] (adv) [Name].
E.g.
petite rue du bac
ruelle du bac
grand chemin du bac
'''

__prepositions_voies = set(['au', 'aux', "d'", 'de', "de l'", 'de la', 'des', 'du', 'la', 'à'])
__street_prefix = set(["petit", "grand", "petite", "grande"])
__street_type_voies = set([
    'allée',
    'allées',
    'arcades',
    'autoroute',
    'avenue',
    'balcon',
    'belvédère',
    'boulevard',
    'carrefour',
    'chaussée',
    'chemin',
    'cité',
    'cour',
    'cours',
    'esplanade',
    'galerie',
    'grande avenue',
    'hameau',
    'impasse',
    'jardin',
    'parvis',
    'parvis, place',
    'place',
    'passage',
    'passage souterrain',
    'passerelle',
    'patio-place',
    'place',
    'placette',
    'pont',
    'port',
    'porte',
    'promenade',
    'péristyle',
    'quai',
    'rond-point',
    'route',
    'rue',
    'ruelle',
    'sente',
    'sentier',
    'square',
    'terrasse',
    'villa',
    'voie'
])

__street_type_voies_normalized = set(text_normalize(x, force_ascii=True) for x in __street_type_voies)


__address_start_re = re.compile(r"(?:\b\L<StreetPrefix> )?\b\L<StreetType>\b(?: \L<article>\b)?",
                                StreetPrefix = __street_prefix,
                                StreetType = __street_type_voies_normalized,
                                article = __prepositions_voies)

def _match_street_name(text: str, street_names_dict, street_fullnames_dict, strict = False) :
    if not text:
        return ("", 0)

    # Check exact match with one of the dict
    if text in street_fullnames_dict or text in street_names_dict:
        return (text, 1)

    # Early exit in strict mode
    if strict:
        return ("", 0)

    # Approximative match with a dictionary
    # The confidence of the match is base on:
    # * The a-contrario score (probability of matching with other words with the same distance )
    # * The ratio: (edit distance) / (text length)
    def approx_match(text, d):
        m = None
        if len(text) <= 255 and len(text) > 0:
            m = d.best_match(text, 2)
            if m:
                m["confidence"] = max(0, min(1 / m["count"], 1 - m["distance"] / len(text)))
        return m

    m = approx_match(text, street_fullnames_dict) or approx_match(text, street_names_dict)

    logging.debug('Searching text: %s -> %s', text, m)
    if m is not None:
        return (m["word"], m["confidence"])


    # Le texte n'est ni une adresse complete ni un nom de voie connu, il est donc certainement composé de
    # bruits avant, ou le type de voie n'est pas correct
    #
    # Exemple : travaille prés de la petite   rue      de       Jacques Chirac
    #           ^                  ^ |        |        |        |
    #           ----- bruit -------- prefix?  typvoie  prevoie  nomvoie
    #
    # La confidence maximale devient: 0.5
    # Si bruit -> confidence = 0.3
    # 

    m = __address_start_re.search(text)
    if m:
        b = m.start()
        e = m.end()
        confidence = 0.5 if b == 0 else 0.3
        # Test d'un changement de typvoie
        nomvoie = text[e:].lstrip()
        if not nomvoie:
            return ("",0)
        m = approx_match(nomvoie, street_names_dict)
        logging.debug('Subseq searching text: %s -> %s', nomvoie, m)
        if m and m["confidence"] >= 0.5:
            return (text[b:e] + " " + m["word"], m["confidence"])
        return (text[b:], confidence)

    # No match
    return ("",0)



__sn_number = r"(?:\d{1,3})"
__sn_number_1 = "({n}(?:-{n})*(?:\\s*bis|ter)?)".format(n = __sn_number)
__sn_number_2 = "{n}(?:\\s+et\\s+{n})?".format(n = __sn_number_1)
__street_number_re = re.compile(__sn_number_2, re.IGNORECASE)

'''
Re for street-numbers. Supports:
14
14 bis
14 ter
14-18
14 et 18
'''
def _match_street_number(text: str) -> (Iterable[str], int):
    text = text.strip()
    m = __street_number_re.fullmatch(text)

    if m:
        t = ",".join(x for x in m.groups() if x)
        return (t, 1)

    return ("", 0)



if __name__ == "__main__":
    import sys
    import fileinput

    logging.basicConfig(level=logging.DEBUG, format="%(levelname)s: %(message)s")

    street_names_path = "../resources/denominations-emprises-voies-actuelles.csv"
    p = Parser(street_names_path)

    with sys.stdin as fp:
        text = fp.read()
        p.decode(text)


import pytest
@pytest.mark.parametrize("text, kwargs, expected", [
    ('  Rue du   Marché Saint-Honoré ',
     dict(force_ascii = 0, block = 0, remove_abbrv = 0, strip_dashes = 0),
     'rue du marché saint-honoré'),
    ('  Rue du   Marché Saint-Honoré ',
     dict(force_ascii = 0, block = 0, remove_abbrv = 0, strip_dashes = 1),
     'rue du marché saint honoré'),  
    ('  Rue du   Marché Saint-Honoré ',
     dict(force_ascii = 1, block = 0, remove_abbrv = 0, strip_dashes = 0),
     'rue du marche saint-honore'),
    ("  Grenelle-St-Germ.  ",
     dict(force_ascii = 0, block = 0, remove_abbrv = 1, strip_dashes = 0),      
     'grenelle-saint-germain'),
    ('Ste-Apolline',
     dict(force_ascii = 0, block = 0, remove_abbrv = 1, strip_dashes = 1),
     'sainte apolline'),
    ("À brial(Cte.)O. ,él., p.de France, Plumet, 18.",
     dict(force_ascii = 0, block = 0, remove_abbrv = 1, strip_dashes = 1),
     "à brial(cte.)o\u2022 ,él., place de france, plumet, 18."),
])
def test_normalization(text, kwargs, expected):
    assert text_normalize(text, **kwargs) == expected