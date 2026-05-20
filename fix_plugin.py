import re

with open('D:/C++/wealth-pilot/src/core/plugin/PluginSystem.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 修复被截断的注释和变量声明
fixes = [
    (r'// 解析插件元数据\?.*?PluginInfo info;', '// 解析插件元数据\n    PluginInfo info;'),
    (r'// 验证脚本\?.*?QString errorMsg;', '// 验证脚本\n    QString errorMsg;'),
    (r'// 解析指标\?.*?QVariantList values;', '// 解析指标\n        QVariantList values;'),
]

for pattern, replacement in fixes:
    content = re.sub(pattern, replacement, content)

# 修复其他编码问题
# 查找并修复第 474 行附近的问题
lines = content.split('\n')
for i, line in enumerate(lines):
    if '?' in line and not line.strip().startswith('//'):
        # 检查是否是编码问题
        if 'wrapper' in line or 'info' in line or 'values' in line:
            print(f"Line {i + 1}: {line}")

with open('D:/C++/wealth-pilot/src/core/plugin/PluginSystem.cpp', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print('Done')
