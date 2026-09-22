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
### Setting up permissions
Morse Keyboard grabs your keyboard directly and creates a virtual
keyboard to send the Morse output and forward non-intercepted keys.
For this I use `libevdev` which requires access to `/dev/input` and
`/dev/uinput`, which normal users don't have by default.

**Option 1: Run as root**
```bash
sudo ./morse-keyboard
```

**Option 2: Add yourself to the `input` group and set up a udev rule (recommended)**
1. Add yourself to the `input` group
   ```bash
   sudo usermod -aG input $USER
   ```
2. Create `/etc/udev/rules.d/99-uinput.rules` with:
   ```
   KERNEL=="uinput", GROUP="input", MODE="0660"
   ```
3. Reload udev rules and log out and back in for the group change to apply
   ```bash
   sudo udevadm control --reload-rules
   ```

### Running Morse Keyboard
1. Download the [latest release](https://github.com/duisterethomas/morse-keyboard/releases/latest)
2. Make `morse-keyboard` executable: `chmod +x morse-keyboard`
3. Run `./morse-keyboard`

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
