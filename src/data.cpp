#include "header.hpp"
#define BONGO_ERROR 1

#include <unistd.h>
#include <limits.h>

extern "C" {
#include <SDL2/SDL.h>
}

namespace data {
Json::Value cfg;
std::map<std::string, sf::Texture> img_holder;

void create_config() {
    const char *s =
        R"V0G0N({
    "resolution": {
        "letterboxing": false,
        "width": 1920,
        "height": 1080,
        "horizontalPosition": 0,
        "verticalPosition": 0
    },
    "decoration": {
        "leftHanded": false,
        "transparent": true,
        "opacity": 255,
        "rgb": [255, 255, 255],
        "offsetX": [0, 11],
        "offsetY": [0, -65],
        "scalar": [1.0, 1.0]
    },
    "binds": {
        "mouse": true,
        "paw": [255, 255, 255],
        "pawEdge": [0, 0, 0],
        "left": [81, 87, 69, 82, 84, 65, 83, 68, 70, 71, 90, 88, 67, 86, 66, 49, 50, 51, 52, 53, 38, 40, 17, 16, 18, 93, 192, 9],
        "right": [89, 85, 73, 79, 80, 72, 74, 75, 76, 77, 78, 54, 55, 56, 57, 48, 37, 39, 219, 221, 186, 188, 190, 222, 191, 220, 187, 189, 13, 8, 34, 33, 35, 36, 45, 46],
        "wave": [32]
    },
    "mousePaw": {
        "mousePawComment": "coordinates start in the top left of the window",
        "pawStartingPoint": [211, 159],
        "pawEndingPoint": [258, 228]
    }
})V0G0N";
    std::string error;
    Json::CharReaderBuilder cfg_builder;
    Json::CharReader *cfg_reader = cfg_builder.newCharReader();
    cfg_reader->parse(s, s + strlen(s), &cfg, &error);
    delete cfg_reader;
}

void error_msg(std::string error, std::string title) {
    SDL_MessageBoxButtonData buttons[] = {
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Retry" },
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Cancel" },
    };

    SDL_MessageBoxColorScheme colorScheme = {
        { /* .colors (.r, .g, .b) */
     /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
        { 255, 255,255 },
        /* [SDL_MESSAGEBOX_COLOR_TEXT] */
        { 0, 0, 0 },
        /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
        { 0, 0, 0 },
        /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
        { 255,255, 255 },
        /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
        { 128, 128, 128 }
        }
    };

    SDL_MessageBoxData messagebox_data = {
    	SDL_MESSAGEBOX_ERROR,
    	NULL,
    	title.c_str(),
    	error.c_str(),
    	SDL_arraysize(buttons),
    	buttons,
    	&colorScheme
    };

    int button_id;

    SDL_ShowMessageBox(&messagebox_data, &button_id);

    if (button_id == -1 || button_id == 1) {
        exit(BONGO_ERROR);
    }
}

bool update(Json::Value &cfg_default, Json::Value &cfg) {
    bool is_update = true;
    for (const auto &key : cfg.getMemberNames()) {
        if (cfg_default.isMember(key)) {
            if (cfg_default[key].type() != cfg[key].type()) {
                error_msg("Value type error in config.json", "Error reading configs");
                return false;
            }
            if (cfg_default[key].isArray() && !cfg_default[key].empty()) {
                for (Json::Value &v : cfg[key]) {
                    if (v.type() != cfg_default[key][0].type()) {
                        error_msg("Value type in array error in config.json", "Error reading configs");
                        return false;
                    }
                }
            }
            if (cfg_default[key].isObject()) {
                is_update &= update(cfg_default[key], cfg[key]);
            } else {
                cfg_default[key] = cfg[key];
            }
        } else {
            cfg_default[key] = cfg[key];
        }
    }
    return is_update;
}

bool init() {
    while (true) {
        create_config();
        std::ifstream cfg_file("config.json", std::ifstream::binary);
        if (!cfg_file.good()) {
            break;
        }
        std::string cfg_string((std::istreambuf_iterator<char>(cfg_file)), std::istreambuf_iterator<char>()), error;
        Json::CharReaderBuilder cfg_builder;
        Json::CharReader *cfg_reader = cfg_builder.newCharReader();
        Json::Value cfg_read;
        if (!cfg_reader->parse(cfg_string.c_str(), cfg_string.c_str() + cfg_string.size(), &cfg_read, &error)) {
            delete cfg_reader;
            error_msg("Syntax error in config.json:\n" + error, "Error reading configs");
        } else if (update(cfg, cfg_read)) {
            delete cfg_reader;
            break;
        }
    }

    img_holder.clear();

    return custom::init();

}

sf::Texture &load_texture(std::string path) {
    if (img_holder.find(path) == img_holder.end()) {
        while (!img_holder[path].loadFromFile(path)) {
            error_msg("Cannot find file " + path, "Error importing images");
        }
    }
    return img_holder[path];
}
}; // namespace data
