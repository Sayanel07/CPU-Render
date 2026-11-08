#pragma once
#include <glm/vec4.hpp>

/// <summary>
/// General size (can vary depending on sytems):
/// <para> - 1 bytes </para>
/// <para> - 8 bits </para>
/// </summary>
using byte = unsigned char;

/// <summary>
/// General size (can vary depending on sytems):
/// <para> - 2 bytes </para>
/// <para> - 16 bits </para>
/// </summary>
using ushort = unsigned short;

/// <summary>
/// General size (can vary depending on sytems):
/// <para> - 4 bytes </para>
/// <para> - 32 bits </para>
/// </summary>
using uint = unsigned int;
	
/// <summary>
/// General size (can vary depending on sytems):
/// <para> - 4 bytes </para>
/// <para> - 32 bits </para>
/// </summary>
using ulong = unsigned long;

/// <summary>
/// General size (can vary depending on sytems):
/// <para> - 8 bytes </para>
/// <para> - 64 bits </para>
/// </summary>
using ulonglong = unsigned long long;

/// <summary>
/// UTF8 C-style string
/// </summary>
using cstr = const char*;
	
/// <summary>
/// UTF16 C-style string
/// </summary>
using wcstr = const wchar_t*;
	
using int8   = signed char;
using int16  = short;
using int32  = int;
using int64  = long long;
using uint8  = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

using Color = glm::vec4;