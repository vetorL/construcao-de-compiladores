#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
lexer="$project_dir/cobol-lexer"
temporary_dir=$(mktemp -d)
trap 'rm -rf "$temporary_dir"' EXIT HUP INT TERM

fail() {
    echo "FAIL: $1" >&2
    exit 1
}

run_ok() {
    name=$1
    "$lexer" "$project_dir/tests/$name.cob" \
        >"$temporary_dir/$name.out" 2>"$temporary_dir/$name.err" ||
        fail "$name.cob should scan successfully"
    test ! -s "$temporary_dir/$name.err" || fail "$name.cob wrote diagnostics"
}

for fixture in basic keywords identifiers numbers strings comments continuation columns period punctuation; do
    run_ok "$fixture"
done

grep -q '1:8.*IDENTIFICATION.*"IDENTIFICATION"' "$temporary_dir/basic.out" ||
    fail "basic token/location"
grep -q '2:8.*PROGRAM-ID.*"PROGRAM-ID"' "$temporary_dir/basic.out" ||
    fail "PROGRAM-ID keyword"
grep -q '3:8.*IDENTIFICATION.*"identification"' "$temporary_dir/keywords.out" ||
    fail "case-insensitive keyword comparison"
grep -q 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA' "$temporary_dir/identifiers.out" ||
    fail "30-character identifier"
grep -q 'NUMBER.*"+123"' "$temporary_dir/numbers.out" || fail "signed integer"
grep -q 'NUMBER.*"123.45"' "$temporary_dir/numbers.out" || fail "decimal number"
grep -q 'STRING.*"\\"HELLO WORLD\\""' "$temporary_dir/continuation.out" ||
    fail "continued string"
test "$(grep -c 'DISPLAY' "$temporary_dir/comments.out")" -eq 1 || fail "comment lines"
grep -q '1:8.*IDENTIFICATION' "$temporary_dir/columns.out" || fail "sequence area"
! grep -q 'IGNORED' "$temporary_dir/columns.out" || fail "columns 73 onward"

period_numbers=$(grep -c 'NUMBER' "$temporary_dir/period.out")
period_periods=$(grep -c 'PERIOD' "$temporary_dir/period.out")
test "$period_numbers" -eq 4 || fail "period numeric count"
test "$period_periods" -eq 3 || fail "period punctuation count"
for token_name in LPAREN RPAREN COMMA SEMICOLON PERIOD; do
    grep -q "$token_name" "$temporary_dir/punctuation.out" ||
        fail "punctuation token $token_name"
done

if "$lexer" "$project_dir/tests/errors.cob" \
    >"$temporary_dir/errors.out" 2>"$temporary_dir/errors.err"; then
    fail "errors.cob should return exit status 1"
else
    status=$?
    test "$status" -eq 1 || fail "lexical error exit status"
fi
grep -q "invalid character" "$temporary_dir/errors.err" || fail "invalid character recovery"
grep -q "identifier cannot end" "$temporary_dir/errors.err" || fail "trailing hyphen"
grep -q "30-character limit" "$temporary_dir/errors.err" || fail "identifier length"
grep -q "unterminated double-quoted string" "$temporary_dir/errors.err" || fail "unterminated string"
grep -q "continuation indicator" "$temporary_dir/errors.err" || fail "orphan continuation"
grep -q 'MOVE.*"MOVE"' "$temporary_dir/errors.out" || fail "continued scanning after error"

if "$lexer" >"$temporary_dir/usage.out" 2>"$temporary_dir/usage.err"; then
    fail "missing filename should return exit status 2"
else
    status=$?
    test "$status" -eq 2 || fail "CLI error exit status"
fi

cc -std=c11 -Wall -Wextra -Wpedantic -I"$project_dir/src" \
    "$project_dir/tests/test_api.c" "$project_dir/src/lexer.c" \
    "$project_dir/src/token.c" "$project_dir/src/fixed_format.c" \
    "$project_dir/src/lexer.yy.c" -o "$temporary_dir/test_api"
"$temporary_dir/test_api" || fail "reentrant API and EOF"

echo "All lexer tests passed."
