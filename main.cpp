#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>

const std::string config_filename = "config.yaml";

int main() {
	// Create the config yaml if it doesn't exist
	if (!std::filesystem::exists(config_filename)) {
		YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "config_version" << YAML::Value << 1;
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

	return 0;
}
