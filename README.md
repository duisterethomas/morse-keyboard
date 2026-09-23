# Morse Keyboard
A wacky project to turn any key on your keyboard into a Morse keyboard input on Linux.

# Usage
## Dependencies
- yaml-cpp
- libevdev

### Arch Linux
```bash
sudo pacman -S --needed yaml-cpp libevdev
```

## Setting up permissions
Morse Keyboard grabs your keyboard directly and creates a virtual
keyboard to send the Morse output and forward non-intercepted keys.
For this I use `libevdev` which requires access to `/dev/input` and
`/dev/uinput`, which normal users don't have by default.

### Option 1: Run as root
This is the simpler option. Just run `morse-keyboard` as root at step 4 of the [running instructions](#running).
```bash
sudo ./morse-keyboard
```

### Option 2: Set up a udev rule (recommended)
A bit more complicated than option 1, but once set up you never have to run `morse-keyboard` as root.
1. Add your user to the `input` group
   ```bash
   sudo usermod -aG input $USER
   ```
2. Create `/etc/udev/rules.d/99-uinput.rules` with the following contents:
   ```
   KERNEL=="uinput", GROUP="input", MODE="0660"
   ```
3. Reload udev rules
   ```bash
   sudo udevadm control --reload-rules
   ```
4. Log out and back in for the group change to apply

## Running
1. Download the [latest release](https://github.com/duisterethomas/morse-keyboard/releases/latest)
2. Make `morse-keyboard` executable: `chmod +x morse-keyboard`
3. Make sure the `uinput` kernel module is loaded with `sudo modprobe uinput`
4. Run `./morse-keyboard`

You can stop Morse Keyboard by pressing `CTRL` + `C` in the terminal it's running in.

## Morse interpretation
By default the Morse input is interpreted like this:
- Morse key press duration < 150ms = Short press
- Morse key press duration >= 150ms = Long press
- Morse key press duration >= 400ms = Press and release the original key
- Morse key not pressed for 300ms = Convert Morse to key press

All of these values can be configured in the `config.yaml` config.

_Please note that this project is designed for the en_us keyboard layout. So if you use any other layout the Morse might not map correctly._

# Building
## Build dependencies
- cmake
- gcc
- g++
- ninja
- yaml-cpp
- pkg-config
- libevdev

### Arch Linux (based)
```bash
sudo pacman -S --needed base-devel cmake ninja yaml-cpp libevdev
```

### Debian/Ubuntu (based)
```bash
sudo apt install build-essential cmake ninja-build libyaml-cpp-dev pkgconf libevdev-dev
```

### Fedora (based)
```bash
sudo dnf install cmake gcc gcc-c++ ninja-build yaml-cpp-devel pkgconf libevdev-devel
```

### Alpine Linux (based)
```bash
sudo apk add cmake gcc g++ ninja yaml-cpp-dev pkgconf libevdev-dev
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
