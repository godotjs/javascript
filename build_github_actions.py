#!/usr/bin/env python
"""

usage:

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
    parser.add_argument("--ECMSA-github-folder", required=True)
    return parser.parse_args()


def main():
    args = parseargs()
    assert os.path.isdir(args.godot_github_folder)
    assert os.path.isdir(args.ECMSA_github_folder)

    for x in ["actions", "workflows"]:
        subprocess.call(["rm", "-rf", os.path.join(args.ECMSA_github_folder, x)])
        subprocess.call(
            ["cp", "-r", os.path.join(args.godot_github_folder, x), os.path.join(args.ECMSA_github_folder, x)]
        )
    workflows = {
        "android_builds.yml": BuildOpts("", args.godot_version),
        "ios_builds.yml": BuildOpts("", args.godot_version),
        "javascript_builds.yml": BuildOpts("", args.godot_version),
        "linux_builds.yml": BuildOpts("", args.godot_version),
        "macos_builds.yml": BuildOpts("", args.godot_version),
        "server_builds.yml": BuildOpts("", args.godot_version),
        # "static_checks.yml": BuildOpts("", args.godot_version),
        "windows_builds.yml": BuildOpts("", args.godot_version),
    }
    for wf_base_fn, build_opts in workflows.items():
        full_fn = os.path.join(args.ECMSA_github_folder, "workflows", wf_base_fn)
        data = yaml.safe_load(open(full_fn))

        pass


if __name__ == "__main__":
    main()
