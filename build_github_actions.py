#!/usr/bin/env python
"""

usage:
python build_github_actions.py --godot-version "3.4-stable" --godot-github-folder ../../.github --ECMAS-github-folder .github
"""

import argparse
import yaml
import os
import subprocess
from dataclasses import dataclass


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

    workflows = {
        "android_builds.yml": BuildOpts("module_text_server_fb_enabled=yes verbose=yes", args.godot_version),
        "ios_builds.yml": BuildOpts("", args.godot_version),
        "javascript_builds.yml": BuildOpts("", args.godot_version),
        "linux_builds.yml": BuildOpts("", args.godot_version),
        "macos_builds.yml": BuildOpts("", args.godot_version),
        "server_builds.yml": BuildOpts("", args.godot_version),
        # "static_checks.yml": BuildOpts("", args.godot_version),
        "windows_builds.yml": BuildOpts("", args.godot_version),
    }
    for wf_base_fn, build_opts in workflows.items():
        full_fn = os.path.join(args.ECMAS_github_folder, "workflows", wf_base_fn)
        data = yaml.safe_load(open(full_fn))
        data["env"]["SCONSFLAGS"] = build_opts.SCONSFLAGS
        data["env"]["GODOT_BASE_BRANCH"] = build_opts.GODOT_BASE_BRANCH
        assert len(data["jobs"]) == 1
        only_template_name = list(data["jobs"].keys())[0]
        new_steps = []
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
            yaml.safe_dump(data, fh)


if __name__ == "__main__":
    main()
