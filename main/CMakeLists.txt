cmake_minimum_required(VERSION 3.16)

# Collect all source files
file(GLOB GAME_SRCS    "game/*.cpp")
file(GLOB SHARED_SRCS  "shared/*.cpp")
file(GLOB LUA_MAIN     "lua/LuaMain/*.cpp")
file(GLOB LUA_BIND     "lua/LuaBindings/*.cpp")
file(GLOB BMF_SRCS     "bmf/*.cpp")
file(GLOB PAL_SRCS     "pal/*.cpp")

# Exclude ltests.cpp
list(FILTER LUA_MAIN EXCLUDE REGEX ".*ltests\\.cpp$")

idf_component_register(
    SRCS
        "main.cpp"
        "fastopen.c"
        "sdcard.c"
        ${GAME_SRCS}
        ${SHARED_SRCS}
        ${LUA_MAIN}
        ${LUA_BIND}
        ${BMF_SRCS}
        ${PAL_SRCS}
    PRIV_REQUIRES
        esp_lcd
        fatfs
        nvs_flash
        bt
        badge-bsp
        esp_timer
        esp_psram
        driver
        esp_driver_jpeg
        sdmmc
    INCLUDE_DIRS
        "."
        "shared"
        "game"
        "lua/LuaMain"
        "lua/LuaBindings"
        "bmf"
        "pal"
)

# Global build defines (apply to all components so dependencies see TANMATSU_BUILD)
idf_build_set_property(COMPILE_OPTIONS "-DTANMATSU_BUILD=1" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DLODEPNG_NO_COMPILE_ALLOCATORS" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DDISABLE_NETWORK=1" APPEND)
# Uncomment to disable background art and save memory:
idf_build_set_property(COMPILE_OPTIONS "-DDISABLE_BACKGROUND_ART=1" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=unused-variable" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=unused-const-variable" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=unused-function" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=deprecated-declarations" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=format" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-format" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=misleading-indentation" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=restrict" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=address" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=maybe-uninitialized" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=unused-but-set-parameter" APPEND)

# C++-specific flags: apply only to this component's CXX files
target_compile_options(${COMPONENT_LIB} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)
