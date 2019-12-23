set_project("ctxco")
set_version("0.0.1")
set_xmakever("2.2.8")
set_warnings("all", "error")
set_languages("gnu15")

add_rules("mode.debug", "mode.release")

if is_mode("debug") then
    set_symbols("debug")
    set_optimize("none")
    if is_mode("check") and is_arch("i386", "x86_64") then
        add_cxflags("-fsanitize=address", "-ftrapv")
        add_mxflags("-fsanitize=address", "-ftrapv")
        add_ldflags("-fsanitize=address")
    end
end

if is_mode("release") then
    set_symbols("hidden")
    set_strip("all")
    add_cxflags("-fomit-frame-pointer")
    add_mxflags("-fomit-frame-pointer")
    set_optimize("fastest")
    add_vectorexts("sse2", "sse3", "ssse3", "mmx")
end

option("shared")
    set_default(false)
    set_showmenu(true)
    set_category("option")
    set_description("Enable or disable shared mode")
option_end()

option("demo")
    set_default(false)
    set_showmenu(true)
    set_category("option")
    set_description("Enable or disable demo")
option_end()

target("ctxco")
    if has_config("shared") then
        add_cxflags("-fPIC")
        add_ldflags("-fPIC")
        set_kind("shared")
    else
        set_kind("static")
    end
    set_default(true)
    add_files("src/ctxco/*.c", "src/ctxco/ctx.S")
    add_headerfiles("src/ctxco/ctx.h", "src/ctxco/ctxco.h")
target_end()

target("demo_simple")
    set_default(has_config("demo"))
    set_kind("binary")
    add_deps("ctxco")
    add_packages("ctxco")
    add_files("src/demo/simple.c")
target_end()