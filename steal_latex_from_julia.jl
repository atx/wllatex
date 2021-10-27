#! /usr/bin/env julia

using REPL

symbol_table = merge(
    REPL.REPLCompletions.latex_symbols,
    REPL.REPLCompletions.emoji_symbols
)

valid_keys = sort([
    k for (k, v) in symbol_table
    if isascii(k) && k[1] == '\\' && length(v) == 1
])

to_hex(c) = "0x"*string(convert(Int, c), base=16)

format_wchar_array(s) = "{ " * join(to_hex.(vcat(collect(s), [0])), ", ") * " }"

println("static const struct mapping latex_mappings[] = {")
for k in valid_keys
    v = symbol_table[k]
    println("\t{ $(format_wchar_array(k)), $(to_hex(v[1])) },")
end
println("};")
