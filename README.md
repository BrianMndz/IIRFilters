# Audio-Plugin-Template

This is a template repository for a JUCE project using CMake and Xcode.
_Based on the Pamplejuce by Sudara but for beginners._

## Getting Started
git clone your new repo

```
git clone --recurse-submodules my-cloned-repo
```

Update the submodules
```sh
git submodule update --init --recursive
cmake -G Xcode -H. -B Xcode
````

or
```sh
cmake -B Builds -G Xcode
````