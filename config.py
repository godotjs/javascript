def can_build(env, platform):
    return (platform == "windows" and env["use_mingw"]) or platform == "linux"


def configure(env):
    pass


def get_doc_classes():
    return [
        "JavaScript",
        "JavaScriptModule",
    ]


def get_doc_path():
    return "doc_classes"
