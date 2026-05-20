# 修复 PluginSystem.cpp 中的编码问题

with open('D:/C++/wealth-pilot/src/core/plugin/PluginSystem.cpp', 'r', encoding='utf-8', errors='replace') as f:
    lines = f.readlines()

# 修复特定行
fixed_lines = []
for i, line in enumerate(lines):
    # 检查是否有编码问题
    if '\ufffd' in line:
        # 替换损坏的字符
        line = line.replace('\ufffd', '')
        # 检查是否是注释后面跟着变量声明的情况
        if 'PluginInfo info;' in line:
            # 分离注释和变量声明
            parts = line.split('PluginInfo info;')
            if len(parts) == 2:
                line = parts[0].rstrip() + '\n    PluginInfo info;' + parts[1]
        elif 'QString errorMsg;' in line:
            parts = line.split('QString errorMsg;')
            if len(parts) == 2:
                line = parts[0].rstrip() + '\n    QString errorMsg;' + parts[1]
        elif 'QVariantList values;' in line:
            parts = line.split('QVariantList values;')
            if len(parts) == 2:
                line = parts[0].rstrip() + '\n        QVariantList values;' + parts[1]
    fixed_lines.append(line)

with open('D:/C++/wealth-pilot/src/core/plugin/PluginSystem.cpp', 'w', encoding='utf-8', newline='\r\n') as f:
    f.writelines(fixed_lines)

print('Done')
