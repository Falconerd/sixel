# Sixel

A simple pixel art tool.

![](https://raw.githubusercontent.com/Falconerd/sixel/master/screenshot.png)
![](https://raw.githubusercontent.com/Falconerd/sixel/master/example.png)

## CLI Options

| Option | Description | Example | Defaults |
|--------|-------------|---------|----------|
| `-d width height` | Width and Height of the Image | `-d 24 16` | `16 16` |
| `-w width height` | Width and Height of the Window | `-w 800 600` | `512 512` |
| `-p colours...` | Adds up to 127 hex colours to the palette. Accepts alpha value as the last two characters. | `-p ff0000ff 00ff00ff 0000ffff` | [https://www.lexaloffle.com/pico-8.php](Pico8) |
| `-o path` | Save to this file path | `-o example.png` | `test.png` |

## Controls

| Key | Description |
|-----|-------------|
| B | Draw mode |
| E | Erase mode |
| F | Fill mode |
| S | Save |
| H | Previous colour |
| J | Colour + 8 |
| K | Colour - 8 |
| L | Next colour |
| 1-8 | Colours 1-8 |
| Shift + 1-8 | Colours 9-16 |
| Scroll Wheel | Zoom |
| Right Click (Hold) | Pan |
| Esc | Quit |

## Requirements

### GLFW3
See the GLFW website for build linking info https://www.glfw.org/docs/latest/build_guide.html#build_link
