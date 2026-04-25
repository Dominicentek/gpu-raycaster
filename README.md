# gpu-raycaster

silly raycasting experiment

todo:
- user friendly scene editor
- transform the player position when going through portals
- ability for portals to apply a roll rotation
- dynamic scene updates
- make it a usable library for games

## features

- textured walls
- textured floors/ceilings
- variable floor/ceiling heights
- fog
- portals
  - can translate/rotate rays
  - a line can have multiple different portals

## scene format

the file format is mostly just sets of numbers. each number can be omitted as they have a default value.

textures are indicated by a single number. they are the row major index into the "textures.png" image, which is a 16x16 tileset.
you can resize it to any size you want, as long as its a multiple of 16.

the file starts with a header. the header consists of a set of 9 numbers:
- `uint` (0): number of lines in the scene
- `uint` (0): number of sectors in the scene
- `uint` (0): number of portals in the scene
- `uint` (0): players starting sector number
- `float` (0): distance at which the fog starts
- `float` (8): distance from the fog start at which the fog ends
- `float` (0): fog color R channel (0 to 1)
- `float` (0): fog color G channel (0 to 1)
- `float` (0): fog color B channel (0 to 1)

the following lines start with letters.
- `s` indicates a sector
- `l` indicates a line
- `p` indicates a portal
lines that dont start with these letters are ignored

the letter `s` (sector) is followed by these numbers:
- `uint` (0): starting index in line data
- `uint` (0): ending index in line data
- `float` (0): floor height
- `float` (1): ceiling height
- `uint` (0): floor texture
- `uint` (0): ceiling texture

the letter `l` (line) is followed by these numbers:
- `float` (0): the lines starting X position
- `float` (0): the lines starting Y position
- `float` (0): the lines ending X position
- `float` (0): the lines ending Y position
- `uint` (0): line texture
- `float` (1): X scale for the texture (relative to line dimensions)
- `float` (1): Y scale for the texture (relative to sector height)
- `float` (0): X offset for the texture (0 to 1)
- `float` (0): Y offset for the texture (0 to 1)
- `uint` (0): starting index into portal data
- `uint` (0): ending index into portal data

the letter `p` (portal) is followed by these numbers:
- `uint` (0): destination sector
- `float` (0): translation on the X axis
- `float` (0): translation on the Y axis
- `float` (0): rotation (applied after translation)
- `float` (0.5): rotation origin (relative to line dimensions, 0 -> line start, 1 -> line end)
