#! /usr/bin/env julia

using REPL

valid_keys = sort([
    k for (k, v) in REPL.REPLCompletions.latex_symbols
    if isascii(k) && k[1] == '\\' && length(v) == 1
])

to_hex(c) = "0x"*string(convert(Int, c), base=16)

format_wchar_array(s) = "{ " * join(to_hex.(vcat(collect(s), [0])), ", ") * " }"

println("static const struct mapping latex_mappings[] = {")
for k in valid_keys
    v = REPL.REPLCompletions.latex_symbols[k]
    println("\t{ $(format_wchar_array(k)), $(to_hex(v[1])) },")
end
println("};")
