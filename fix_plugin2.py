# 修复 PluginSystem.cpp 中的编码问题

with open('D:/C++/wealth-pilot/src/core/plugin/PluginSystem.cpp', 'r', encoding='utf-8', errors='replace') as f:
    content = f.read()

# 修复被截断的中文字符串
replacements = [
    ("'reason': '无明确信�?}'", "'reason': '无明确信号'}"),
    ("'message': '预警消息',", "'message': '预警消息',"),
]

for old, new in replacements:
    content = content.replace(old, new)

with open('D:/C++/wealth-pilot/src/core/plugin/PluginSystem.cpp', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print('Done')
