set_project("xent-core")
set_version("0.1.0")
set_languages("c23", "c++20")
add_rules("mode.debug", "mode.release")
set_warnings("all")

option("simd")
    set_default(false)
    set_description("Enable experimental SIMD path scaffolding")
option_end()

option("ispc")
    set_default(false)
    set_showmenu(true)
    set_description("Enable ISPC SIMD backend when available")
option_end()

target("xent_core")
    set_kind("static")
    add_includedirs("include", {public = true})
    add_headerfiles("include/xent/*.h")
    add_files("src/core/*.c", "src/layout/*.c", "src/text/*.c", "src/cli/*.c", "src/plugins/*.c")
    add_cflags("-ffunction-sections", "-fdata-sections", { force = true })
    if has_config("simd") then
        add_defines("XENT_ENABLE_SIMD=1", {public = true})
    else
        add_defines("XENT_ENABLE_SIMD=0", {public = true})
    end
    if has_config("ispc") then
        add_defines("XENT_ISPC_ENABLED=1", {public = true})

        -- Register the generated-header include directory at load time so
        -- every C source that #includes the ISPC header can find it.
        on_load(function (target)
            local headerdir = path.join(target:autogendir(), "rules", "ispc")
            target:add("includedirs", headerdir)
        end)

        -- Compile every .ispc file *before* any C file is compiled.
        -- This guarantees the generated _ispc.h header already exists when
        -- xent_simd.c is built.  Multi-target builds (e.g. sse4 + avx2)
        -- produce per-ISA object files that are all added to the link.
        before_build(function (target)
            import("lib.detect.find_program")
            local ispc_bin = find_program("ispc")
            assert(ispc_bin, "ispc compiler not found on PATH – install via `scoop install ispc`")

            local headerdir = path.join(target:autogendir(), "rules", "ispc")
            local objdir    = path.join(target:autogendir(), "rules", "ispc", "objs")
            os.mkdir(headerdir)
            os.mkdir(objdir)

            local ispc_targets = {"sse4", "avx2"}
            local targets_str  = table.concat(ispc_targets, ",")

            local sources = os.files(path.join(os.scriptdir(), "src/simd/*.ispc"))
            for _, sourcefile in ipairs(sources) do
                local basename   = path.basename(sourcefile)
                local objectfile = path.join(objdir, basename .. ".obj")
                local headerfile = path.join(headerdir, basename .. "_ispc.h")

                -- Incremental: skip when source has not changed.
                local src_mtime = os.mtime(sourcefile)
                local obj_mtime = os.mtime(objectfile)
                if src_mtime > obj_mtime then
                    local argv = {
                        sourcefile,
                        "-o", objectfile,
                        "-h", headerfile,
                        "--target=" .. targets_str,
                        "--arch=x86-64",
                        "--opt=fast-math",
                    }
                    if is_mode("debug") then
                        table.insert(argv, "-g")
                    end
                    os.vrunv(ispc_bin, argv)
                end

            end
        end)

        -- After the normal archive is produced, append the ISPC object files
        -- into the static library so consumers link them transparently.
        after_build(function (target)
            local objdir = path.join(target:autogendir(), "rules", "ispc", "objs")
            local objs = os.files(path.join(objdir, "*.obj"))
            if #objs > 0 then
                local ar = target:tool("ar")
                os.vrunv(ar, table.join({"rcs", target:targetfile()}, objs))
            end
        end)
    else
        add_defines("XENT_ISPC_ENABLED=0", {public = true})
    end

for _, demo in ipairs({"demo_basic", "demo_flex_vs_swiftstack", "demo_dump_json"}) do
    target(demo)
        set_kind("binary")
        add_files("examples/" .. demo .. ".c")
        add_deps("xent_core")
        add_includedirs("include")
end

for _, bench_name in ipairs({"bench_layout", "bench_simd", "bench_compare_recursive_baseline", "bench_dirty_vs_full", "bench_regression_gates"}) do
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
    "test_layout_grid",
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
            "test_layout_grid",
        }
        for _, t in ipairs(tests) do
            os.exec("xmake build " .. t)
            os.exec("xmake run " .. t)
        end
    end)
