# Contributing

## Project Structure

TODO

## Preparing your PR

The project is using [`clang-format`](https://clang.llvm.org/docs/ClangFormat.html) to format most files. You need to run `bash ./clang_format.sh` before your PR for a successful pipeline.

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

