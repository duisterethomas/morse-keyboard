#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <optional>

#include "version.h"

const std::string config_filename = "config.yaml";

struct morse_code {
	int key_code;
	std::string key;
	std::optional<bool> forced_shift_state;
};

const std::unordered_map<std::string, morse_code> morse_codes = {
    {".-", morse_code{KEY_A, "A"}},
	{"-...", morse_code{KEY_B, "B"}},
	{"-.-.", morse_code{KEY_C, "C"}},
	{"-..", morse_code{KEY_D, "D"}},
	{".", morse_code{KEY_E, "E"}},
	{"..-.", morse_code{KEY_F, "F"}},
	{"--.", morse_code{KEY_G, "G"}},
	{"....", morse_code{KEY_H, "H"}},
	{"..", morse_code{KEY_I, "I"}},
	{".---", morse_code{KEY_J, "J"}},
	{"-.-", morse_code{KEY_K, "K"}},
	{".-..", morse_code{KEY_L, "L"}},
	{"--", morse_code{KEY_M, "M"}},
	{"-.", morse_code{KEY_N, "N"}},
	{"---", morse_code{KEY_O, "O"}},
	{".--.", morse_code{KEY_P, "P"}},
	{"--.-", morse_code{KEY_Q, "Q"}},
	{".-.", morse_code{KEY_R, "R"}},
	{"...", morse_code{KEY_S, "S"}},
	{"-", morse_code{KEY_T, "T"}},
	{"..-", morse_code{KEY_U, "U"}},
	{"...-", morse_code{KEY_V, "V"}},
	{".--", morse_code{KEY_W, "W"}},
	{"-..-", morse_code{KEY_X, "X"}},
	{"-.--", morse_code{KEY_Y, "Y"}},
	{"--..", morse_code{KEY_Z, "Z"}},
	{"-----", morse_code{KEY_0, "0", false}},
	{".----", morse_code{KEY_1, "1", false}},
	{"..---", morse_code{KEY_2, "2", false}},
	{"...--", morse_code{KEY_3, "3", false}},
	{"....-", morse_code{KEY_4, "4", false}},
	{".....", morse_code{KEY_5, "5", false}},
	{"-....", morse_code{KEY_6, "6", false}},
	{"--...", morse_code{KEY_7, "7", false}},
	{"---..", morse_code{KEY_8, "8", false}},
	{"----.", morse_code{KEY_9, "9", false}},
	{".-.-.-", morse_code{KEY_DOT, ".", false}},
	{"--..--", morse_code{KEY_COMMA, ",", false}},
	{"---...", morse_code{KEY_SEMICOLON, ":", true}},
	{"..--..", morse_code{KEY_SLASH, "?", true}},
	{".----.", morse_code{KEY_APOSTROPHE, "'", false}},
	{"-....-", morse_code{KEY_MINUS, "-", false}},
	{"-..-.", morse_code{KEY_SLASH, "/", false}},
	{"-.--.", morse_code{KEY_9, "(", true}},
	{"-.--.-", morse_code{KEY_0, ")", true}},
	{".-..-.", morse_code{KEY_APOSTROPHE, "\"", true}},
	{"-...-", morse_code{KEY_EQUAL, "=", false}},
	{".-.-.", morse_code{KEY_EQUAL, "+", true}},
	{".--.-.", morse_code{KEY_2, "@", true}}
};

struct KeyboardDevice {
    std::string path;
    std::string name;
};

std::vector<KeyboardDevice> list_keyboards() {
    std::vector<KeyboardDevice> keyboards;

	std::string input_directory = "/dev/input";
	if (std::filesystem::exists("/dev/input/by-id")) {
		input_directory = "/dev/input/by-id";
	}

    for (const auto& entry : std::filesystem::directory_iterator(input_directory.c_str())) {
        std::string path = entry.path().string();
        if (path.find("event") == std::string::npos) {
            continue;
        }

        int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            continue;
        }

        struct libevdev *dev = nullptr;
        if (libevdev_new_from_fd(fd, &dev) < 0) {
            close(fd);
            continue;
        }

        if (libevdev_has_event_code(dev, EV_KEY, KEY_SPACE)) {
            keyboards.push_back({path, libevdev_get_name(dev)});
        }

        libevdev_free(dev);
        close(fd);
    }

	std::sort(
		keyboards.begin(), keyboards.end(),
		[](const KeyboardDevice& a, const KeyboardDevice& b) {
			return a.name < b.name;
		}
	);

    return keyboards;
}

void save_config(const std::string& filename, const std::string& keyboard_path, int long_threshold, int space_threshold, int end_threshold) {
	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Comment(
		"Set the path to the keyboard device below\n"
		"It is the easiest to look for a device ending with \"-event-kbd\" in \"/dev/input/by-id/\"\n"
		"If that directory doesn't exist you'll have to find another way to get the right keyboard device in \"/dev/input/\""
	);
	out << YAML::Key << "keyboard" << YAML::Value << keyboard_path;
	out << YAML::Newline;
	out << YAML::Newline;
	out << YAML::Key << "long_threshold" << YAML::Value << long_threshold << YAML::Comment("Duration in milliseconds the spacebar must be held to be considered a long press");
	out << YAML::Key << "space_threshold" << YAML::Value << space_threshold << YAML::Comment("Duration in milliseconds the spacebar must be held to insert a space");
	out << YAML::Key << "end_threshold" << YAML::Value << end_threshold << YAML::Comment("Duration in milliseconds after the last Morse input to convert the Morse sequence to a key press");
	out << YAML::EndMap;

	std::ofstream fout(config_filename);
	if (!fout) {
		throw std::runtime_error("Failed to open file for writing: " + config_filename);
	}
	fout << out.c_str();
	fout.close();
}

int main() {
	std::cout << "Morse Keyboard v" << PROJECT_VERSION << "\n\n";

	// Create the config yaml if it doesn't exist
	if (!std::filesystem::exists(config_filename)) {
		save_config(
			config_filename,
			"",
			150,
			400,
			300
		);

		std::cout << "Config file generated at: " << std::filesystem::absolute(config_filename) << "\n\n";
	}

	// Load the config yaml
	YAML::Node config = YAML::LoadFile(config_filename);

	std::string keyboard_path = config["keyboard"].as<std::string>();
	int long_threshold = config["long_threshold"].as<int>();
	int space_threshold = config["space_threshold"].as<int>();
	int end_threshold = config["end_threshold"].as<int>();

	if (keyboard_path.empty()) {
		// Find all keyboards
		std::vector<KeyboardDevice> keyboards = list_keyboards();

		// List all keyboards to user
		for (size_t i = 0; i < keyboards.size(); ++i) {
			std::cout << i << ": " << keyboards[i].name << " [" << keyboards[i].path << "]\n";
		}

		// Ask user to select a keyboard
		int keyboard_index;
		std::cout << "Select keyboard: ";
		std::cin >> keyboard_index;
		std::cout << "\n";

		// Select and save the keyboard
		keyboard_path = keyboards[keyboard_index].path;
		config["keyboard"] = keyboard_path;

		save_config(config_filename, keyboard_path, long_threshold, space_threshold, end_threshold);
	}

    // Open the physical keyboard device
    int fd = open(keyboard_path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        throw std::runtime_error("Failed to open input device");
    }

    struct libevdev *dev = nullptr;
    int rc = libevdev_new_from_fd(fd, &dev);
    if (rc < 0) {
        throw std::runtime_error("Failed to init libevdev");
    }

	std::cout << "Using " << libevdev_get_name(dev) << " [" << keyboard_path << "]\n\n";

	// Wait 3 seconds to prevent keys getting stuck
	std::cout << "Release all keys on the keyboard...\n\n";
	usleep(3000000);

    // Grab exclusive access
    rc = libevdev_grab(dev, LIBEVDEV_GRAB);
    if (rc < 0) {
        throw std::runtime_error("Failed to grab device");
    }

    // Create a virtual uinput device that mirrors the original keyboard and sends the morse output
    struct libevdev_uinput *uidev = nullptr;
    rc = libevdev_uinput_create_from_device(
        dev,
        LIBEVDEV_UINPUT_OPEN_MANAGED,
        &uidev
    );
    if (rc < 0) {
        throw std::runtime_error("Failed to create uinput device");
    }

	std::cout << "Your spacebar is now the morse input!\nHold it for " << space_threshold << " milliseconds to enter a space\nYou can stop Morse Keyboard by pressing CTRL + C in this terminal\n\n";

	std::chrono::steady_clock::time_point space_start;
	std::chrono::steady_clock::time_point space_end;
	bool space_pressed = false;
	bool space_released = false;
	std::string received_morse;

	bool leftshift_pressed = false;
	bool rightshift_pressed = false;

    while (true) {
        struct input_event ev;
        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            if (ev.type == EV_KEY) {
				if (ev.code == KEY_SPACE) {
					if (ev.value == 1) {
						space_start = std::chrono::steady_clock::now();
						space_pressed = true;
						space_released = false;
					} else if (ev.value == 0 && space_pressed) {
						space_end = std::chrono::steady_clock::now();
						space_pressed = false;
						space_released = true;

						std::chrono::duration<double, std::milli> space_duration = space_end - space_start;
						if (space_duration.count() > long_threshold) {
							received_morse += "-";
						} else {
							received_morse += ".";
						}
					}

					continue;

				} else if (ev.code == KEY_LEFTSHIFT) {
					leftshift_pressed = ev.value != 0;

				} else if (ev.code == KEY_RIGHTSHIFT) {
					rightshift_pressed = ev.value != 0;
				}
            }

            // Forward every event that isn't space
            libevdev_uinput_write_event(
                uidev,
                ev.type,
                ev.code,
                ev.value
            );

        } else if (rc == -EAGAIN) {
            usleep(1000);
        }

		// Allow space on long press
		if (space_pressed) {
			std::chrono::duration<double, std::milli> space_duration = std::chrono::steady_clock::now() - space_start;
			if (space_duration.count() > space_threshold) {
				space_pressed = false;
				received_morse = "";

				libevdev_uinput_write_event(
					uidev,
					EV_KEY,
					KEY_SPACE,
					1
				);
				libevdev_uinput_write_event(
					uidev,
					EV_SYN,
					SYN_REPORT,
					0
				);
				usleep(50000);
				libevdev_uinput_write_event(
					uidev,
					EV_KEY,
					KEY_SPACE,
					0
				);
				libevdev_uinput_write_event(
					uidev,
					EV_SYN,
					SYN_REPORT,
					0
				);
			}
		}

		// Send the keystroke
		if (space_released) {
			std::chrono::duration<double, std::milli> space_release_duration = std::chrono::steady_clock::now() - space_end;
			if (space_release_duration.count() > end_threshold) {
				space_released = false;

				std::cout << received_morse;

				auto it = morse_codes.find(received_morse);

				if (it != morse_codes.end()) {
					if (it->second.forced_shift_state.has_value()) {
						libevdev_uinput_write_event(
							uidev,
							EV_KEY,
							KEY_LEFTSHIFT,
							static_cast<int>(*it->second.forced_shift_state)
						);
						libevdev_uinput_write_event(
							uidev,
							EV_KEY,
							KEY_RIGHTSHIFT,
							static_cast<int>(*it->second.forced_shift_state)
						);
						libevdev_uinput_write_event(
							uidev,
							EV_SYN,
							SYN_REPORT,
							0
						);
						usleep(5000);
					}

					libevdev_uinput_write_event(
						uidev,
						EV_KEY,
						it->second.key_code,
						1
					);
					libevdev_uinput_write_event(
						uidev,
						EV_SYN,
						SYN_REPORT,
						0
					);
					usleep(50000);
					libevdev_uinput_write_event(
						uidev,
						EV_KEY,
						it->second.key_code,
						0
					);

					if (it->second.forced_shift_state.has_value()) {
						libevdev_uinput_write_event(
							uidev,
							EV_KEY,
							KEY_LEFTSHIFT,
							static_cast<int>(leftshift_pressed)
						);
						libevdev_uinput_write_event(
							uidev,
							EV_KEY,
							KEY_RIGHTSHIFT,
							static_cast<int>(rightshift_pressed)
						);
					}

					libevdev_uinput_write_event(
						uidev,
						EV_SYN,
						SYN_REPORT,
						0
					);

					std::cout << " -> " << it->second.key << "\n";
				} else {
					std::cout << " Invalid morse\n";
				}

				received_morse = "";
			}
		}
    }

    libevdev_uinput_destroy(uidev);
    libevdev_free(dev);
    close(fd);

    return 0;
}
