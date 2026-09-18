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

const std::string config_filename = "config.yaml";

const std::unordered_map<std::string, int> morse_to_key_code = {
    {".-", KEY_A},   {"-...", KEY_B}, {"-.-.", KEY_C}, {"-..", KEY_D},  {".", KEY_E},
    {"..-.", KEY_F}, {"--.", KEY_G},  {"....", KEY_H}, {"..", KEY_I},   {".---", KEY_J},
    {"-.-", KEY_K},  {".-..", KEY_L}, {"--", KEY_M},   {"-.", KEY_N},   {"---", KEY_O},
    {".--.", KEY_P}, {"--.-", KEY_Q}, {".-.", KEY_R},  {"...", KEY_S},  {"-", KEY_T},
    {"..-", KEY_U},  {"...-", KEY_V}, {".--", KEY_W},  {"-..-", KEY_X}, {"-.--", KEY_Y},
    {"--..", KEY_Z}
};

int main() {
	// Create the config yaml if it doesn't exist
	if (!std::filesystem::exists(config_filename)) {
		YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "keyboard" << YAML::Value << "";
        out << YAML::Key << "long_threshold" << YAML::Value << 150;
        out << YAML::Key << "space_threshold" << YAML::Value << 400;
        out << YAML::Key << "end_threshold" << YAML::Value << 300;
        out << YAML::EndMap;

        std::ofstream fout(config_filename);
        if (!fout) {
            throw std::runtime_error("Failed to open file for writing: " + config_filename);
        }
        fout << out.c_str();
        fout.close();
	}

	// Load the config yaml
	YAML::Node config = YAML::LoadFile(config_filename);

	std::string keyboard_path = config["keyboard"].as<std::string>();
	int long_threshold = config["long_threshold"].as<int>();
	int space_threshold = config["space_threshold"].as<int>();
	int end_threshold = config["end_threshold"].as<int>();

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

	// Wait 1 second to prevent the return key getting stuck
	usleep(1000000);

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

	std::chrono::steady_clock::time_point space_start;
	std::chrono::steady_clock::time_point space_end;
	bool space_pressed = false;
	bool space_released = false;
	std::string received_morse;

    while (true) {
        struct input_event ev;
        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            if (ev.type == EV_KEY && ev.code == KEY_SPACE) {
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

				auto it = morse_to_key_code.find(received_morse);

				if (it != morse_to_key_code.end()) {
					int key_code = it->second;

					libevdev_uinput_write_event(
						uidev,
						EV_KEY,
						key_code,
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
						key_code,
						0
					);
					libevdev_uinput_write_event(
						uidev,
						EV_SYN,
						SYN_REPORT,
						0
					);

					std::cout << " -> " << libevdev_event_code_get_name(EV_KEY, key_code) << "\n";
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
