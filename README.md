# Rpg maker tools I made (2024 & beyond)

This is a repo I'll use to publish some RPG Maker tools that I've created.

## RmkDelocalise

### Description
Meant for removing the dependency on system locale for running games by renaming every asset and references to itself to its filename hash.

Inspired by the impressively awful Steam Yume Nikki port, which accomplished a similar task but by much, **much** worse means.

Unlike whatever process Steam YN went through, this tool does not break any references, does not replace files with similarly named ones, does not change sound pitches, and perfectly reserialises the game if asset name hashing is not enabled.

This tool also has a second purpose for encountering references to deleted resources, as it has to check for the existence of every asset reference.

And a *third* purpose, which is translating file names and updating references to them in the game.

This tool uses [md5-c](https://github.com/Zunawe/md5-c/).

### Note

This tool might not work on case-sensitive systems (linux) due to some games (2kki) refering to directories (../music, when the actual folder name is Music) with different capitalization.

### Usage

```
USAGE: RmkDelocalise [switches] /game/directory
-d              Enables dry run (does not write any files).
                Using only this switch is useful to find references to
                deleted game resources.

-h              If an asset is found to not be present in the asset map,
                insert a hash of the asset name into the asset map.
                Otherwise, do not change the asset name.

-u              DO NOT warn about assets that do not exist.

-m              Provide a file that describes the asset map.
                EXAMPLE: -m ./assetmap.txt

-M              Print the asset map after execution.

-r              Load provided asset map in reverse, if any.

-D              DO NOT alert about identically named files.

-c              Specify the codepage to use when reading game text.
                Default is 932 (shift-jis).
                EXAMPLE: -c 932
```

The format of the file to be provided to `-m` is as follows:

```
>CharSet // " > " being the first character sets current asset folder.
#adsasf dsfgjndfgn // " # " being the first character skips the line entirely
origfile.png|transfile.png //the name of the file in the original game directory, then the name of the file in the output directory.
```

#### Examples
`RmkDelocalise /game/dir` will recreate the game directory, however unused/unreferenced files will not be present.

`RmkDelocalise -d /game/dir` will, as described by the switch description, perform a dry run. This is useful for finding references to removed files.

`RmkDelocalise -h /game/dir` will recreate the game directory, but all files will have been renamed to a hash of themselves and the folder they reside in. This removes any locale dependencies to run the game without crashes, however obviously the game text will remain unchanged.

`RmkDelocalise -duDM /game/dir` will output the generated asset map without any warnings of unused files or identical filenames. The resulting output is in a format that can be fed back to `-m`. This is useful for translating file names.

If you wish to ship your game without locale dependence, but still wish for users to be able to view the original directory structure:

`RmkDelocalise -uDhM /game/dir > assetmap.txt`

Then ship your game with `assetmap.txt`, and users can get the original directory structure by doing:

`RmkDelocalise -uDr -m assetmap.txt /game/dir`

## TxtXyz

### Description

Prints a text representation of an XYZ file. This was made because Picture\真っ黒.xyz in Yume Nikki for some reason differed in the original version and the Steam version but the image appeared identical.
(the difference turned out being the original image was filled with palette index 1 but in the Steam version it's index 0???????)

### Usage

```
TxtXyz [many file paths, or just one]
```

## RouteDecode

### Description

Decodes a move route command's parameters and prints it in a readable format.

NOTE TO SELF the fuck's up with the target value

### Usage

```
RouteDecode [paste the move route numbers into here]
```

#### Example

`RouteDecode 22 8 0 0 34 12 131 67 131 120 131 147 131 103 95 48 48 57 5 23 23 34 12 131 67 131 120 131 147 131 103 95 48 48 57 0` will output:
```
Repeats: No, Skippable: No
change_graphic イベント_009 [5]
wait
wait
change_graphic イベント_009 [0]
```