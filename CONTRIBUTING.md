# Contributing

## Project Structure

- [Clang](https://clang.llvm.org/): Code formatting
  - ``.clang-format``
  - ``.clang-tidy``
  - ``clang-format.sh``
- [Custom Modules in C++](https://docs.godotengine.org/en/stable/contributing/development/core_and_modules/custom_modules_in_cpp.html#custom-modules-in-c)
  - ``config.py``: Configs for this module
  - ``doc``: For showing documentation in editor
  - ``doc_classes``: For showing documentation in editor
  - ``icons``: For showing icons in editor
  - ``SCsub``: Will be called from Godots `SConstruct` during build
  - ``editor``: Custom `.cpp` only bundled for `target=editor`
  - ``misc``: Scripts and other files, which aren't related to `c++`
  - ``tests``: Some testcases to run in CICD
  - ``thirdparty``: Dependencies or libraries which shouldn't be analysed by static checks
- GodotJS custom data
  - ``.github``: Runs custom CI/CD in GitHub
  - ``docs``: Add some additional stuff for README.md
  - ``src``: Contains custom C++ code
    - [language](src/language/README.md)


## Preparing your PR

The project is using [`clang-format`](https://clang.llvm.org/docs/ClangFormat.html) to format C++ files. 
You need to run `bash ./misc/formatting/clang_format.sh` before your PR for a successful pipeline.

Furthermore, there is an `utf-8` and `LF` checker to fix file formats. Additionally, some spellchecks run inside the [pipeline](.github/workflows/static_checks.yml).

## Release library

As a maintainer you are able to create a new release.
1. Go to  [create new release](https://github.com/godotjs/javascript/releases/new)
2. Next you should ``Choose a tag`` and create a new one in the format `v0.0.0`
3. Select target ``master``
4. Click ``Generate release notes``
5. (Optional) You can edit the generated release notes
6. Add a release title in the format `v0.0.0`
7. Press ``Publish release``

