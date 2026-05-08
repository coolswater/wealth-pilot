# Fix PortfolioPage.cpp hardcoded colors
$filePath = "D:\C++\wealth-pilot\src\views\portfolio\PortfolioPage.cpp"
$content = Get-Content $filePath -Raw

# Fix QProgressBar stylesheet - add .arg() call
$content = $content -replace '(\s+QProgressBar::chunk \{[^}]+border-radius: 4px;\r?\n\s+\}\r?\n\s+"\))', '$1.arg(Tokens::Colors::Border, Tokens::Colors::Success, Tokens::Colors::Warning, Tokens::Colors::Danger));'
$content = $content -replace '("\)\.arg\(Tokens::Colors::Border, Tokens::Colors::Success, Tokens::Colors::Warning, Tokens::Colors::Danger\)\);', '$1'

# Fix remaining Tokens::Colors::Border in stylesheets
$content = $content -replace 'background-color: Tokens::Colors::Border;', 'background-color: %1;'
$content = $content -replace 'gridline-color: Tokens::Colors::Border;', 'gridline-color: %1;'
$content = $content -replace 'selection-background-color: Tokens::Colors::Border;', 'selection-background-color: %1;'
$content = $content -replace 'border: 1px solid Tokens::Colors::Border;', 'border: 1px solid %1;'

# Fix QComboBox stylesheet
$content = $content -replace '(\s+QComboBox QAbstractItemView \{[^}]+selection-background-color: )%1;', '$1Tokens::Colors::Border;'

$content | Set-Content $filePath -Encoding UTF8
Write-Host "Fixed hardcoded colors in PortfolioPage.cpp"
