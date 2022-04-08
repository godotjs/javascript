#!/usr/bin/env python
"""
run this every time you upgrade the godot-base version to generate new matching github workflows
You must be in this directory, and in the modules subfolder of godot (just as if you would install this project into godot)

usage:
python build_github_actions.py --godot-version "3.4.4-stable" --godot-github-folder ../../.github --ECMAS-github-folder .github

"""

import argparse
import yaml
import os
import subprocess
from dataclasses import dataclass, field
from typing import Dict, List, Any
import copy

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


def checkout_local_godot_install(tag: str):
    cmd = ["git", "checkout", f"tags/{tag}"]
    ret = subprocess.run(cmd, cwd="../../")
    if ret.returncode != 0:
        raise RuntimeError(f"godot not setup properly, could not checkout '{' '.join(cmd)}'")


def get_windows_mingw_checkout_steps() -> List[Dict[str, Any]]:
    out = [
        {
            "name": "setup-msys2",
            "uses": "msys2/setup-msys2@v2",
            "with": {"msystem": "MINGW64", "update": True, "install": "mingw-w64-x86_64-gcc"},
        },
        {
            "name": "update mingw2",
            "run": "pacman -Syu --needed --noconfirm mingw-w64-x86_64-python3-pip mingw-w64-x86_64-gcc mingw-w64-i686-python3-pip mingw-w64-i686-gcc make",
        },
        {
            "name": "update scons",
            "run": "pip3 install scons",
        },
    ]
    return out


def get_ECMAScript_checkout_steps() -> List[Dict[str, Any]]:
    out = [
        {
            "name": "Checkout Godot",
            "uses": "actions/checkout@v2",
            "with": {"repository": "godotengine/godot", "ref": "${{ env.GODOT_BASE_BRANCH }}"},
        },
        {
            "name": "Checkout ECMAScript",
            "uses": "actions/checkout@v2",
            "with": {"path": "${{github.workspace}}/modules/ECMAScript/"},
        },
    ]
    return out


def get_rid_of_ubsan_asan_linux(matrix_step: Dict[str, Any]) -> Dict[str, Any]:
    for get_rid_of in ["use_ubsan=yes", "use_asan=yes"]:
        matrix_step["name"] = matrix_step["name"].replace(get_rid_of, "").replace(" , ", " ").replace(", )", ")")
        matrix_step["sconsflags"] = matrix_step["sconsflags"].replace(get_rid_of, "").replace(", )", ")")
    return matrix_step


def fix_all_workflows(
    ECMAS_github_folder: str, workflows: Dict[str, BuildOpts], wf_actions_that_require_shell: List[str]
) -> List[str]:
    wf_names: List[str] = []
    for wf_base_fn, build_opts in workflows.items():
        full_fn = os.path.join(ECMAS_github_folder, "workflows", wf_base_fn)
        data = yaml.safe_load(open(full_fn))
        wf_names.append(data["name"])

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
            # quickjs can't build under msvc, must use mingw, install it here
            new_steps += get_windows_mingw_checkout_steps()
            data["jobs"][only_template_name]["defaults"] = {"run": {"shell": "msys2 {0}"}}

        elif "linux" in wf_base_fn:
            for matrix_step in data["jobs"][only_template_name]["strategy"]["matrix"]["include"]:
                # quickjs fails under ubsan & asan, don't include those flags
                if "name" in matrix_step and "Editor and sanitizers" in matrix_step["name"]:
                    matrix_step = get_rid_of_ubsan_asan_linux(matrix_step)

        base_github_string = "./.github/"
        for step in data["jobs"][only_template_name]["steps"]:
            # replace godot checkout routine with this checkout routine
            if "uses" in step and "checkout" in step["uses"]:
                new_steps += get_ECMAScript_checkout_steps()
            elif (
                "uses" in step
                and base_github_string in step["uses"]
                and any(x in step["uses"] for x in wf_actions_that_require_shell)
            ):
                step["uses"] = step["uses"].replace(base_github_string, "./modules/ECMAScript/.github/")
                to_add = {"shell": "msys2 {0}" if "windows" in wf_base_fn else "sh"}
                if "with" not in step:
                    step["with"] = to_add
                else:
                    step["with"].update(to_add)
                new_steps.append(step)
            else:
                new_steps.append(step)

        data["jobs"][only_template_name]["steps"] = new_steps
        with open(full_fn, "w") as fh:
            yaml.dump(data, fh, sort_keys=False, allow_unicode=True)
    return wf_names


def fix_all_actions(ECMAS_github_folder: str, actions: List[str]) -> List[str]:
    """
    This can be simplified once:
    https://github.com/actions/runner/pull/1767
    is completed
    """
    actions_that_require_shell_set = set()
    for action_base_fn in actions:
        full_action_fn = os.path.join(ECMAS_github_folder, action_base_fn)
        data = yaml.safe_load(open(full_action_fn))
        new_steps = []
        for step in data["runs"]["steps"]:
            if "shell" in step:
                for shell in ["sh", "msys2 {0}"]:
                    cp_step = copy.deepcopy(step)
                    cp_step["shell"] = shell
                    cp_step["if"] = f"inputs.shell == '{shell}'"
                    new_steps.append(cp_step)
                data["inputs"]["shell"] = {"description": "the shell to run this under", "default": "sh"}
                actions_that_require_shell_set.add(action_base_fn)
            else:
                new_steps.append(step)
            # new_steps.append(step)
            # Uncomment this when github actions updated
            # if "shell" in step:
            #     step["shell"] = "${{ inputs.shell }}"
            #     data["inputs"]["shell"] = {"description": "the shell to run this under", "default": "sh"}
            # new_steps.append(step)

            # We ca
        data["runs"]["steps"] = new_steps
        with open(full_action_fn, "w") as fh:
            yaml.dump(data, fh, sort_keys=False, allow_unicode=True)
    return list(sorted([x.split("/")[1] for x in actions_that_require_shell_set]))


def add_publish_workflow(out_fn: str, wf_name_list: List[str]):
    # "on": {"tag":  "",   "workflow_run": {"workflows": wf_name_list, "types": ["completed"]}},
    # run_id: ${{ github.event.workflow_run.id }},
    # var matchArtifact = artifacts.data.artifacts.filter((artifact) => {
    #   return artifact.name == "pr"
    # })[0];

    script_text = (
        """var total_slept = 0;
var downloaded_files = [];
var seen_completed_wfs = [];
var expected_to_see = """
        + str(len(wf_name_list))
        + """;
while (total_slept < 3600000 && seen_completed_wfs.length < expected_to_see) {
	var all_workflows = await github.rest.actions.listWorkflowRunsForRepo({
		owner: context.repo.owner,
		repo: context.repo.repo,
	});
	console.log("Expecting to download from " + expected_to_see + " workflows, currently at " + seen_completed_wfs.length ". Have already downloaded " + downloaded_files.length + " files as " + downloaded_files);
	for (const workflow of all_workflows.data.workflow_runs) {
		if (workflow.head_sha == "${{ github.sha }}") {
			console.log("found " + workflow.name + " " + workflow.status);
			if (workflow.status == "completed") {
				if (seen_completed_wfs.includes(workflow.name)) {continue;}
				if (workflow.conclusion == "success") {
					var artifacts = await github.rest.actions.listWorkflowRunArtifacts({
						owner: context.repo.owner,
						repo: context.repo.repo,
						run_id: workflow.id,
						per_page: 100,
					});
					for (const artifact of artifacts.data.artifacts) {
						var fn = '${{github.workspace}}/' + artifact.name + '.zip';
						if (downloaded_files.includes(fn)) {continue;}
						var download = await github.rest.actions.downloadArtifact({
							owner: context.repo.owner,
							repo: context.repo.repo,
							artifact_id: artifact.id,
							archive_format: 'zip',
						});
						var fs = require('fs');
						fs.writeFileSync(fn, Buffer.from(download.data));
						downloaded_files.push(fn);
					}
					seen_completed_wfs.push(workflow.name);
				}
			}
		}
	}
	if (seen_completed_wfs.length < expected_to_see) {
		console.log("sleeping " + 300000);
		await new Promise(r => setTimeout(r, 300000));
		total_slept = total_slept + 300000;
		console.log("done sleeping " + 300000);
	}
}
console.log(downloaded_files);"""
    )

    data = {
        "name": "🚢 Publish release",
        # You should tag as late as possible, don't want to sleep too long and get push job killed
        "on": {
            "push": {"tags": ["*"]},
        },
        "jobs": {
            "collect-template": {
                "runs-on": "ubuntu-latest",
                "steps": [
                    # {"name": "show dir", "run": "sleep 900"},
                    {
                        "name": "download artifacts",
                        "uses": "actions/github-script@v6",
                        "if": "startsWith(github.ref, 'refs/tags')",
                        "with": {"script": script_text},
                    },
                    {"name": "show dir", "run": "ls -R"},
                    {
                        "name": "Upload binaries to release",
                        "uses": "svenstaro/upload-release-action@v2",
                        "if": "startsWith(github.ref, 'refs/tags')",
                        "with": {
                            "repo_token": "${{ secrets.GITHUB_TOKEN }}",
                            "file": "*.zip",
                            "tag": "${{ github.ref }}",
                            "overwrite": "true",
                            "file_glob": "true",
                        },
                    },
                ],
            }
        },
    }

    with open(out_fn, "w") as fh:
        yaml.dump(data, fh, sort_keys=False, allow_unicode=True)


def main():
    args = parseargs()
    assert os.path.isdir(args.godot_github_folder)
    assert os.path.isdir(args.ECMAS_github_folder)
    checkout_local_godot_install(args.godot_version)

    for x in ["actions", "workflows"]:
        subprocess.call(["rm", "-rf", os.path.join(args.ECMAS_github_folder, x)])
        subprocess.call(
            ["cp", "-r", os.path.join(args.godot_github_folder, x), os.path.join(args.ECMAS_github_folder, x)]
        )

    basic_flags = " "
    actions = [
        "actions/godot-build/action.yml",
        "actions/godot-cache/action.yml",
        "actions/godot-deps/action.yml",
        "actions/upload-artifact/action.yml",
    ]
    wf_actions_that_require_shell = fix_all_actions(args.ECMAS_github_folder, actions)
    workflows = {
        "android_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "ios_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "javascript_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "linux_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "macos_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "server_builds.yml": BuildOpts(basic_flags, args.godot_version),
        "windows_builds.yml": BuildOpts(f"{basic_flags} use_mingw=yes", args.godot_version),
    }
    wf_names = fix_all_workflows(args.ECMAS_github_folder, workflows, wf_actions_that_require_shell)
    subprocess.call(["rm", os.path.join(args.ECMAS_github_folder, "workflows", "static_checks.yml")])

    out_publish_fn = os.path.join(args.ECMAS_github_folder, "workflows", "on_tag.yml")
    add_publish_workflow(out_publish_fn, wf_names)


if __name__ == "__main__":
    main()
