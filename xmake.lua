set_project("xent-core")
set_version("0.1.0")
set_languages("c23", "c++20")
add_rules("mode.debug", "mode.release")
set_warnings("all")

option("simd")
    set_default(false)
    set_description("Enable experimental SIMD path scaffolding")
option_end()

option("highway")
    set_default(false)
    set_showmenu(true)
    set_description("Enable Highway SIMD backend when available")
option_end()

add_requires("highway", {optional = true})

target("xent_core")
    set_kind("static")
    add_includedirs("include", {public = true})
    add_headerfiles("include/xent/*.h")
    add_files("src/core/*.c", "src/layout/*.c", "src/text/*.c", "src/cli/*.c", "src/plugins/*.c")
    if has_config("simd") then
        add_defines("XENT_ENABLE_SIMD=1", {public = true})
    else
        add_defines("XENT_ENABLE_SIMD=0", {public = true})
    end
    if has_config("highway") and has_package("highway") then
        add_files("src/simd/*.cc")
        add_packages("highway")
        add_defines("XENT_HIGHWAY_ENABLED=1", {public = true})
    else
        add_defines("XENT_HIGHWAY_ENABLED=0", {public = true})
    end

for _, demo in ipairs({"demo_basic", "demo_flex_vs_swiftstack", "demo_dump_json"}) do
    target(demo)
        set_kind("binary")
        add_files("examples/" .. demo .. ".c")
        add_deps("xent_core")
        add_includedirs("include")
end

for _, bench_name in ipairs({"bench_layout", "bench_compare_recursive_baseline", "bench_dirty_vs_full", "bench_regression_gates"}) do
    target(bench_name)
        set_kind("binary")
        add_files("bench/" .. bench_name .. ".c")
        add_deps("xent_core")
        add_includedirs("include")
end

for _, test_name in ipairs({
    "test_tree",
    "test_layout_flex",
    "test_layout_flex_spec",
    "test_layout_swiftstack",
    "test_layout_swiftstack_spec",
    "test_direction",
    "test_layout_rounding",
    "test_layout_stress",
    "test_dirty",
    "test_text_cache",
    "test_text_backend_contract",
    "test_text_linebreak_policy",
    "test_text_shaping",
    "test_mixed_protocols",
    "test_rtl_baseline_conformance",
}) do
    target(test_name)
        set_kind("binary")
        add_files("tests/" .. test_name .. ".c")
        add_deps("xent_core")
        add_includedirs("include", "tests")
end

task("test")
    set_menu {
        usage = "xmake test",
        description = "Build and run all unit tests.",
    }
    on_run(function ()
        local tests = {
            "test_tree",
            "test_layout_flex",
            "test_layout_flex_spec",
            "test_layout_swiftstack",
            "test_layout_swiftstack_spec",
            "test_direction",
            "test_layout_rounding",
            "test_layout_stress",
            "test_dirty",
            "test_text_cache",
            "test_text_backend_contract",
            "test_text_linebreak_policy",
            "test_text_shaping",
            "test_mixed_protocols",
            "test_rtl_baseline_conformance",
        }
        for _, t in ipairs(tests) do
            os.exec("xmake build " .. t)
            os.exec("xmake run " .. t)
        end
    end)
