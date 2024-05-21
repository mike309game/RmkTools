# Rpg maker tools I made (2024 & beyond)

This is a repo I'll use to publish some RPG Maker tools that I've created.

Currently there is only one tool.

## RmkDelocalise

### Description
Meant for removing the dependency on system locale for running games by renaming every asset and references to itself to its filename hash.

Inspired by the impressively awful Steam Yume Nikki port, which accomplished a similar task but much, **much** worse.

Unlike whatever process Steam YN went through, this tool does not break any references, does not replace files with similarly named ones, does not change sound pitches, and perfectly reserialises the game if asset name hashing is not enabled.

This tool also has a second purpose for encountering references to deleted resources, as it has to check for the existence of every asset reference.

And a *third* purpose, which is translating file names and updating references to them in the game.


This tool uses [md5-c](https://github.com/Zunawe/md5-c/) and [Getopt-for-Visual-Studio](https://github.com/skandhurkat/Getopt-for-Visual-Studio) (no, thanks, I will NOT opt to use vcpkg. You are insane).

### Usage

```
USAGE: RmkDelocalise.exe [switches] /game/directory
-d              Enables dry run (does not write any files).
                Using only this switch is useful to find references to
                deleted game resources.

-h              If an asset is found to not be present in the asset map,
                insert a hash of the asset name into the asset map.
                Otherwise, do not change the asset name.

-u              DO NOT warn about assets that do not exist.

-m              Provide a file that describes the asset map.
                EXAMPLE: -p ./assetmap.txt

-M              Print the asset map after execution.

-R              Print the asset map but invert the positions of the
                original file and the translated file.

-c              Specify the codepage to use when reading game text.
                Default is 932 (shift-jis).
                EXAMPLE: -c 932

-C              Specify the codepage to use when reading arguments
                given to this program. Default is the same as the
                game text codepage (shift-jis). MUST BE BEFORE ANY
                OTHER ARGUMENTS.
                EXAMPLE: -C 932
```

The purpose of `-C` is due to Windows arguments not being unicode.

The format of the file to be provided to `-m` is as follows:

```
>CharSet // " > " being the first character sets current asset folder.
origfile.png|transfile.png //the name of the file in the original game directory, then the name of the file in the output directory.
```

#### Examples
`RmkDelocalise.exe /game/dir` will recreate the game directory, however unused/unreferenced files will not be present.

`RmkDelocalise.exe -d /game/dir` will, as described by the switch description, perform a dry run. This is useful for finding references to removed files.

`RmkDelocalise.exe -h /game/dir` will recreate the game directory, but all files will have been renamed to a hash of themselves and the folder they reside in. This removes any locale dependencies to run the game without crashes, however obviously the game text will remain unchanged.

`RmkDelocalise.exe -uM /game/dir` will output the generated asset map without any warnings to unused files. The resulting output is in a format that can be fed back to `-m`. This is useful for translating file names.

If you wish to ship your game without locale dependence, but still wish for users to be able to view the original directory structure:

`RmkDelocalise.exe -uhR /game/dir > assetmap.txt`

Then ship your game with `assetmap.txt`, and users can get the original directory structure by doing:

`RmkDelocalise.exe -u -m assetmap.txt /game/dir`

# Lots of ranting and swearing

It is an absolute fucking pain to utilise liblcf.

In windows, it apparently can't be built as a shared library because ??????? AND you are forced to use vcpkg in windows, because msys packages for icu are seemingly broken and because the cmakefile specifically mentions only vcpkg is supported.

Vcpkg is an absolute motherfucking joke and I can't believe anyone thinks it is a good idea. I have wasted [SEVERAL HOURS](https://cdn.discordapp.com/attachments/864670403795353641/1238315866597101639/image.png?ex=664e0045&is=664caec5&hm=5e8a655622d18b45d2b55d05cc7ebcacbdd1203e22d1e4b9fac0a9fff72fe736&) (if discord says the link isn't available just paste it back into discord) JUST to install qt5. I have no idea why they think it is a good idea to instead of distributing prebuilt packages the user should waste time of their day building them. It also has really weird integration with visual studio that I have no idea how it works. 

And after having dealt with vcpkg i STILL have to deal with cmake, which is also shit software and I believe anything other than makefiles are a stupid waste of time especially for such a basic library like liblcf. I still have to figure out how to properly statically link this tool, but no matter what I do liblcf is being built with /MD, and every time I compile things with bad compiler arguments i have to delete the cmake cache, wait for it to regenerate, wait for liblcf to build, and hope to God that it works.
And THEN, I'm locked into using the shit, sluggish, barely usable, [possibly spyware](https://cdn.discordapp.com/attachments/1129106591921209475/1242557487530639430/image.png?ex=664e4556&is=664cf3d6&hm=c339ead3382844cc870d47872da73267a05233bcf6d3f665d73cca38c65ccbbc&) piece of software that is known as Visual Studio 2022. I can't overstate how unreasonably sluggish visual studio is, comparing it to vs2013 which is almost perfect with little to no sluggishness, but I can't use 2013 because it has no cmake integration, and I can't just yoink liblcf .lib files built by cmake because then it complains about shit that i already forgot but it had to do with there already being a msvcrt linked with liblcf.

Perhaps I could use vs2019 but I feel it'll barely be an improvement and I have to install 6gb of i-have-no-fucking-idea for cpp support in vs2019.
