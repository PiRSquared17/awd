# The AWD project #
This Google Code project contains the open specification and open-source tool-chain for the second generation AWD file format. The format is currently considered stable, however the tools are still being developed.

The tool-chain so far comprises the following components, all of which are currently in development:
  * **libawd**: A C++ library to (greatly) simplify encoding of AWD files
  * **PyAWD**: A Python toolkit, offering a pythonic way to work with AWD files. Can be compiled as a wrapper for libawd, or standalone (with poorer writing performance) and works for both Python 2.6 and 3.x.
  * **Maya exporter**: AWD exporter for Autodesk Maya
  * **Blender exporter**: AWD exporter for Blender
  * **3ds Max exporter**: AWD exporter for Autodesk 3ds Max
  * Exporters for more 3D packages are planned.


# FAQ #
## What is AWD? ##
AWD is a binary file format for 3D scenes, objects and related data. It's main use is with the [Away3D engine](http://away3d.com), but any encoder/decoder that conforms with the open specification can work with AWD files. The file format specification and a set of tools for working with AWD files are maintained by the Away3D development team.

AWD is designed to be light-weight, fast and easy to parse, and to be extensible both by user applications and future versions of the file format, while remaining both forwards and backwards compatible.

## What is the status of AWD? Of the AWD SDK? ##
Version 2.0 of the file format is stable, however the SDK and exporter plug-ins are still in development. Code that is checked into the repository should at all times be expected to compile and work. Files that are generated by the AWD SDK should be expected to parse correctly in the latest version of the Away3D codebase. That said, there is no general purpose build system in place yet, which means that building the code and installing it properly may be cumbersome. Some exporter plug-ins are released in binary form through the downloads section of this site.

## How can I help? ##
If you want to help with the development of AWD, the biggest areas where help is currently needed is in testing and exporter plug-in development (e.g. for tools like Cinema4D and blender.) Also, feedback on the SDK code and on the file format specification is greatly appreciated, although the format can only evolve in a backwards-compatible way.

## What is Away3D? ##
Away3D is a real-time 3D engine designed for ActionScript 3 and the Adobe Flash Platform. It is free and open-source and can be downloaded from [www.away3d.com](http://www.away3d.com). See the website for more information.