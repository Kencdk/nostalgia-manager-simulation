path = r'D:\DEV\Nostalgia\NostalgiaManager\NostalgiaManager.vcxproj'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Fix the ClInclude malformed line (backtick-n literal + duplicate)
old = 'src\\data\\Database.h" />`n    <ClInclude Include="src\\data\\XmlRules.h" />\n    <ClInclude Include="src\\data\\XmlRules.h" />'
new  = 'src\\data\\Database.h" />\n    <ClInclude Include="src\\data\\XmlRules.h" />'

if old in content:
    content = content.replace(old, new)
    print('Fixed ClInclude entries')
else:
    idx = content.find('Database.h')
    print('Not found. Context:', repr(content[max(0,idx-5):idx+200]))

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print('Done')


