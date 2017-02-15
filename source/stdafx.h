#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cpr/cpr.h>
#include <cpr/multipart.h>
#include "Hooking.Patterns.h"
#include "IniReader.h"
#include "json/reader.h"
#include "injector/injector.hpp"
#include "injector/assembly.hpp"
#include "injector/hooking.hpp"
#include "injector/calling.hpp"
#include "injector/utility.hpp"