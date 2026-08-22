#include <mapf_common/scenario_reader.h>
#include <filesystem>
#include <iostream>

mapf::Agents mapf::reader::read_scenario(std::istream &in) {
    std::string line;

    if (!std::getline(in, line)) {
        throw std::runtime_error{"empty scenario"};
    }

    std::istringstream header{line};
    std::string word;
    std::string version;

    if (!(header >> word >> version) || word != "version") {
        throw std::runtime_error{"scenario must start with 'version x.x'"};
    }

    Agents agents;
    for (int line_no = 2; std::getline(in, line); ++line_no) {
        if (line.empty()) {
            continue;
        }

        std::istringstream ss{line};

        int bucket;
        std::string map_name;
        int map_width;
        int map_height;
        int start_x;
        int start_y;
        int goal_x;
        int goal_y;
        double optimal_length;

        if (!(ss >> bucket >> map_name >> map_width >> map_height
              >> start_x >> start_y >> goal_x >> goal_y >> optimal_length)) {

            throw std::runtime_error{
                "line " + std::to_string(line_no) + ": expected 9 fields"
            };
        }

        ss >> std::ws;



        if (!ss.eof()) {
            throw std::runtime_error{
                "line " + std::to_string(line_no) + ": expected 9 fields"
            };
        }

        agents.push_back({
            .start = {.row = start_y, .col = start_x},
            .goal = {.row = goal_y, .col = goal_x},
        });
    }

    return agents;
}
