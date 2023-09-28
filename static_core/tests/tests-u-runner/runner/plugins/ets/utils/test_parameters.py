# This file provides functions thar are responsible for loading test parameters
# that are used in templates (yaml lists).

import os.path as ospath
from typing import Dict, List, Any
from pathlib import Path
import yaml

from runner.plugins.ets.utils.constants import YAML_EXTENSIONS, LIST_PREFIX
from runner.plugins.ets.utils.exceptions import InvalidFileFormatException
from runner.utils import iter_files, read_file

Params = Dict


def load_params(dirpath: Path) -> Params:
    """
    Loads all parameters for a directory
    """
    result = {}

    for filename, filepath in iter_files(dirpath, allowed_ext=YAML_EXTENSIONS):
        name_without_ext, _ = ospath.splitext(filename)
        if not name_without_ext.startswith(LIST_PREFIX):
            raise InvalidFileFormatException(message="Lists of parameters must start with 'list.'", filepath=filepath)
        listname = name_without_ext[len(LIST_PREFIX):]
        params: List[Dict[str, Any]] = parse_yaml(filepath)
        if not isinstance(params, list):
            raise InvalidFileFormatException(message="Parameters list must be YAML array", filepath=filepath)
        result[listname] = params
    return result


def parse_yaml(path: str) -> Any:
    """
    Parses a single YAML list of parameters
    """
    text = read_file(path)
    try:
        params = yaml.load(text, Loader=yaml.FullLoader)
    except Exception as common_exp:
        raise InvalidFileFormatException(
            message=f"Could not load YAML: {str(common_exp)}",
            filepath=path) from common_exp
    return params
