#!/usr/bin/env python
"""
run this every time you upgrade the godot-base version to generate new matching github workflows

usage:
python build_github_actions.py --godot-version "3.4" --godot-github-folder ../../.github --ECMAS-github-folder .github

"""

import argparse
import yaml
import os
import subprocess
from dataclasses import dataclass

# https://stackoverflow.com/a/33300001 + some changes
def str_presenter(dumper, data):
    if len(data.splitlines()) > 1 or "\n" in data:  # check for multiline string
        return dumper.represent_scalar("tag:yaml.org,2002:str", data, style="|")
    return dumper.represent_scalar("tag:yaml.org,2002:str", data)


yaml.add_representer(str, str_presenter)

# to use with safe_dump:
yaml.representer.SafeRepresenter.add_representer(str, str_presenter)

# END https://stackoverflow.com/a/33300001


@dataclass
class BuildOpts:
    SCONSFLAGS: str
    GODOT_BASE_BRANCH: str


def parseargs():
    parser = argparse.ArgumentParser()
    parser.add_argument("--godot-version", required=True)
    parser.add_argument("--godot-github-folder", required=True)
    parser.add_argument("--ECMAS-github-folder", required=True)
    return parser.parse_args()


def main():
    args = parseargs()
    assert os.path.isdir(args.godot_github_folder)
    assert os.path.isdir(args.ECMAS_github_folder)

    for x in ["actions", "workflows"]:
        subprocess.call(["rm", "-rf", os.path.join(args.ECMAS_github_folder, x)])
        subprocess.call(
            ["cp", "-r", os.path.join(args.godot_github_folder, x), os.path.join(args.ECMAS_github_folder, x)]
        )

    basic_flags = "module_text_server_fb_enabled=yes verbose=yes"
    workflows = {
        "android_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "ios_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "javascript_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "linux_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "macos_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "server_builds.yml": BuildOpts(basic_flags, args.godot_version),
        # "static_checks.yml": BuildOpts(basic_flags, args.godot_version),
        "windows_builds.yml": BuildOpts(basic_flags, args.godot_version),
    }
    subprocess.call(["rm", os.path.join(args.ECMAS_github_folder, "workflows", "static_checks.yml")])
    for wf_base_fn, build_opts in workflows.items():
        full_fn = os.path.join(args.ECMAS_github_folder, "workflows", wf_base_fn)
        data = yaml.safe_load(open(full_fn))
        data["env"]["SCONSFLAGS"] = build_opts.SCONSFLAGS
        data["env"]["GODOT_BASE_BRANCH"] = build_opts.GODOT_BASE_BRANCH
        if True in data.keys():
            new_data = {"name": data["name"], "on": data[True]}
            del data[True]
            for k, v in data.items():
                if k in ("name", "on"):
                    continue
                new_data[k] = v
            data = new_data
        assert len(data["jobs"]) == 1
        only_template_name = list(data["jobs"].keys())[0]
        new_steps = []
        for step in data["jobs"][only_template_name]["steps"]:
            if "javascript" in full_fn:
                print(step)
            if "uses" in step and "checkout" in step["uses"]:
                checkout_godot = {
                    "name": "Checkout Godot",
                    "uses": "actions/checkout@v2",
                    "with": {"repository": "godotengine/godot", "ref": "${{ env.GODOT_BASE_BRANCH }}"},
                }
                checkout_ecmas = {
                    "name": "Checkout ECMAScript",
                    "uses": "actions/checkout@v2",
                    "with": {"path": "${{github.workspace}}/modules/ECMAScript/"},
                }
                new_steps.append(checkout_godot)
                new_steps.append(checkout_ecmas)
            else:
                new_steps.append(step)
        data["jobs"][only_template_name]["steps"] = new_steps
        with open(full_fn, "w") as fh:
            yaml.dump(data, fh, sort_keys=False, allow_unicode=True)


if __name__ == "__main__":
    main()
