import os
import sys
from flask import Flask

# FIXME: bad use
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import directories


SODUCO_DIRECTORIES_PATH = "/data/directories"
SODUCO_ANNOTATIONS_PATH = "/data/annotations"

app = Flask(__name__, instance_relative_config=True)
app.config.from_object(__name__)
app.register_blueprint(directories.bp_directories)
