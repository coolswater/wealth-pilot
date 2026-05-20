# 修复 PluginSystem.cpp 中的编码问题 - 使用字节级别操作

with open('D:/C++/wealth-pilot/src/core/plugin/PluginSystem.cpp', 'rb') as f:
    content = f.read()

# 修复被截断的注释和变量声明
# 这些问题是由于 UTF-8 字符被截断导致的

# 1. 修复 "解析插件元数据" 后面的截断
# 原始: // 解析插件元数据?    PluginInfo info;
# 目标: // 解析插件元数据\n    PluginInfo info;
content = content.replace(
    b'// \xe8\xa7\xa3\xe6\x9e\x90\xe6\x8f\x92\xe4\xbb\xb6\xe5\x85\x83\xe6\x95\xb0\xe6\x8d\xa?\xe3?\x80\x80\x80\x80PluginInfo info;',
    b'// \xe8\xa7\xa3\xe6\x9e\x90\xe6\x8f\x92\xe4\xbb\xb6\xe5\x85\x83\xe6\x95\xb0\xe6\x8d\xae\r\n    PluginInfo info;'
)

# 2. 修复 "验证脚本" 后面的截断
content = content.replace(
    b'// \xe9\xaa\x8c\xe8\xaf\x81\xe8\x84\x9a\xe6\x9c\xac?\xe3?\x80\x80\x80\x80QString errorMsg;',
    b'// \xe9\xaa\x8c\xe8\xaf\x81\xe8\x84\x9a\xe6\x9c\xac\r\n    QString errorMsg;'
)

# 3. 修复 "解析指标" 后面的截断
content = content.replace(
    b'// \xe8\xa7\xa3\xe6\x9e\x90\xe6\x8c\x87\xe6\x87\x87\xe6\x87\x87?\xe3?\x80\x80\x80\x80\x80\x80QVariantList values;',
    b'// \xe8\xa7\xa3\xe6\x9e\x90\xe6\x8c\x87\xe6\x87\x87\xe6\x87\x87\r\n        QVariantList values;'
)

with open('D:/C++/wealth-pilot/src/core/plugin/PluginSystem.cpp', 'wb') as f:
    f.write(content)

print('Done')
