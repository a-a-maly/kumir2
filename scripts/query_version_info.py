import re
import subprocess
import os
import os.path
import sys
import time
from urllib.parse import unquote


def to_str(x):
    assert isinstance(x, str) or isinstance(x, bytes)
    if isinstance(x, str):
        return x
    else:
        return x.decode("utf-8")


OUT_FILE = sys.stdout

if "nt" == os.name:
    GIT_PATH_SEARCH = [
        r"C:\Git\bin",
        r"C:\Program Files\Git\bin",
        r"C:\Program Files (x86)\Git\bin"
    ]
else:
    GIT_PATH_SEARCH = [
        "/usr/bin",
        "/usr/local/bin"
    ]


def _add_path_env(value):
    if "nt" == os.name:
        value = '"' + value + '"'
        sep = ";"
    else:
        sep = ":"
    old_path = os.getenv('PATH')
    if old_path is None:
        new_path = value
    else:
        new_path = old_path + sep + value
    os.environ['PATH'] = new_path


GIT_FOUND = False

for path_variant in GIT_PATH_SEARCH:
    candidate = path_variant + os.path.sep + "git"
    if "nt" == os.name:
        candidate += ".exe"
    if os.path.exists(candidate):
        _add_path_env(path_variant)
        GIT_FOUND = True
        break

if not GIT_FOUND:
    sys.stderr.write("Git executable not found!\n")
    sys.exit(1)

TOP_LEVEL_DIR = os.getcwd()

def get_version_information():
    assert isinstance(TOP_LEVEL_DIR, str)
    result = {
        "taggedRelease": False,
        "version": None,
        "hash": None,
        "branch": None,
        "date": get_date()
    }

    if os.path.exists(TOP_LEVEL_DIR + os.path.sep + ".git"):
        try:
            version_info = subprocess.check_output(
                "git describe --abbrev=0 --tags --exact-match",
                shell=True,
                stderr=subprocess.PIPE
            ).strip()
            result["taggedRelease"] = True
            result["version"] = to_str(version_info)
        except subprocess.CalledProcessError:
            pass

        if result["version"]:
            return result

        branch_name = ""
        try:
            branch_name = to_str(subprocess.check_output(
                "git rev-parse --abbrev-ref HEAD",
                shell=True
            )).strip()
        except subprocess.CalledProcessError:
            pass
        if branch_name == "HEAD": # detached HEAD?
            branch_name = os.environ.get("CI_COMMIT_REF_NAME", "").strip()
        if not branch_name:
            branch_name = "NONE"

        hash_tag = ""
        try:
            hash_tag = to_str(subprocess.check_output(
                "git rev-parse --short=12 --verify HEAD",
                shell=True
            )).strip()
        except subprocess.CalledProcessError:
            pass
        if not hash_tag:
            hash_tag = os.environ.get("CI_COMMIT_SHA", "").strip()
        if not hash_tag:
            hash_tag = "0000000000000000000000000000000000000000"

        result["taggedRelease"] = False
        result["branch"] = branch_name
        result["hash"] = hash_tag

    else:
        dir_name = os.path.basename(TOP_LEVEL_DIR)
        match = re.match(r"kumir2-(.+)", dir_name)
        version_info = match.group(1)
        if version_info.startswith("2"):
            result["version"] = version_info
            result["taggedRelease"] = True

    return result


def get_date():
    timestamp = int(get_timestamp())
    localtime = time.localtime(timestamp)
    assert isinstance(localtime, time.struct_time)
    return "{:04}{:02}{:02}".format(localtime.tm_year, localtime.tm_mon, localtime.tm_mday)


def get_timestamp():
    assert isinstance(TOP_LEVEL_DIR, str)
    if os.path.exists(TOP_LEVEL_DIR + os.path.sep + ".git"):
        return to_str(subprocess.check_output(
            "git --no-pager log -1 --pretty=format:%ct",
            shell=True,
            stderr=subprocess.PIPE
        )).strip()
    else:
        return "{}".format(int(time.time()))


def is_tag(version):
    return version.startswith("2")


def find_suitable_list_file_name(version_info):
    base = TOP_LEVEL_DIR + os.path.sep + "subdirs-disabled-{}.txt"
    if version_info["taggedRelease"]:
        name = base.format(version_info["version"])
    else:
        name = base.format(version_info["branch"])
    if os.path.exists(name):
        return name
    if version_info["taggedRelease"]:
        match = re.match(r"(.+)-(alpha|beta|rc|pt|test)[0-9]+", version_info["version"])
        version_base = match.group(1)
    else:
        version_base = version_info["branch"]
    if version_base:
        name = base.format(version_base)
        if os.path.exists(name):
            return name


def disabled_modules():
    version_info = get_version_information()
    disabled_list_file_name = find_suitable_list_file_name(version_info)
    disabled_list = []
    if disabled_list_file_name:
        with open(disabled_list_file_name) as source:
            lines = source.read().split('\n')
            for line in lines:
                line = line.strip()
                if line and not line.startswith("#"):
                    disabled_list += [line]
    return disabled_list


def cmake_disabled_modules():
    mods = ["[" + entry + "]" for entry in disabled_modules()]
    if mods:
        return " ".join(mods)
    else:
        return ""


def cmake_version_info():
    version_name = get_version_information()
    assert isinstance(version_name, dict)
    timestamp = get_timestamp()
    output = ""
    if version_name["taggedRelease"]:
        output += "-DGIT_TAG=\"{}\";".format(version_name["version"])
        output += "-DGIT_BRANCH=\"unknown\";"
        output += "-DGIT_HASH=\"unknown\";"
    else:
        output += "-DGIT_TAG=\"unknown\";"
        output += "-DGIT_BRANCH=\"{}\";".format(version_name["branch"])
        output += "-DGIT_HASH=\"{}\";".format(version_name["hash"])
    output += "-DGIT_TIMESTAMP=\"{}\";".format(timestamp)
    return output


def cmake_version_info_tbht():
    version_name = get_version_information()
    assert isinstance(version_name, dict)
    timestamp = get_timestamp()
    output_values = []
    if version_name["taggedRelease"]:
        output_values += to_str(version_name["version"])
        output_values += ["unknown"] * 2
    else:
        output_values += ["unknown"]
        output_values += [to_str(version_name["branch"])]
        output_values += [to_str(version_name["hash"])]
    output_values += [to_str(timestamp)]
    return ";".join(output_values)


def source_file_name(prefix: str, suffix: str):
    version_info = get_version_information()
    if version_info["taggedRelease"]:
        version_name = version_info["version"]
    else:
        version_name = version_info["branch"] + "-"
        version_name += version_info["hash"] + "-"
        version_name += version_info["date"]
    return prefix + version_name + suffix


def package_bundle_name():
    prefix = ""
    suffix = ""
    nl = ""
    for arg in sys.argv:
        if arg.startswith("--prefix="):
            prefix = unquote(arg[9:].replace('@', '%'))
        elif arg.startswith("--suffix="):
            suffix = unquote(arg[9:].replace('@', '%'))
        elif "--nl" == arg:
            if os.name.startswith("nt"):
                nl = "\r\n"
            else:
                nl = "\n"
    output = source_file_name(prefix, suffix) + nl
    return output


def nsis_include_file():
    version_info = get_version_information()
    data = ""
    if version_info["taggedRelease"]:
        data += "OutFile \"kumir2-" + version_info["version"] + "-install.exe\"\r\n"
        data += "Name \"Кумир " + version_info["version"] + "\"\r\n"
        data += "InstallDir \"$PROGRAMFILES\\Kumir-" + version_info["version"] + "\"\r\n"
        data += "!define VERSION_SUFFIX \"" + version_info["version"] + "\"\r\n"
    else:
        data += "OutFile \"kumir2-" + version_info["branch"] + "-"
        data += version_info["date"] + "-" + version_info["hash"] + "-install.exe\"\r\n"
        data += "Name \"Кумир 2.x-" + version_info["branch"] + "\"\r\n"
        data += "InstallDir \"$PROGRAMFILES\\Kumir2x-" + version_info["branch"] + "\"\r\n"
        data += "!define VERSION_SUFFIX \"" + version_info["branch"] + "\"\r\n"
    return data


class ChangelogEntry:
    def __init__(self, timestamp: int, author: str, description: str):
        self.timestamp = timestamp
        self.author = author
        self.description = description


def get_changelog(max_count=1000, after=(2015, 5, 1)):
    if os.path.exists(".git"):
        result = []
        command = "git --no-pager log --max-count={} --after={}/{}/{} --no-merges --pretty=format:'%ct;;;%an <%ae>;;;%s'"
        command = command.format(max_count, after[0], after[1], after[2])
        lines = to_str(subprocess.check_output(
            "git describe --abbrev=0 --tags --exact-match",
            shell=True,
            stderr=subprocess.PIPE
        ).strip()).splitlines()
        for line in lines:
            tokens = line.split(";;;")
            entry = ChangelogEntry(int(tokens[0]), tokens[1].strip(), tokens[2].strip())
            result += [entry]
        return result


def main():
    global OUT_FILE
    global TOP_LEVEL_DIR
    mode = "package_bundle_name"
    out_file_name = None
    for arg in sys.argv:
        if arg.startswith("--mode="):
            mode = arg[7:]
        elif arg.startswith("--out="):
            out_file_name = arg[6:]
        elif arg.startswith("--toplevel="):
            TOP_LEVEL_DIR = arg[11:]
    custom_encoding = False
    if out_file_name:
        if mode.startswith("nsis"):
            open_mode = "wb"
            custom_encoding = "CP1251"
        else:
            open_mode = "w"
        OUT_FILE = open(out_file_name, open_mode)
    if mode in globals():
        data = globals()[mode]()
        if custom_encoding:
            data = data.encode(custom_encoding)
        OUT_FILE.write(data)
    OUT_FILE.close()


if __name__ == "__main__":
    main()
