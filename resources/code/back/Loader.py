import os
import json
import tempfile
import copy
import zipfile
from pathlib import Path
import back.Application as Application

class LoadError(RuntimeError):
    '''
    Generic Load Error.
    '''
    def __init__(self, msg):
        super().__init__(msg)

class  FileDoNotExist(LoadError):
    '''
    Indicates that a file do not exist.
    '''

    def __init__(self, filename):
        super().__init__(f"File do not exists: \"{filename}\"")

class UnsupportedLoadType(LoadError):
    '''
    Indicates that the selected file format is not supported for loading.
    '''
    def __init__(self, filename):
         super().__init__(f"Unknown file type for loading: {filename}. Supported types: {', '.join(__type_to_loader.keys())}.")

# Public members
# =============================================================================================
def load_DOM_guess_type(filename, load_policy="raise_error_if_do_not_exists"):
    '''
    Guesses the format of the file from the filename extension, then tries to load the document.
    The behavior to apply when the target file does not exist can be defined thanks to a load policy.

    Parameters
    ----------
    filename: (str)
        Path to the target destination. for a ZIP file, all document will be extracted into the map. For Json-file, we 
        ignore the name and just keep information about the 4 last number to decide the page.

    load_policy: (str)
        Selects the behavior to apply when target file (json file, either in ZIP or in filesystem)
        do not exists:
        - "raise_error_if_do_not_exists": raise FileDoNotExists
        - "read_empty": load file as an empty file

    Returns
    --------
    preloaded_map: (map)
        map where the DOM document content will be saved. <key=num_page; value=DOM document content>

    None if error

    Exceptions
    ----------
    UnsupportedLoadType:
        When the filename extension does not indicate a supported file format.

    FileAlreadyExists:
        When the target file do not exists and the `load_policy` is "raise_error_if_do_not_exists".

    LoadError:
        When another load-related error is detected.
    '''
    loader = ___guess_loader_from_filename(filename)
    if loader is None:
        raise UnsupportedLoadType(filename)
    return loader(filename, load_policy)


def load_DOM_json(filename, load_policy="raise_error_if_do_not_exists"):
    '''
    Tries to load the DOM doc content from a JSON file. The page id will be identified by the 4 digits 
    before the extension. 'file-0001.json' will load the data for the page 1.
    The behavior to apply when the target file do not exists can be defined thanks to a load_policy'.

    Parameters
    ---------- 
    filename: (str)
        Path to the target input. It will generate an error if no page number was founded.

    load_policy: (str)
        Selects the behavior to apply when target file (json file, either in ZIP or in filesystem)
        do not exists:
        - "raise_error_if_do_not_exists": raise FileDoNotExists
        - "read_empty": load file as an empty file

    Returns
    --------
    preloaded_map: (map)
        map where the DOM document content will be saved. <key=num_page; value=DOM document content>

    Exceptions
    ----------
    UnsupportedLoadType:
        When the filename extension does not indicate a supported file format.

    FileAlreadyExists:
        When the target file do not exists and the `load_policy` is "raise_error_if_do_not_exists".

    LoadError:
        When another load-related error is detected.
    '''
    preloaded_dict = dict()
    pattern = Path(filename)
    stem = pattern.stem
    str_page_id = stem[-4:]
    for i in str_page_id:
        if not(('0' <= i) and (i <= '9')):
            raise UnsupportedLoadType(filename)
    if not os.path.exists(filename):
        if load_policy == "raise_error_if_do_not_exists":
            raise FileDoNotExist(filename)
        return preloaded_dict

    with open(pattern) as JSONFile:
        json_data = __json_from_file(JSONFile)
        preloaded_dict[str_page_id] = json_data
        JSONFile.close()
    return preloaded_dict


def load_DOM_zip(filename, load_policy="raise_error_if_do_not_exists", page_id=None):
    '''
    Tries to load the DOM doc content from a zip file.
    The ZIP file will contain files names like `0001.json` 
    where `0001` is the 0-padded, 4-digits number of the page.
    All json file from the ZIP file will be loaded.
    The behavior to apply when the target file do not exists can be defined thanks to a load_policy'.

    Parameters
    ----------
    filename: (str)
        Path to the target input. It will generate an error if no page number was founded.

    load_policy: (str)
        Selects the behavior to apply when target file (json file, either in ZIP or in filesystem)
        do not exists:
        - "raise_error_if_do_not_exists": raise FileDoNotExists
        - "read_empty": load file as an empty file

    Returns
    --------
    preloaded_map: (map)
        map where the DOM document content will be saved. <key=num_page; value=DOM document content>

    None if error

    Exceptions
    ----------
    UnsupportedLoadType:
        When the filename extension does not indicate a supported file format.

    FileAlreadyExists:
        When the target file do not exists and the `load_policy` is "raise_error_if_do_not_exists".

    LoadError:
        When another load-related error is detected.
    '''
    preloaded_dict = dict()
    zipPath = Path(filename)
    if not os.path.exists(filename):
        if load_policy == "raise_error_if_do_not_exists":
            raise FileDoNotExist(filename)
        return None

    with zipfile.ZipFile(zipPath) as zipObj:
        for json in zipObj.namelist():
            str_page_id = str(Path(json).stem)
            with zipObj.open(json) as jsonFile:
                json=__json_from_file(jsonFile)
                jsonFile.close()
            preloaded_dict[str_page_id]=json
        zipObj.close()
    return preloaded_dict


def load_page(filename, page_id):
    '''
    Return the preloaded content of the page if already loaded.
    Return None if not already preloaded.

    Paramaters
    ----------
    dictionary: (dict)
        the dictionnary were the content of the document are stored.
    page_id: (int)
        the target page.

    Returns
    -------
    content of the page from dictionnary if founded
    None else
    '''
    str_page_id = f"{page_id:04}"
    if not filename:
        return None
    dictionary = load_DOM_guess_type(filename, load_policy="load_empty") or dict()
    return dictionary.get(str_page_id, None)

# Internal definitions
# =============================================================================================
__load_policies = ["raise_error_if_do_not_exists", "load_empty"]
__type_to_loader = {".json": load_DOM_json, ".zip": load_DOM_zip}

def ___guess_loader_from_filename(filename):
    '''
     Returns the function to use to save a document according to `filename` extension
    or `None` if no suitable function is found.
    '''
    suffix = Path(filename).suffix.lower()
    if suffix in __type_to_loader:
        return __type_to_loader[suffix]
    return None

def __json_from_file(jsonFile):
    '''
    Load and prepare the json to be used in the application.
    Create parent link between dict.
    '''
    jsondoc = json.load(jsonFile)
    for x in jsondoc:
        if x["type"] == "ENTRY" or x["type"] == "TITLE_LEVEL_1" or x["type"] == "TITLE_LEVEL_2":
            if x.get("origin") is None:
                x["origin"] = "computer"
            if x.get("checked") is None:
                x["checked"] = False
    return jsondoc

def insert_parent_link(dic):
    '''
    Insert in the json a parent link to help with the merge.
    '''
    if dic.get("children", None):
        for child in dic["children"]:
            child["parent"] = dic
    return True



