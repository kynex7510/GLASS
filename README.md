# GLASS

(WIP) OpenGL ES implementation for the 3DS.

## Setup

Download a prebuilt version, use as a CMake dependency, or build manually.

### HOS build

```sh
cmake -B BuildHOS -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/3DS.cmake" -DCMAKE_BUILD_TYPE=Release -DGLASS_COMPILE_EXAMPLES=ON
cmake --build BuildHOS --config Release
cmake --install BuildHOS --prefix BuildHOS/Release
```

### BM build

```sh
cmake -B BuildBM -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="$CTR_BM_TOOLCHAIN_ROOT/Toolchain.cmake" -DCMAKE_BUILD_TYPE=Release -DGLASS_COMPILE_EXAMPLES=ON
cmake --build BuildBM --config Release
cmake --install BuildBM --prefix BuildBM/Release
```

## Usage

Read the [docs](DOCS.md). Additionally, the [examples](Examples) folder includes some examples.

## Credits

The following projects were used for reference:

- **[citro3d](https://github.com/devkitPro/citro3d)**
- **[picasso](https://github.com/devkitPro/picasso)**
- **[libctru](https://github.com/devkitPro/libctru)**
- **[3dbrew](https://www.3dbrew.org/wiki/Main_Page)**
- **[docs.gl](https://docs.gl)**

Additionally, many thanks to:
- **[oreo639](https://github.com/oreo639)**
- **[Swiftloke](https://github.com/Swiftloke)**
- **[PSI](https://github.com/PSI-Rockin)**
- **[DeltaV](https://github.com/LiquidFenrir)**
- **[neobrain](https://github.com/neobrain)**
- **[profi200](https://github.com/profi200)**

for providing informations about the GPU, graphics, and generally being helpful.