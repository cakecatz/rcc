#!/bin/bash

GREEN_TEXT="\e[92m"
RESET_TEXT="\e[0m"

# colored_text text color_code
colored_text() {
  echo -e "\e[${2}m${1}\e[0m"
}

try() {
  expected="$1"
  input="$2"

  ./rcc "$input" >tmp.s
  gcc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    colored_text "${input} => ${expected} failures" 91
    colored_text "Expected" 92
    echo $expected
    colored_text "Received" 91
    echo $actual

    colored_text "===== assembla code =====" 96
    cat tmp.s
    colored_text "=========================" 96

    exit 1
  fi
}

try 0 0
try 42 42
try 21 '5+20-4'
try 41 " 12 + 34 - 5 "
try 47 "5+6*7"
try 15 "5*(9-6)"
try 4 "(3+5)/2"
try 5 "-10+15"

# for equal
try 1 "5==5"
try 0 "5==4"

# for not equal

try 0 "5!=5"
try 1 "5!=4"

# for gt
try 0 "3 > 4"
try 0 "4 > 4"

# for gte
try 1 "4 >= 4"

# for lt
try 1 "3 < 4"
try 0 "4 < 4"

# for lte
try 1 "4 <= 4"

echo OK
