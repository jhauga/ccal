@echo off
REM win_ccal_cmd_test
::  Quick test, running ccal command line tool using several formats and `-q, --quote` option.

echo Testing integer formatting for decimal calculations
echo.

echo Test 1: 1 + 1.00 should output 2 (not 2.00)
ccal.exe 1 + 1.00

echo.
echo Test 2: 5.0000 + 5.0000 should output 10 (not 10.0000)
ccal.exe 5.0000 + 5.0000

echo.
echo Test 3: 2.50 * 4 should output 10 (not 10.00)
ccal.exe 2.50 x 4

echo.
echo Test 4: 10.00 / 2.00 should output 5 (not 5.00)
ccal.exe 10.00 / 2.00

echo.
echo Test 5: 1.50 + 1.00 should output 2.50 (keep meaningful decimals)
ccal.exe 1.50 + 1.00

echo.
echo Test 6: 1.01 - 1.00 should output 0.01 (keep small decimals)
ccal.exe 1.01 - 1.00

echo.
echo Test 7: Complex expression (2.00 + 3.00) * 2.00 should output 10
ccal.exe -q "(2.00 + 3.00) * 2.00"
ccal.exe --quote "(2.00 + 3.00) * 2.00"

echo.
echo Test 8: Regular integer calculation 5 + 3 should output 8
ccal.exe 5 + 3

echo.
echo All tests completed!