import re

with open('D:/C++/wealth-pilot/src/ai/service/AIService.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 精确替换 - 使用正则确保不会重复替换
# 先替换 std::function 参数中的 Result
content = re.sub(r'std::function<void\(Result<QString>\)', 'std::function<void(AIResult<QString>)>', content)
content = re.sub(r'std::function<void\(Result<AIAnalysis>\)', 'std::function<void(AIResult<AIAnalysis>)>', content)

# 替换函数返回类型
content = re.sub(r'Result<QString> AIService::chatSync', 'AIResult<QString> AIService::chatSync', content)

# 替换 lambda 参数
content = re.sub(r'\(Result<QString> (result|r)\)', r'(AIResult<QString> \1)', content)

# 替换方法调用 - 使用负向断言避免重复替换
content = re.sub(r'(?<!AI)Result<QString>::err\(ErrorCode::[^,]+,\s*"', 'AIResult<QString>::error("', content)
content = re.sub(r'(?<!AI)Result<QString>::ok\(', 'AIResult<QString>::ok(', content)
content = re.sub(r'(?<!AI)Result<QString>::error\(', 'AIResult<QString>::error(', content)
content = re.sub(r'(?<!AI)Result<AIAnalysis>::ok\(', 'AIResult<AIAnalysis>::ok(', content)
content = re.sub(r'(?<!AI)Result<AIAnalysis>::error\(', 'AIResult<AIAnalysis>::error(', content)
content = re.sub(r'(?<!AI)Result<AIAnalysis>::fromError\(result\.error\(\)\)',
                 'AIResult<AIAnalysis>::error(result.errorMessage())', content)

# 替换 unwrap 为 value
content = content.replace('r.unwrap()', 'r.value()')
content = content.replace('result.unwrap()', 'result.value()')

# 删除不需要的 ErrorCode 相关代码
content = content.replace('ErrorCode errorCode = ErrorCode::Success;\n    ', '')
content = content.replace('errorCode = r.errorCode();\n            ', '')

with open('D:/C++/wealth-pilot/src/ai/service/AIService.cpp', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print('Done')
