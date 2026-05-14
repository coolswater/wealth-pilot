#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
DCF (Discounted Cash Flow) 估值模型

用法:
    python dcf.py <symbol> [years] [growth_rate] [discount_rate]

输出:
    JSON 格式的估值结果
"""

import json
import sys
from typing import Dict, Any, Optional


def calculate_dcf(
        symbol: str,
        years: int = 5,
        growth_rate: float = 0.10,
        discount_rate: float = 0.12,
        terminal_growth: float = 0.03
) -> Dict[str, Any]:
    """
    计算 DCF 估值
    
    Args:
        symbol: 股票代码
        years: 预测年数
        growth_rate: 年增长率
        discount_rate: 折现率
        terminal_growth: 永续增长率
    
    Returns:
        估值结果字典
    """
    # 模拟数据（实际应从 API 获取）
    current_fcf = 1000  # 当前自由现金流（百万）

    # 计算未来现金流
    future_fcfs = []
    for year in range(1, years + 1):
        fcf = current_fcf * ((1 + growth_rate) ** year)
        future_fcfs.append(fcf)

    # 计算折现值
    discounted_fcfs = []
    for i, fcf in enumerate(future_fcfs):
        discount_factor = 1 / ((1 + discount_rate) ** (i + 1))
        discounted_fcfs.append(fcf * discount_factor)

    # 计算终值
    terminal_fcf = future_fcfs[-1] * (1 + terminal_growth)
    terminal_value = terminal_fcf / (discount_rate - terminal_growth)
    discounted_terminal = terminal_value / ((1 + discount_rate) ** years)

    # 计算企业价值
    enterprise_value = sum(discounted_fcfs) + discounted_terminal

    # 模拟净债务
    net_debt = 500

    # 计算股权价值
    equity_value = enterprise_value - net_debt

    # 模拟股本
    shares_outstanding = 100

    # 计算每股价值
    fair_value_per_share = equity_value / shares_outstanding

    return {
        "symbol": symbol,
        "enterprise_value": round(enterprise_value, 2),
        "equity_value": round(equity_value, 2),
        "fair_value_per_share": round(fair_value_per_share, 2),
        "assumptions": {
            "years": years,
            "growth_rate": f"{growth_rate * 100:.1f}%",
            "discount_rate": f"{discount_rate * 100:.1f}%",
            "terminal_growth": f"{terminal_growth * 100:.1f}%"
        },
        "details": {
            "current_fcf": current_fcf,
            "future_fcfs": [round(f, 2) for f in future_fcfs],
            "discounted_fcfs": [round(f, 2) for f in discounted_fcfs],
            "terminal_value": round(terminal_value, 2),
            "discounted_terminal": round(discounted_terminal, 2),
            "net_debt": net_debt,
            "shares_outstanding": shares_outstanding
        }
    }


def main():
    if len(sys.argv) < 2:
        print(json.dumps({
            "error": "Usage: python dcf.py <symbol> [years] [growth_rate] [discount_rate]"
        }))
        sys.exit(1)

    symbol = sys.argv[1]
    years = int(sys.argv[2]) if len(sys.argv) > 2 else 5
    growth_rate = float(sys.argv[3]) if len(sys.argv) > 3 else 0.10
    discount_rate = float(sys.argv[4]) if len(sys.argv) > 4 else 0.12

    try:
        result = calculate_dcf(symbol, years, growth_rate, discount_rate)
        print(json.dumps(result, indent=2, ensure_ascii=False))
    except Exception as e:
        print(json.dumps({"error": str(e)}))
        sys.exit(1)


if __name__ == "__main__":
    main()
