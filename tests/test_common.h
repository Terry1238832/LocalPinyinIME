#pragma once

#include <iostream>
#include <string>

#define REQUIRE_TRUE(expr) do { if (!(expr)) { std::wcerr << L"Requirement failed: " << L#expr << L"\n"; return 1; } } while (false)
#define REQUIRE_EQ(left, right) do { if (!((left) == (right))) { std::wcerr << L"Requirement failed: " << L#left << L" == " << L#right << L"\n"; return 1; } } while (false)
