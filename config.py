def can_build(env, platform):
    # crashes on linux right now
    return not env.msvc


def configure(env):
    pass


def get_doc_classes():
    return []


def get_doc_path():
    return "doc_classes"
