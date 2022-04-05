#!/usr/bin/env python
"""
run this every time you upgrade the godot-base version to generate new matching github workflows

usage:
python build_github_actions.py --godot-version "3.4.4-stable" --godot-github-folder ../../.github --ECMAS-github-folder .github

"""

import argparse
import yaml
import os
import subprocess
from dataclasses import dataclass, field
from typing import Dict

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
    ENV: Dict[str, str] = field(default_factory=dict)

    def add_to_flags(self, toadd: str) -> None:
        if not self.SCONSFLAGS.endswith(" "):
            toadd = f" {toadd}"
        self.SCONSFLAGS = f"{self.SCONSFLAGS} {toadd}"

    def get_fixed_flags(self) -> str:
        todel = ["warnings=all", "werror=yes"]
        for x in todel:
            self.SCONSFLAGS = self.SCONSFLAGS.replace(x, "")
        return self.SCONSFLAGS


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

    basic_flags = " "
    workflows = {
        "android_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "ios_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "javascript_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "linux_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "macos_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "server_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "windows_builds.yml": BuildOpts(f"{basic_flags} use_mingw=yes", args.godot_version),
    }
    subprocess.call(["rm", os.path.join(args.ECMAS_github_folder, "workflows", "static_checks.yml")])
    for wf_base_fn, build_opts in workflows.items():
        full_fn = os.path.join(args.ECMAS_github_folder, "workflows", wf_base_fn)
        data = yaml.safe_load(open(full_fn))

        build_opts.add_to_flags(data["env"]["SCONSFLAGS"])
        data["env"]["SCONSFLAGS"] = build_opts.get_fixed_flags()
        data["env"]["GODOT_BASE_BRANCH"] = build_opts.GODOT_BASE_BRANCH
        for k, v in build_opts.ENV.items():
            data["env"][k] = v

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
        if "windows" in wf_base_fn:
            # msvc_config_var = "SCONS_CACHE_MSVC_CONFIG"
            # if msvc_config_var in data["env"]:
            #     del data["env"][msvc_config_var]
            # data["jobs"][only_template_name]["runs-on"] = data["jobs"][only_template_name]["runs-on"].replace(
            #     "latest", "2019"
            # )
            data["jobs"][only_template_name]["defaults"] = {"run": {"shell": "msys2 {0}"}}
            new_steps.append(
                {
                    "uses": "msys2/setup-msys2@v2",
                    "with": {"msystem": "MINGW64", "update": True, "install": "mingw-w64-x86_64-cc"},
                }
            )

        for step in data["jobs"][only_template_name]["steps"]:
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
