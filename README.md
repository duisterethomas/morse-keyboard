# Morse Keyboard
A wacky project to turn your spacebar into a Morse keyboard input on Linux.

# Usage
## Dependencies
- yaml-cpp
- libevdev

### Arch
```bash
sudo pacman -S --needed yaml-cpp libevdev
```

## Running
1. Download the [latest release](https://github.com/duisterethomas/morse-keyboard/releases/latest)
2. Run `./morse-keyboard` once to generate the config file
3. Edit `config.yaml` and set the full path to the keyboard device after `keyboard:`.
   The easiest way to find your keyboard is to look for a device ending with `-event-kbd` in `/dev/input/by-id/`.
   If that directory doesn't exist you'll have to find another way to get the right keyboard event device in `/dev/input`.
4. Run `./morse-keyboard`
_To be able to run Morse Keyboard either your user has to be in the `input` group, or Morse Keyboard has to be run as root._

You can stop Morse Keyboard by pressing `CTRL` + `C` in the terminal it's running in.

## Morse interpretation
By default the Morse input is interpreted like this:
- Space press duration < 150ms = Short press
- Space press duration >= 150ms = Long press
- Space press duration >= 400ms = Insert a space
- Space not pressed for 300ms = Convert Morse to key press

All of these values can be configured in the `config.yaml` config.

_Please note that this project is designed for the en_us keyboard layout. So if you use any other layout the Morse might not map correctly._

# Building
## Build dependencies
- cmake
- ninja
- yaml-cpp
- pkg-config
- libevdev

### Arch
```bash
sudo pacman -S --needed cmake ninja yaml-cpp pkgconf libevdev
```

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

# Morse source
The Morse standard used in this project is defined on [this ITU page](https://www.itu.int/rec/R-REC-M.1677-1-200910-I/).
