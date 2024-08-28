add_requires("gtest")

-- Make all files in the unit directory into targets
for _, file in ipairs(os.files("./unit/**.cpp")) do
    local name = path.basename(file)
    target("test_" .. name)
        set_kind("binary")
        set_default(false)
        on_config(function (target) 
            if not target:has_cxxincludes("format") then 
                if has_package("fmt") then
                    target:add("packages", "fmt")
                else 
                    target:add("defines", "LLWFLOWS_NDEBUG")
                end
            end
        end)
        add_files(file)
        add_files("../workflows/**.cpp")
        add_tests(name)
        add_packages("gtest")
        add_defines("LLWFLOWS_STATIC", "LLWFLOWS_LOG_CONTEXT", "LLWFLOWS_LOG_LEVEL_DEBUG")
    target_end()
end
