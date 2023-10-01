def can_build(env, platform):
    return not (platform == "windows" and not env["use_mingw"])


def configure(env):
    pass


def get_doc_classes():
    return [
        "JavaScript",
        "JavaScriptModule",
    ]


def get_doc_path():
    return "doc_classes"
