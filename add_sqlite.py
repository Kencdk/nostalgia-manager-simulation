import sys
path = r'D:\DEV\Nostalgia\NostalgiaManager\NostalgiaManager.vcxproj'
with open(path, 'r', encoding='utf-8') as f:
    txt = f.read()

target = '    <ClCompile Include="src\\data\\DatabaseSqlite.cpp" />'
insert = (
    '\n    <ClCompile Include="..\\third_party\\sqlite\\sqlite3.c">'
    '\n      <PreprocessorDefinitions>SQLITE_DEFAULT_MEMSTATUS=0;SQLITE_THREADSAFE=1;%(PreprocessorDefinitions)</PreprocessorDefinitions>'
    '\n    </ClCompile>'
)
if target not in txt:
    print("ERROR: target not found")
    sys.exit(1)
txt2 = txt.replace(target, target + insert, 1)
with open(path, 'w', encoding='utf-8') as f:
    f.write(txt2)
print("Done. sqlite3.c count:", txt2.count('sqlite3.c'))
