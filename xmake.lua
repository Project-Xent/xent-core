set_project("xent-core")
-- Version from the git tag: release CI injects $XENT_VERSION (tag minus "v").
set_version(os.getenv("XENT_VERSION") or "0.3.0-dev")
set_languages("c23")
add_rules("mode.debug", "mode.release")
set_warnings("all")

local unit_tests = {
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
    "test_mixed_protocols",
    "test_rtl_baseline_conformance",
    "test_layout_grid",
    "test_layout_grid_spec",
    "test_yoga_generated",
    "test_yoga_supported",
    "test_cli",
    "test_profile",
    "test_display",
    "test_external_measure",
    "test_ordered_topology",
    "test_simd_equivalence",
    "test_fault_injection",
}

option("simd")
    set_default(false)
    set_description("Enable SIMD kernels at runtime by default")
option_end()

option("ispc")
    set_default(false)
    set_showmenu(true)
    set_description("Build the ISPC SIMD backend")
option_end()

option("fault_injection")
    set_default(false)
    set_showmenu(true)
    set_description("Enable internal allocation fault-injection tests")
option_end()

target("xent_core")
    set_kind("static")
    add_includedirs("include", { public = true })
    add_headerfiles("include/xent/*.h")
    add_files("src/core/*.c", "src/layout/*.c", "src/text/*.c", "src/cli/*.c")
    if has_config("fault_injection") then
        add_defines("XENT_ENABLE_FAULT_INJECTION=1", { public = true })
    end
    if is_plat("mingw") then
        add_cflags("-ffunction-sections", "-fdata-sections", { force = true })
    end
    if is_plat("windows") then
        add_cflags("/Gy", "/Gw", { tools = "cl", force = true })
    end
    if has_config("simd") then
        add_defines("XENT_ENABLE_SIMD=1", { public = true })
    else
        add_defines("XENT_ENABLE_SIMD=0", { public = true })
    end
    if has_config("ispc") then
        add_defines("XENT_ISPC_ENABLED=1", { public = true })
        on_load(function (target)
            local dir = path.join(target:autogendir(), "ispc")
            target:add("includedirs", dir)
        end)
        before_build(function (target)
            import("lib.detect.find_program")
            local ispc = find_program("ispc")
            assert(ispc, "ispc compiler not found on PATH")

            local dir = path.join(target:autogendir(), "ispc")
            local ext = is_plat("windows") and ".obj" or ".o"
            local source = path.join(os.scriptdir(), "src", "simd", "xent_ispc_kernels.ispc")
            local object = path.join(dir, "xent_ispc_kernels" .. ext)
            local header = path.join(dir, "xent_ispc_kernels_ispc.h")
            local script = path.join(os.scriptdir(), "xmake.lua")
            local newest_input = math.max(os.mtime(source), os.mtime(script))
            if os.mtime(object) < newest_input then
                os.mkdir(dir)
                local argv = {
                    source,
                    "-o", object,
                    "-h", header,
                    "--target=sse4,avx2",
                    "--arch=x86-64",
                    "-O2",
                }
                if is_mode("debug") then
                    table.insert(argv, "-g")
                end
                os.vrunv(ispc, argv)
            end

            local objects = target:objectfiles()
            table.insert(objects, object)
            table.insert(objects, path.join(dir, "xent_ispc_kernels_sse4" .. ext))
            table.insert(objects, path.join(dir, "xent_ispc_kernels_avx2" .. ext))
        end)
    else
        add_defines("XENT_ISPC_ENABLED=0", { public = true })
    end

for _, demo in ipairs({ "demo_basic", "demo_flex_vs_swiftstack", "demo_dump_json" }) do
    target(demo)
        set_default(false)
        set_kind("binary")
        add_files("examples/" .. demo .. ".c")
        add_deps("xent_core")
        add_includedirs("include")
end

for _, bench_name in ipairs({ "bench_layout", "bench_simd", "bench_compare_recursive_baseline", "bench_dirty_vs_full", "bench_regression_gates" }) do
    target(bench_name)
        set_default(false)
        set_kind("binary")
        add_files("bench/" .. bench_name .. ".c")
        add_deps("xent_core")
        add_includedirs("include")
end

target("bench_yoga_native")
    set_default(false)
    set_kind("binary")
    add_files("bench/yoga/bench_yoga_native.c")
    add_deps("xent_core")
    add_includedirs("include")

for _, test_name in ipairs(unit_tests) do
    target(test_name)
        set_default(false)
        set_kind("binary")
        if test_name == "test_yoga_generated" or test_name == "test_yoga_supported" then
            add_files("tests/yoga/" .. test_name .. ".c")
        else
            add_files("tests/" .. test_name .. ".c")
        end
        add_deps("xent_core")
        add_includedirs("include", "tests")
end

task("test")
    set_menu {
        usage = "xmake test",
        description = "Build and run all unit tests.",
    }
    on_run( function ()
        for _, t in ipairs(unit_tests) do
            if t ~= "test_fault_injection" or has_config("fault_injection") then
                os.exec("xmake build " .. t)
                os.exec("xmake run " .. t)
            end
        end
    end)

task("gen-yoga")
    set_menu {
        usage = "xmake gen-yoga --source=<yoga-generated-dir> [--output=<out.c>]",
        description = "Regenerate the supported Yoga conformance fixture.",
        options = {
            {'s', "source", "kv", nil, "Yoga tests/generated directory."},
            {'o', "output", "kv", nil, "Generated C file; defaults to tests/yoga/test_yoga_supported.c."},
        },
    }
    on_run(function ()
        import("core.base.option")
        local source = option.get("source")
        assert(source, "--source is required")
        local args = {"lua", path.join(os.scriptdir(), "tools", "gen_yoga.lua"), source}
        local output = option.get("output")
        if output then table.insert(args, output) end
        os.execv("xmake", args)
    end)
