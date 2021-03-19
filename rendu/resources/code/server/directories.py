import os
import os.path as osp
import zipfile
import tempfile as tmp
import json
import io
import glob
from io import BytesIO
from PIL import Image as img
from flask import Blueprint, request, jsonify, send_file, safe_join, abort, Response
from back import Application, Loader, Saver, PDFInfo

bp_directories = Blueprint('directories', __name__, url_prefix='/directories')
bp_directories.config = {}

def get_stem(path):
    '''
    Return the stem of the path arg.
    Ex: 'aaa/bbb/ccc.zip'
    > 'ccc'
    '''
    return os.path.splitext(os.path.basename(path))[0]

def get_stem_with_extension(path, ext):
    '''
    Return the stem of the path arg with the good extension (.zip/.txt/...).
    Ex: 'aaa/bbb/ccc.pdf', '.zip'
    > ''ccc.zip'
    '''
    return "{}.{}".format(get_stem(path), ext)


@bp_directories.record
def record_config(setup_state):
    bp_directories.config = setup_state.app.config

@bp_directories.route('/', methods=['GET'])
def show_all_directories():
    list_dir = glob.glob(safe_join(bp_directories.config['SODUCO_DIRECTORIES_PATH'], '*.pdf'))
    directories = dict()
    for d in list_dir:
        directory_name = get_stem_with_extension(d, 'pdf')
        directories[directory_name] = dict()


    res_dict = dict()
    res_dict["directories"] = directories
    return jsonify(res_dict)


@bp_directories.route('/<directory>', methods=['GET'])
def get_directory(directory):
    directory_path = safe_join(bp_directories.config['SODUCO_DIRECTORIES_PATH'], get_stem_with_extension(directory, "pdf"))
    res_json = dict()
    try:
        doc = PDFInfo(directory_path)
        pages = doc.get_num_pages()
    except RuntimeError as e:
        abort(500, description="Wrong pdf")
    res_json["num_pages"] = pages
    directory_name = get_stem_with_extension(directory_path, 'pdf')
    res_json["filename"] = directory_name
    return jsonify(res_json)


@bp_directories.route('/<directory>/<int:view>/image', methods=['GET'])
def get_image(directory, view):
    directory_path = safe_join(bp_directories.config['SODUCO_DIRECTORIES_PATH'], get_stem_with_extension(directory, "pdf"))
    if not osp.exists(directory_path):
        abort(404, f"pdf file of {directory} not found")
    app = Application(directory_path, view, None, deskew_only=True)
    image = img.fromarray(app.DeskewedImage)
    data = BytesIO()
    image.save(data, 'JPEG')
    data.seek(0)
    return send_file(data, mimetype='image/jpeg')


@bp_directories.route('/<directory>/<int:view>/annotation', methods=['GET', 'PUT'])
def access_annotation(directory, view):
    '''
    Return the content or a file containing the content seeked.

    ---------------------------------------------------------
    | force_compute | download | Exist        | Missing     |
    ---------------------------------------------------------
    |       0       |          | C = loadfile | c = compute |
    |               |          |              | cache c     |
    ---------------------------------------------------------
    |       1       |          | C = compute  | c = compute |
    |               |          |              | cache c     |
    ---------------------------------------------------------
    |               |     0    |    return    |     return  |
    |               |          | jsonify(c)   | jsonify(c)  |
    ---------------------------------------------------------
    |               |     1    |    return    |return       |
    |               |          | file         | file        |
    |               |          | containing C | containing C|
    ---------------------------------------------------------
    Parameters
    ----------
    directory: (str)
        name of the pdf

    view : (Integer)
        the numero of the page of the pdf

    Methods
    -------
    GET|PUT

    Action
    ------
    force_compute : (None|'0'|'1')
    download: (None|'0'|'1')


    if force_compute == '1', download == '1' and there is already a file, we create a virtual file co   ntaining C and return it

    '''
    if request.method == 'GET':
        force_compute = turn_to_bool(request.args.get('force_compute'))
        download = turn_to_bool(request.args.get('download'))
        content = None
        if not force_compute: 
            content = Loader.load_page(safe_join(bp_directories.config['SODUCO_ANNOTATIONS_PATH'], get_stem_with_extension(directory, "zip")), view)
            mode = "cached"
        if not content or force_compute: #if no content were found on the server we compute
            directory_path = safe_join(bp_directories.config['SODUCO_DIRECTORIES_PATH'], get_stem_with_extension(directory, "pdf"))
            if not osp.exists(directory_path):
                abort(404, f"pdf file of {directory} not found")
            app = Application(directory_path, view, None)
            content = app.GetDocument()
            mode = "computed"

        # cache the computed data if not already cached
        directory_found = False
        file_found = False
        directory_path = osp.abspath(safe_join(bp_directories.config['SODUCO_ANNOTATIONS_PATH'], get_stem_with_extension(directory, "zip")))
        if osp.exists(directory_path):
            directory_found = True
        if directory_found:
            zip_name = f'{view:04}.json'
            with zipfile.ZipFile(directory_path) as zip_obj:
                if zip_name in zip_obj.namelist():
                    file_found = True
        if not file_found:
            Saver.save_DOM_guess_type(content, safe_join(bp_directories.config['SODUCO_ANNOTATIONS_PATH'], 
                get_stem_with_extension(directory, "zip")), view, None)

        if download:
            downloaded_page = None
            filename = None
            if force_compute and file_found:
                downloaded_page = download_computed_page(directory, view, content) #return virtual file containing the content
            if not downloaded_page:
                downloaded_page = download_page(directory, view) #return the file on the server containing the content
            return downloaded_page #return file
        else:
            return jsonify({ "content": content, "mode": mode })#return jsonify({ content, mode})
    elif request.method == 'PUT':
        content = request.get_json(force=True)['content']
        Saver.save_DOM_guess_type(content, safe_join(bp_directories.config['SODUCO_ANNOTATIONS_PATH'], 
            get_stem_with_extension(directory, "zip")), view, overwrite_policy="overwrite_all")
        return "Content saved on the server", 200


@bp_directories.route('/<directory>/download_directory', methods=['GET'])
def download_directory(directory):
    directory_path = osp.abspath(safe_join(bp_directories.config['SODUCO_ANNOTATIONS_PATH'], get_stem_with_extension(directory, "zip")))
    # json error if file doesn't exist
    if not osp.exists(directory_path):
        abort(404, f"zip file of {directory} not found")
    return send_file(directory_path, as_attachment=True)

#@bp_directories.route('/<directory>/<int:view>/download_page', methods=['GET'])
def download_page(directory, view, filename = None):
   
    directory_path = osp.abspath(safe_join(bp_directories.config['SODUCO_ANNOTATIONS_PATH'], get_stem_with_extension(directory, "zip")))
    # json error if file doesn't exist
    if not osp.exists(directory_path):
        abort(404, f"zip file of {directory} not found")

    zip_name = f'{view:04}.json'
    mkdtmp = tmp.mkdtemp()
    with zipfile.ZipFile(directory_path) as zip_obj:
        # json error if json doesn't exist in zip
        if zip_name not in zip_obj.namelist():
            abort(404, f"json file {zip_name} not found in {directory}")
        zip_path = zip_obj.extract(zip_name, mkdtmp)

    # Return json file
    return send_file(zip_path, as_attachment=True, mimetype='text/plain')

def download_computed_page(directory, view, content):
    proxy = io.StringIO()
    json.dump(content, proxy)
    mem = io.BytesIO()
    mem.write(proxy.getvalue().encode('utf-8'))
    mem.seek(0)
    proxy.close()
    return send_file(
    mem,
    as_attachment= True,
    attachment_filename=  f'{view:04}' + '_' + get_stem_with_extension(directory, "json"),
    mimetype= 'text/json')

def turn_to_bool(action):
    if not action or action == '0':
        return False
    if action == '1':
        return True
    abort(400, "bad request. force_compute and download can be only 0 or 1!")
