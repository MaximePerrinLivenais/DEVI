import os
import json
import tempfile
from filelock import FileLock
import copy
import zipfile
from pathlib import Path

# Exceptions
# =============================================================================
class SaveError(RuntimeError):
    '''
    Generic save error.
    '''
    def __init__(self, msg):
        super().__init__(msg)


class FileAlreadyExists(SaveError):
    '''
    Indicates that a file already exists when trying to save it without forcing overwrite.
    '''
    def __init__(self, filename):
        super().__init__(f"File already exists: \"{filename}\"")


class UnsupportedSaveType(SaveError):
    '''
    Indicates that the selected file format is not supported for saving.
    '''
    def __init__(self, filename):
        super().__init__(f"Unknown file type for saving: {filename}. Supported types: {', '.join(__type_to_saver.keys())}.")


# Public members
# =============================================================================
def save_DOM_guess_type(doc_content, filename, page_id, overwrite_policy="raise_error_if_exists"):
    '''
    Guesses the save format from the filename extension, then tries to save the document.
    The document will be cleaned from temporary data before being saved.
    The behavior to apply when the target file already exists can be defined thanks to an overwrite policy.

    Parameters
    ----------
    doc_content: (dict)
        DOM document content to be saved.

    filename: (str)
        Path to the target destination. For a ZIP file it will be the exact name. For JSON files
        it will be adapted to include page number.

    page_id: (int)
        Number of the page in the document.

    overwrite_policy: (str)
        Selects the behavior to apply when target file (json file, either in ZIP or in filesystem)
        already exists:
        - "raise_error_if_exists": raise FileAlreadyExists
        - "overwrite_all": silently overwrite the destination
        - "overwrite_none": skip (do nothing) without warning

    Returns
    -------
    None

    Exceptions
    ----------
    UnsupportedSaveType: 
        When the filename extension does not indicate a supported file format.

    FileAlreadyExists: 
        When the target file already exists and the `overwrite_policy` is "raise_error_if_exists".

    SaveError:
        When another save-related error is detected.
    '''
    saver = __guess_saver_from_filename(filename)
    if saver is None:
        raise UnsupportedSaveType(filename)
    # else:
    saver(doc_content, filename, page_id, overwrite_policy=overwrite_policy)


def save_DOM_json(doc_content, filename, page_id, overwrite_policy="raise_error_if_exists"):
    '''
    Tries to save the document in JSON format. The filename will be adapted to include page number:
    `file.json` will become `file-0001.json`, when `0001` is the 0-padded, 4-digits number of the page.
    The document will be cleaned from temporary data before being saved.
    The behavior to apply when the target file already exists can be defined thanks to an overwrite policy.

    Parameters
    ----------
    doc_content: (dict)
        DOM document content to be saved.

    filename: (str)
        Path to the target destination. It will be adapted to include page number.

    page_id: (int)
        Number of the page in the document.

    overwrite_policy: (str)
        Selects the behavior to apply when target file (json file, either in ZIP or in filesystem)
        already exists:
        - "raise_error_if_exists": raise FileAlreadyExists
        - "overwrite_all": silently overwrite the destination
        - "overwrite_none": skip (do nothing) without warning

    Returns
    -------
    None

    Exceptions
    ----------
    FileAlreadyExists: 
        When the target file already exists and the `overwrite_policy` is "raise_error_if_exists".

    SaveError:
        When another save-related error is detected.
    '''
    pattern = Path(filename)
    stem = pattern.stem
    real_output_filename = pattern.with_name(f"{stem}-{page_id:04}.json")
    if os.path.exists(real_output_filename):
        if overwrite_policy == "raise_error_if_exists":
            raise FileAlreadyExists(real_output_filename)
        elif overwrite_policy == "overwrite_none":
            # log skipping?
            print(f"Skipping saving of file \"{real_output_filename}\" because \"no overwrite\" policy is selected.")
            return
        # else: overwrite_all
    # else: file does not exist
    # in all other cases, save the file
    json_data = __doc_to_json(doc_content)
    with open(real_output_filename, "w") as output_file:
        json.dump(json_data, output_file, ensure_ascii=False)


def save_DOM_zip(doc_content, filename, page_id, overwrite_policy="raise_error_if_exists"):
    '''
    Tries to save the document in ZIP format.
    The ZIP file will contain files names like `0001.json` 
    where `0001` is the 0-padded, 4-digits number of the page.
    The document will be cleaned from temporary data before being saved.
    The behavior to apply when the target file already exists can be defined thanks to an overwrite policy.

    Parameters
    ----------
    doc_content: (dict)
        DOM document content to be saved.

    filename: (str)
        Path to the target destination of the ZIP file.

    page_id: (int)
        Number of the page in the document.

    overwrite_policy: (str)
        Selects the behavior to apply when target file (json file, either in ZIP or in filesystem)
        already exists:
        - "raise_error_if_exists": raise FileAlreadyExists
        - "overwrite_all": silently overwrite the destination
        - "overwrite_none": skip (do nothing) without warning

    Returns
    -------
    None

    Exceptions
    ----------
    FileAlreadyExists: 
        When the target file already exists (the JSON file) and the `overwrite_policy` is "raise_error_if_exists".

    SaveError:
        When another save-related error is detected.
    '''
    zipPath = Path(filename)
    jsonName = f"{page_id:04}.json"
    lock = FileLock(filename + ".lock")
    with lock:
        if zipPath.exists():
            if not zipPath.is_file():
                raise SaveError(f"\"{zipPath}\" is not a file. Cannot save.")
            if not zipfile.is_zipfile(zipPath):
                raise SaveError(f"\"{zipPath}\" is not a valid zip file. Cannot save.")
        # else: we open/append the file and investigate further
        replace_json = False  # indicates whether replacing the json file is necessary
        with zipfile.ZipFile(zipPath, 'a') as zipObj:
            if jsonName in zipObj.namelist():
                if overwrite_policy == "raise_error_if_exists":
                    raise FileAlreadyExists(f"{zipPath}#{jsonName}")
                elif overwrite_policy == "overwrite_none":
                    # log skipping?
                    print(f"Skipping saving of file \"{zipPath}#{jsonName}\" because \"no overwrite\" policy is selected.")
                    return
                else: # overwrite_all and json already present
                    # defer the replacement of the json file
                    # (because the zip module does not permit to remove an element directly)
                    replace_json = True
            else: # json file does not exist
                # since the file is open, we can add a new element
                json_data = __doc_to_json(doc_content)
                json_bytes = json.dumps(json_data, ensure_ascii=False)
                zipObj.writestr(jsonName, json_bytes)

        if replace_json:
            # Hack because we cannot remove/update a file in a ZIP file.
            opFile, tmpZip = tempfile.mkstemp(dir=os.path.dirname(zipPath))
            os.close(opFile)
            with zipfile.ZipFile(tmpZip, 'w') as zipOut:
                with zipfile.ZipFile(zipPath, 'r') as zipIn:
                    for f in zipIn.infolist():
                        if jsonName != f.filename:
                            data = zipIn.read(f.filename)
                            zipOut.writestr(f.filename, data)
                json_data = __doc_to_json(doc_content)
                json_bytes = json.dumps(json_data, ensure_ascii=False)
                zipOut.writestr(jsonName, json_bytes)
            os.remove(zipPath)
            os.rename(tmpZip, zipPath)


# Internal definitions
# =============================================================================
__overwrite_policies = ["raise_error_if_exists", "overwrite_all", "overwrite_none"]
__type_to_saver = {".json": save_DOM_json, ".zip": save_DOM_zip}


def __guess_saver_from_filename(filename):
    '''
    Returns the function to use to save a document according to `filename` extension
    or `None` if no suitable function is found.
    '''
    suffix = Path(filename).suffix.lower()
    if suffix in __type_to_saver:
        return __type_to_saver[suffix]
    else:
        return None


def __doc_to_json(doc_content):
    '''
    Prepares the document to be saved in JSON.
    Removes temporary content like parent information which is not meant to be saved directly.
    '''
    doc_no_tmp_data = copy.deepcopy(doc_content)
    return doc_no_tmp_data
