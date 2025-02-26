2025-02-26 it's a makefile thingy now :) eat my asshole bill gates

# Lots of ranting and swearing

It is an absolute fucking pain to utilise liblcf.

In windows, it apparently can't be built as a shared library because ??????? AND you are forced to use vcpkg in windows, because msys packages for icu are seemingly broken and because the cmakefile specifically mentions only vcpkg is supported.

Vcpkg is an absolute motherfucking joke and I can't believe anyone thinks it is a good idea. I have wasted [SEVERAL HOURS](https://cdn.discordapp.com/attachments/864670403795353641/1238315866597101639/image.png?ex=664e0045&is=664caec5&hm=5e8a655622d18b45d2b55d05cc7ebcacbdd1203e22d1e4b9fac0a9fff72fe736&) (if discord says the link isn't available just paste it back into discord) JUST to install qt5. I have no idea why they think it is a good idea to instead of distributing prebuilt packages the user should waste time of their day building them. It also has really weird integration with visual studio that I have no idea how it works. 

And after having dealt with vcpkg i STILL have to deal with cmake, which is also shit software and I believe anything other than makefiles are a stupid waste of time especially for such a basic library like liblcf. I still have to figure out how to properly statically link this tool, but no matter what I do liblcf is being built with /MD, and every time I compile things with bad compiler arguments i have to delete the cmake cache, wait for it to regenerate, wait for liblcf to build, and hope to God that it works.
And THEN, I'm locked into using the shit, sluggish, barely usable, [possibly spyware](https://cdn.discordapp.com/attachments/1129106591921209475/1242557487530639430/image.png?ex=664e4556&is=664cf3d6&hm=c339ead3382844cc870d47872da73267a05233bcf6d3f665d73cca38c65ccbbc&) piece of software that is known as Visual Studio 2022. I can't overstate how unreasonably sluggish visual studio is, comparing it to vs2013 which is almost perfect with little to no sluggishness, but I can't use 2013 because it has no cmake integration, and I can't just yoink liblcf .lib files built by cmake because then it complains about shit that i already forgot but it had to do with there already being a msvcrt linked with liblcf.

Perhaps I could use vs2019 but I feel it'll barely be an improvement and I have to install 6gb of i-have-no-fucking-idea for cpp support in vs2019.
