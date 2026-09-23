#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <unistd.h>

#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/yaml.h>

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

std::vector<KeyboardDevice> list_keyboards(const std::string& input_directory, const int& morse_key_code) {
    std::vector<KeyboardDevice> keyboards;

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

        if (libevdev_has_event_code(dev, EV_KEY, morse_key_code)) {
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

void save_config(const std::string& filename, const std::string& keyboard_path, const std::string& morse_key, const int& long_threshold, const int& original_key_threshold, const int& end_threshold) {
	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Comment(
		"The path to the keyboard device\n"
		"Remove the line below to get the keyboard selection prompt when running morse-keyboard"
	);
	out << YAML::Key << "keyboard" << YAML::Value << keyboard_path;
	out << YAML::Newline;
	out << YAML::Newline;
	out << YAML::Comment(
		"The keyboard key to use for the Morse input\n"
		"See https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/uapi/linux/input-event-codes.h#n69 for all valid keys.\n"
		"Anything in that file starting with \"KEY_\" should be valid"
	);
	out << YAML::Key << "morse_key" << YAML::Value << morse_key;
	out << YAML::Newline;
	out << YAML::Newline;
	out << YAML::Key << "long_threshold" << YAML::Value << long_threshold << YAML::Comment("Duration in milliseconds the Morse key must be held to be considered a long press");
	out << YAML::Key << "original_key_threshold" << YAML::Value << original_key_threshold << YAML::Comment("Duration in milliseconds the Morse key must be held to press and release the original key");
	out << YAML::Key << "end_threshold" << YAML::Value << end_threshold << YAML::Comment("Duration in milliseconds after the last Morse input to convert the Morse sequence to a key press");
	out << YAML::EndMap;

	std::ofstream fout(config_filename);
	if (!fout) {
		throw std::runtime_error(
			"Failed to open file for writing: " +
			std::filesystem::absolute(config_filename).string() +
        	" (" + std::strerror(errno) + ")"
		);
	}
	fout << out.c_str();
	fout.close();
}

template <typename T>
bool load_config_node(YAML::Node& config, const char* node, T* variable) {
	if (config[node]) {
		try {
			*variable = config[node].as<T>();
			return true;
		} catch (const YAML::BadConversion&) {
			std::cerr << "The value of \"" << node << "\" is invalid, reverting to default: " << *variable << "\n";
			return false;
		}
	} else {
		std::cerr << "\"" << node << "\" entry could not be found in " << std::filesystem::absolute(config_filename) << ", reverting to default: " << *variable << "\n";
		return false;
	}
}

int main() {
	std::cout << "Morse Keyboard v" << PROJECT_VERSION << "\n\n";

	// Set config defaults
	std::string keyboard_path;
	std::string morse_key = "KEY_SPACE";
	int long_threshold = 150;
	int original_key_threshold = 400;
	int end_threshold = 300;

	// Create the config yaml if it doesn't exist
	if (!std::filesystem::exists(config_filename)) {
		try {
			save_config(
				config_filename,
				keyboard_path,
				morse_key,
				long_threshold,
				original_key_threshold,
				end_threshold
			);
		} catch (const std::runtime_error& e) {
			std::cerr << e.what() << "\n";
			return EXIT_FAILURE;
		}

		std::cout << "Config file generated at: " << std::filesystem::absolute(config_filename) << "\n\n";
	}

	// Load the config yaml
	YAML::Node config = YAML::LoadFile(config_filename);

	bool config_changed = false;

	if (!load_config_node(config, "keyboard", &keyboard_path)) config_changed = true;
	if (!load_config_node(config, "morse_key", &morse_key)) config_changed = true;
	if (!load_config_node(config, "long_threshold", &long_threshold)) config_changed = true;
	if (!load_config_node(config, "original_key_threshold", &original_key_threshold)) config_changed = true;
	if (!load_config_node(config, "end_threshold", &end_threshold)) config_changed = true;

	if (config_changed) std::cout << "\n";

	// Try to convert key to key code
	int morse_key_code = libevdev_event_code_from_name(EV_KEY, morse_key.c_str());
	if (morse_key_code == -1) {
		std::cerr << morse_key << " is an invalid key code, reverting to default: KEY_SPACE\n\n";

		morse_key = "KEY_SPACE";
		morse_key_code = KEY_SPACE;

		config_changed = true;
	}

	// Ask the user to set a keyboard if not set or invalid
	if (keyboard_path.empty() || !std::filesystem::exists(keyboard_path)) {
		if (!keyboard_path.empty() && !std::filesystem::exists(keyboard_path)) {
			std::cerr << "\"" << keyboard_path << "\": " << std::strerror(ENOENT) << ", please select a new keyboard\n";
		}

		// Find all keyboards
		std::vector<KeyboardDevice> keyboards;
		// First by id
		if (std::filesystem::exists("/dev/input/by-id")) {
			keyboards = list_keyboards("/dev/input/by-id", morse_key_code);
		}

		// If none found try in /dev/input
		if (keyboards.empty()) {
			keyboards = list_keyboards("/dev/input", morse_key_code);
		}

		if (keyboards.empty()) {
			std::cerr << "No keyboards were found, have you set up the permissions?\n";
			return EXIT_FAILURE;
		}

		// Ask user to select a keyboard
		int keyboard_index;
		while (true) {
			// List all keyboards to user
			for (size_t i = 0; i < keyboards.size(); ++i) {
				std::cout << i << ": " << keyboards[i].name << " [" << keyboards[i].path << "]\n";
			}

			std::cout << "Select a keyboard (0-" << keyboards.size() - 1 << "): ";
			std::cin >> keyboard_index;

			if (std::cin) {
				break;
			}

			std::cout << "That's not a valid option, try again.\n\n";
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		}
		std::cout << "\n";

		// Select and save the keyboard
		keyboard_path = keyboards[keyboard_index].path;
		config["keyboard"] = keyboard_path;

		config_changed = true;
	}

	// Save the config changes
	if (config_changed) {
		try {
			save_config(config_filename, keyboard_path, morse_key, long_threshold, original_key_threshold, end_threshold);
		} catch (const std::runtime_error& e) {
			std::cerr << e.what() << "\n";
			return EXIT_FAILURE;
		}
	}

    // Open the physical keyboard device
    int fd = open(keyboard_path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        std::cerr << "Failed to open input device (" << std::strerror(errno) << ")\n";
		return EXIT_FAILURE;
    }

    struct libevdev *dev = nullptr;
    int rc = libevdev_new_from_fd(fd, &dev);
    if (rc < 0) {
        std::cerr << "Failed to init libevdev (" << std::strerror(-rc) << ")\n";
		return EXIT_FAILURE;
    }

	if (!libevdev_has_event_code(dev, EV_KEY, morse_key_code)) {
		std::cerr << libevdev_get_name(dev) << " [" << keyboard_path << "] does not have " << morse_key << "\n";
		return EXIT_FAILURE;
	}

	std::cout << "Using " << libevdev_get_name(dev) << " [" << keyboard_path << "]\n\n";

	// Wait 3 seconds to prevent keys getting stuck
	std::cout << "Release all keys on the keyboard...\n\n";
	usleep(3000000);

    // Grab exclusive access
    rc = libevdev_grab(dev, LIBEVDEV_GRAB);
    if (rc < 0) {
        std::cerr << "Failed to grab device (" << std::strerror(-rc) << ")\n";
		return EXIT_FAILURE;
    }

    // Create a virtual uinput device that mirrors the original keyboard and sends the Morse output
    struct libevdev_uinput *uidev = nullptr;
    rc = libevdev_uinput_create_from_device(
        dev,
        LIBEVDEV_UINPUT_OPEN_MANAGED,
        &uidev
    );
    if (rc < 0) {
        std::cerr << "Failed to create uinput device (" << std::strerror(-rc) << ")\n";
		return EXIT_FAILURE;
    }

	std::cout << morse_key.substr(4) << " is now the Morse input key!\nYou can stop Morse Keyboard by pressing CTRL + C in this terminal\n\n";

	std::chrono::steady_clock::time_point morse_key_start;
	std::chrono::steady_clock::time_point morse_key_end;
	bool morse_key_pressed = false;
	bool morse_key_released = false;
	std::string received_morse;

	bool leftshift_pressed = false;
	bool rightshift_pressed = false;

    while (true) {
        struct input_event ev;
        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            if (ev.type == EV_KEY) {
				if (ev.code == morse_key_code) {
					if (ev.value == 1) {
						morse_key_start = std::chrono::steady_clock::now();
						morse_key_pressed = true;
						morse_key_released = false;
					} else if (ev.value == 0 && morse_key_pressed) {
						morse_key_end = std::chrono::steady_clock::now();
						morse_key_pressed = false;
						morse_key_released = true;

						std::chrono::duration<double, std::milli> morse_key_duration = morse_key_end - morse_key_start;
						if (morse_key_duration.count() > long_threshold) {
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

            // Forward every event that isn't the Morse key
            libevdev_uinput_write_event(
                uidev,
                ev.type,
                ev.code,
                ev.value
            );

        } else if (rc == -EAGAIN) {
            usleep(1000);
        }

		// Allow original key on long press
		if (morse_key_pressed) {
			std::chrono::duration<double, std::milli> morse_key_duration = std::chrono::steady_clock::now() - morse_key_start;
			if (morse_key_duration.count() > original_key_threshold) {
				morse_key_pressed = false;
				received_morse = "";

				libevdev_uinput_write_event(
					uidev,
					EV_KEY,
					morse_key_code,
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
					morse_key_code,
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
		if (morse_key_released) {
			std::chrono::duration<double, std::milli> morse_key_release_duration = std::chrono::steady_clock::now() - morse_key_end;
			if (morse_key_release_duration.count() > end_threshold) {
				morse_key_released = false;

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

    return EXIT_SUCCESS;
}
