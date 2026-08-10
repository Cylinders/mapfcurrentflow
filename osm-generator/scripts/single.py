import osmnx.utils

from osm_generator.cli import main

osmnx.settings.log_console = True
osmnx.settings.log_level = 10

if __name__ == "__main__":
    main()
