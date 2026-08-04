import sys

path = r'D:\DEV\Nostalgia\NostalgiaManager\NostalgiaManager.vcxproj'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Add sqlite to include dirs
old = r'..\third_party\imgui;..\third_party\imgui\backends;..\third_party\stb;%(AdditionalIncludeDirectories)'
new = r'..\third_party\imgui;..\third_party\imgui\backends;..\third_party\stb;..\third_party\sqlite;%(AdditionalIncludeDirectories)'
count1 = content.count(old)
content = content.replace(old, new)

# Add new compile entries
old2 = '<ClCompile Include="src\\data\\Database.cpp" />'
new2 = (
    '<ClCompile Include="src\\data\\Database.cpp" />\n'
    '    <ClCompile Include="src\\data\\DatabaseSqlite.cpp" />\n'
    '    <ClCompile Include="..\\third_party\\sqlite\\sqlite3.c">'
    '<PreprocessorDefinitions>SQLITE_DEFAULT_MEMSTATUS=0;SQLITE_THREADSAFE=1;%(PreprocessorDefinitions)</PreprocessorDefinitions>'
    '</ClCompile>'
)
count2 = content.count(old2)
content = content.replace(old2, new2)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print(f"include dirs replaced: {count1}, ClCompile entries replaced: {count2}")
