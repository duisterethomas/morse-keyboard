# morse-keyboard
A wacky project to turn your spacebar into a morse keyboard input on Linux

# Building
## Dependencies
- cmake
- ninja
- yaml-cpp
- pkg-config
- libevdev

## Setting up the build environment
```bash
git clone https://github.com/duisterethomas/morse-keyboard
cd morse-keyboard
mkdir build
cd build
cmake -G Ninja ..
```
_You probably don't need to use Ninja, I just like using it because it simplifies building into only needing to run `ninja`._

## Building
In the `build` directory you've made above simply run `ninja` to build.
