set_project("ctxco")
set_version("0.0.1")
set_xmakever("2.2.8")
set_warnings("all", "error")
set_languages("gnu15")

add_rules("mode.debug", "mode.release", "mode.check")

option("demo")
    set_default(false)
    set_showmenu(true)
    set_category("option")
    set_description("Enable or disable demo")
option_end()

target("ctxco")
    if is_kind("shared") then
        add_cxflags("-fPIC")
        add_ldflags("-fPIC")
    end
    set_kind("$(kind)")
    set_default(true)
    add_files("src/ctxco/*.c", "src/ctxco/ctx.S")
    add_headerfiles("src/(ctxco/*.h)")
target_end()

target("demo_simple")
    set_default(has_config("demo"))
    set_kind("binary")
    add_deps("ctxco")
    add_files("src/demo/simple.c")
target_end()

target("demo_epoll")
    set_default(has_config("demo"))
    set_kind("binary")
    add_deps("ctxco")
    add_files("src/demo/epoll.c")
target_end()