#include <mapf_common/map_reader.h>

namespace {
    bool map_char_blocked(char c) {
        return c == '@' || c == 'O' || c == 'T';
    }

    bool map_char_valid(char c) {
        return map_char_blocked(c) || c == '.' || c == 'G' || c == 'S' || c == 'W';
    }
}

mapf::Grid mapf::reader::read_map(std::istream& in) {
    std::string key;
    std::string value;
    int height;
    int width;

    if (!(in >> key >> value) || key != "type" || value != "octile") {
        throw std::runtime_error{"expected header: type octile"};
    }

    if (!(in >> key >> height) || key != "height" || height <= 0) {
        throw std::runtime_error{"invalid height header"};
    }

    if (!(in >> key >> width) || key != "width" || width <= 0) {
        throw std::runtime_error{"invalid width header"};
    }

    if (!(in >> key) || key != "map") {
        throw std::runtime_error{"expected header: map"};
    }

    Grid grid{
        .width = width,
        .height = height,
        .blocked = std::vector<bool>(width * height),
    };

    std::string line;
    for (int row = 0; row < height; ++row) {
        if (!(in >> line)) {
            throw std::runtime_error{"expected more map rows"};
        }

        if (std::ssize(line) != width) {
            throw std::runtime_error{"map row has invalid width"};
        }

        for (int col = 0; col < width; ++col) {
            const char c = line[col];

            if (!map_char_valid(c)) {
                throw std::runtime_error{"invalid map character"};
            }

            grid.blocked[grid.index(row, col)] = map_char_blocked(c);
        }
    }

    if (in >> line) {
        throw std::runtime_error{"too many map rows"};
    }

    return grid;
}
