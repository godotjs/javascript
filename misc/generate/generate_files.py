import platform, os


def get_path(javascript_dir, path):
    return os.path.abspath(os.path.join(javascript_dir, path))


def open_file(javascript_dir, path, mode):
    file = get_path(javascript_dir, path)
    if platform.python_version() > "3":
        return open(file, mode, encoding="utf8")
    else:
        return open(file, mode)


def dump_text_file_to_cpp(javascript_dir, path):
    source = open_file(javascript_dir, path, "r").read()
    lines = source.split("\n")
    source = ""
    length = len(lines)
    for i in range(length):
        line = lines[i].replace('"', '\\"')
        line = '\t"' + line + '\\n"'
        if i < length - 1:
            line += "\n"
        source += line
    return source


def generate(javascript_dir, header, files):
    for fn, subs in files.items():
        with open_file(javascript_dir, fn, "w") as fh:
            name = subs[0]
            file = dump_text_file_to_cpp(javascript_dir, subs[1])
            fh.write(header.format(name, file))


def generate_templates(javascript_dir, header, files):
    for fn, subs in files.items():
        with (open_file(javascript_dir, fn, "w") as fh):
            cpp_name = subs[0]
            inherit = subs[1]
            template_name = subs[2]
            template_description = subs[3]
            file = dump_text_file_to_cpp(javascript_dir, subs[4])
            file = file.replace("GODOT_OBJECT_NAME", "\" GODOT_OBJECT_NAME \"")
            fh.write(header.format(cpp_name, inherit, template_name, template_description, file))
