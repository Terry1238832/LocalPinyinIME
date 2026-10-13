#!/usr/bin/env python3
"""First-party reusable everyday, chat, office and connective short expressions."""
from __future__ import annotations
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
from pypinyin import Style, lazy_pinyin, pinyin

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent

# Each item is independently chosen for ordinary use. Longer items are fixed
# expressions and reusable phrase fragments, not arbitrary subject/verb objects.
SECTIONS = [
    ("conversational_replies_and_clarification", 6000, """
    收到 收到啦 收到了 已收到 刚看到 看到了 看见了 看过了 听到了 知道啦
    明白啦 明白了 了解了 清楚了 搞清楚了 弄明白了 看明白了 听明白了
    大概明白 基本明白 完全明白 差不多懂 还不太懂 不太明白 不太清楚
    说清楚一点 讲清楚一点 再说一次 再说一遍 再讲一遍 再讲一次 再解释一下
    解释一下 说明一下 展开说说 详细说说 具体说说 简单说说 说得具体点
    举个例子看看 换个说法 换种说法 换句话讲 换一种说法 换个角度看
    刚才没看清 刚才没听清 没听明白 没看明白 没弄明白 没说清楚 没讲清楚
    我理解错了 我记错了 我看错了 我听错了 我搞错了 刚才看错了 刚才记错了
    理解有误 表达有误 表达不清 表述不清 有点误会 有些误会 有点歧义
    确认无误 核对无误 没有问题 没啥大问题 问题不大 应该没问题 应该可以
    这样可以 这样也行 这样就行 这样就好 就这么定 就这样定 就这么办 就这样办
    暂时这样 先这样吧 先这么办 先这么定 先看看情况 先确认一下 先核对一下
    不太合适 不太方便 不太确定 不太可能 不太现实 不是很懂 不是很清楚
    有点不懂 有些不懂 大概知道 大概了解 基本了解 基本清楚 具体不清楚
    具体怎么做 具体怎么办 具体怎么弄 具体怎么说 到底怎么回事 到底怎么做
    能否说明 能否解释 能否确认 能否提供 是否方便 是否合适 是否可行 是否有空
    大致如此 基本如此 情况如此 确实是这样 确实如此 确实不错 确实很好
    的确如此 的确不错 说得没错 说得很对 说得有道理 说得很清楚 说得挺好
    有一定道理 挺有道理 很有道理 这倒也是 也是如此 也是这样 差不多这样
    应该是这样 应该没错 应该是的 应该不是 好像是这样 好像不是 大概不是
    未必是这样 不一定如此 不一定是的 不一定能行 看具体情况 得看情况
    看实际情况 看当时情况 看个人情况 根据情况 视情况而定 按实际情况
    问一下看看 查一下看看 看一下再说 问清楚再说 查清楚再说 确认后再说
    再确认一次 再核对一次 再检查一遍 再看看吧 再想想吧 再考虑考虑
    不妨试试 不妨看看 可以试试 可以看看 试试看吧 试一下吧 看看再说
    没想到如此 没想到这样 还真是这样 原来是这样 原来如此 难怪如此
    """),
    ("chat_care_and_polite_requests", 6000, """
    辛苦你了 辛苦大家了 大家辛苦了 辛苦一下 辛苦帮忙 辛苦查看 辛苦确认
    麻烦你了 麻烦大家 麻烦一下 麻烦看看 麻烦确认 麻烦查收 麻烦帮忙
    麻烦帮一下 麻烦问一下 麻烦看一下 麻烦查一下 麻烦等一下 麻烦再发一下
    帮忙看看 帮忙问问 帮忙确认 帮忙核对 帮忙查看 帮忙查一下 帮忙看一下
    帮忙发一下 帮忙找一下 帮忙带一下 帮忙拿一下 帮忙转一下 帮忙改一下
    帮我确认一下 帮我核对一下 帮我查看一下 帮我找一下 帮我改一下 帮我问一下
    请帮忙确认 请帮忙查看 请帮忙核对 请帮忙查收 请帮忙处理 请帮忙看看
    请看一下 请查一下 请问一下 请稍等一下 请稍等片刻 请等一下 请再等一会
    方便的话 有空的话 可以的话 没问题的话 不忙的话 时间允许的话 有时间的话
    如果方便 如果可以 如果有空 如果有时间 如果不忙 如果不急 如果需要
    有空的时候 方便的时候 不忙的时候 空闲的时候 有时间的时候 忙完的时候
    看见后回复 看到后回复 看到请回复 看到请确认 看见请回复 收到请回复
    收到请确认 收到后回复 收到后确认 有空回复 方便回复 有空再回复 方便再回复
    回复一下 回复一下吧 回个消息 回下消息 抽空回复 抽空看看 抽空确认
    等你回复 等你消息 等你确认 等你通知 有消息了 有消息再说 有消息再联系
    随时联系 随时沟通 随时告诉我 随时跟我说 有问题再说 有问题找我
    有事找我 有事联系 有事再说 有事再联系 有事随时说 随时找我
    没事就好 没事就行 没什么事 不是什么大事 不算什么 不是什么问题
    不用客气 不要客气 不必客气 不用见外 不要见外 不用担心 不要担心
    不必担心 不用着急 不要着急 不用急着 不急着做 不急着回复 不着急回复
    别太着急 别太担心 别太紧张 别太辛苦 别太累 别熬太晚 别睡太晚
    早点休息 早点睡觉 早点睡吧 早点休息吧 记得休息 记得吃饭 记得喝水
    注意饮食 注意防晒 注意降温 注意补水 注意天气 注意路况 注意脚下
    路上注意安全 路上小心 出门小心 开车小心 骑车小心 慢点开 慢点骑 慢点走
    到家报个平安 到家说一声 到家告诉我 到了说一声 到了告诉我 回来告诉我
    方便说一声 有空说一声 有事说一声 有问题说一声 有变化说一声 有空聊聊
    一起聊聊 改天好好聊 下次好好聊 改天再聚 下次再聚 有空再聚 有空见面
    有空一起吃饭 改天一起吃饭 下次一起吃饭 下次一起去 改天一起去 有空一起去
    谢谢你提醒 感谢你提醒 谢谢你帮忙 感谢你帮忙 多谢你帮忙 谢谢你的理解
    感谢你的理解 感谢你的支持 谢谢你的支持 感谢你的关心 谢谢你的关心
    不好意思啊 不好意思了 真不好意思 实在不好意思 刚才不好意思 让你久等了
    抱歉让你久等 久等了 等久了吧 久等啦 打扰一下 打扰你了 冒昧打扰
    祝你顺利 祝你开心 祝你好运 祝一切顺利 祝工作顺利 祝旅途愉快
    """),
    ("office_correspondence_and_delivery", 6000, """
    请查收 请审阅 请阅知 请确认 请核对 请审核 请回复 请反馈 请转发
    敬请查收 敬请审阅 敬请确认 敬请回复 敬请反馈 烦请查收 烦请确认
    烦请回复 烦请反馈 烦请核对 烦请审核 烦请查看 请查收附件 请查看附件
    请审阅附件 请核对附件 附件请查收 附件请查看 附件请审阅 附件请核对
    请查阅附件 请参阅附件 详见附件 见附件 附件如下 附件已发 附件已更新
    资料已发送 材料已发送 文件已发送 邮件已发送 报告已发送 表格已发送
    资料已收到 材料已收到 文件已收到 邮件已收到 报告已收到 表格已收到
    资料已更新 材料已更新 文件已更新 表格已更新 文档已更新 报告已更新
    资料已整理 材料已整理 文件已整理 文档已整理 已整理好 已整理完
    更新后的版本 修改后的版本 最新的版本 当前的版本 原来的版本 上一版本
    更新版 修改版 修订版 调整版 校对版 审阅版 审核版 确认版 最终版
    终版 定稿 定稿版 初稿版 修改稿 修订稿 审阅稿 最终稿 当前稿
    请以此为准 以此为准 以附件为准 以邮件为准 以通知为准 以最新为准
    仅供参考 供参考 供您参考 供大家参考 敬请参考 供讨论 供内部参考
    供审阅 供审核 供确认 供核对 供查阅 供讨论参考 供后续参考
    请确认收到 请回复确认 请反馈意见 请提供意见 请提出意见 请补充说明
    请补充材料 请补充信息 请提供材料 请提供信息 请提供资料 请提供附件
    请提供时间 请确认时间 请确认地点 请确认人员 请确认内容 请确认版本
    请核对信息 请核对内容 请核对数据 请核对名单 请核对数量 请核对金额
    请查看进度 请反馈进度 请更新进度 请同步进度 请同步信息 请同步情况
    请尽快处理 请尽快回复 请尽快确认 请及时处理 请及时回复 请及时反馈
    请提前准备 请提前确认 请提前安排 请按时提交 请按时完成 请按时反馈
    已经处理 已经确认 已经核对 已经审核 已经提交 已经完成 已经发送
    尚未确认 尚未处理 尚未审核 尚未提交 尚未完成 尚未发送 尚未收到
    待补充 待完善 待核对 待提交 待发送 待回复 待安排 待跟进 待办理
    补充说明 补充内容 补充意见 补充资料 补充信息 补充附件 补充文档
    已补充说明 已补充材料 已补充资料 已补充信息 已补充附件 已补充内容
    如有疑问 如有问题 如需补充 如需调整 如需修改 如需确认 如需协助
    如有变更 如有调整 如有更新 如有遗漏 如有错误 如有差异 如有异议
    敬请告知 请告知 请及时告知 请提前告知 请另行通知 将另行通知
    后续安排 后续计划 后续进度 后续事项 后续工作 后续处理 后续跟进
    持续跟进 跟进情况 处理情况 核对情况 确认情况 审核情况 完成情况
    更新情况 修改情况 补充情况 整理情况 提交情况 发送情况 接收情况
    敬请关注 请继续关注 感谢配合 多谢配合 谢谢配合 感谢协助 感谢合作
    """),
    ("reusable_connectives_and_reasoning", 6000, """
    话虽如此 即便如此 即使如此 尽管如此 虽然如此 既然如此 如此看来
    这样看来 由此看来 由此可见 从而可见 可见一斑 总体来说 总体而言
    总体上说 总体来看 整体来看 整体而言 整体来说 整体上看 大体上看
    大致来说 大致如此 大致看来 大概来说 从总体上 从整体上 从实际出发
    从长远看 从长远来看 从目前看 从目前来看 从现在看 从现在来看
    从结果看 从结果来看 从表现看 从表现来看 从数据看 从数据来看
    从这个角度 从这个方面 从这个意义 就这个问题 就这个情况 就这个方面
    从某种意义 在某种程度 某种程度上 一定程度上 在一定程度 在这个方面
    在这一点上 在这个问题上 在这个角度 上述情况 上述问题 上述内容
    以上情况 以上问题 以上内容 以下情况 以下问题 以下内容 具体情况如下
    具体来说是 具体而言是 简单而言 简单来说是 换句话讲 换种角度看
    换个角度想 反过来看 反过来想 从另一方面 另一方面是 与此同时 同时也要
    与之相比 与此相比 相比之下 相较之下 相对而言 相对来说 相对来看
    相比而言 相比来说 对比来看 对比之下 比较而言 比较来看 比较之下
    尤其如此 尤其是在 特别是在 特别是当 尤其是当 主要是因为 原因在于
    原因是因为 主要原因 主要原因是 根本原因 根本原因是 直接原因 直接原因是
    原因之一 其中之一 其中一点 其中包括 其中有些 其中部分 其中大部分
    因此需要 因此可以 因此应当 因此必须 因此建议 因此希望 因此认为
    所以需要 所以可以 所以应当 所以建议 所以希望 所以认为 所以说
    也就是说 这就是说 这意味着 这也意味着 这就说明 这也说明 这也表明
    可以看出 可以发现 可以理解 可以认为 可以说是 可以认为是 可以理解为
    可以归纳为 可以总结为 大致可以说 简单可以说 简单理解为 简单概括为
    不仅如此 不止如此 不只如此 除此以外 除此之外 在此之外 在此基础上
    基于这一点 基于这个情况 基于以上情况 基于上述情况 综合来看 综合而言
    综合来说 综合考虑 综合以上 综合上述 统筹考虑 结合实际 结合情况
    按照要求 按照规定 按照安排 按照计划 按照流程 按照实际 按照情况
    根据要求 根据规定 根据安排 根据计划 根据流程 根据实际 根据结果
    正常情况下 一般情况下 通常情况下 特殊情况下 特定情况下 必要情况下
    必要的时候 一般来说是 通常来说是 原则上说 理论上说 实际上说
    实际上是 理论上是 原则上是 通常是这样 一般是这样 大多是这样
    大多数时候 很多时候 有些时候 某些时候 必要时 可以的话 可能的话
    能够的话 只要可以 只要方便 只要需要 只要有空 只要有时间 只要不急
    如果这样 如果那样 如果如此 如果不是 如果没有 如果有的话 如果是的话
    假如这样 假如那样 假如可以 假如需要 假如没有 假如有的话 假如是的话
    既然这样 既然那样 既然可以 既然需要 既然已经 既然没有 既然有了
    """),
    ("daily_planning_and_small_actions", 6000, """
    准备一下 准备准备 收拾一下 收拾收拾 整理一下 整理整理 检查一下
    核对一下 确认一下 清理一下 清理清理 打扫一下 打扫打扫 清洗一下
    洗一洗 擦一擦 扫一扫 收一收 理一理 整一整 找一找 问一问
    查一查 试一试 量一量 比一比 对一对 改一改 写一写 记一记
    读一读 练一练 想一想 算一算 数一数 听一听 说一说 讲一讲
    走一走 转一转 逛一逛 跑一跑 动一动 歇一歇 坐一坐 躺一躺
    等一等 缓一缓 放一放 晾一晾 晒一晒 晾一会 晒一会 放一会
    歇一会 坐一会 躺一会 缓一会 动一下 转一下 逛一下 歇一下
    休息一下 休息一会 放松一下 放松放松 散散步 聊聊天 说说话
    喝点水 喝点茶 喝点热水 吃点东西 吃点水果 吃点饭 吃点零食
    补点水 补充水分 多喝点水 多吃点饭 少吃一点 少喝一点 少放一点
    多放一点 少带一点 多带一点 带一点 带一些 拿一点 拿一些 买一点 买一些
    再来一点 再来一些 再放一点 再加一点 再少一点 再多一点 再试一次
    再看一遍 再查一遍 再问一次 再想一下 再试一下 再量一下 再算一下
    先吃点东西 先喝点水 先休息一下 先坐一会 先歇一会 先放一会
    先收拾一下 先整理一下 先准备一下 先检查一下 先核对一下 先清理一下
    吃完再说 喝完再说 看完再说 读完再说 做完再说 试完再说 查完再说
    问完再说 想好再说 确认再说 等下再说 稍后再说 晚点再说 下次再说
    再看看情况 等确认后 等安排好 等准备好 等收拾好 等处理好 等完成后
    顺手带上 顺手拿上 顺手放好 顺手收好 顺手关上 顺手打开 顺手看看
    顺便带上 顺便拿上 顺便买点 顺便问问 顺便看看 顺便查查 顺便取一下
    顺路买点 顺路取下 顺路带点 顺路看看 顺路过去 顺路回来 顺路接上
    一起过去 一起回来 一起回去 一起去看 一起去买 一起吃点 一起坐坐
    凑个时间 找个时间 找个地方 找个机会 抽个时间 腾个时间 留点时间
    留出时间 留一点时间 安排时间 约好时间 约定时间 确认时间 确认好时间
    不用急着去 不用急着来 不用急着做 先不着急 先别着急 先不用急
    可以慢慢来 可以等等 看好再买 想好再买 比一比再买 比较后再买
    再决定一下 再商量一下 再安排一下 再调整一下 再准备一下 再确认一下
    安排妥当 准备妥当 处理妥当 确认妥当 处理好了 确认好了 安排好了
    """),
    ("work_discussion_and_next_steps", 6000, """
    先讨论一下 先沟通一下 先了解一下 先分析一下 先梳理一下 先确认需求
    先确认范围 先确认时间 先确认人员 先整理材料 先补充材料 先核对数据
    再讨论一下 再沟通一下 再了解一下 再分析一下 再梳理一下 再补充一下
    沟通一下 讨论一下 分析一下 了解一下 梳理一下 核实一下 评估一下
    对比一下 比较一下 汇总一下 统计一下 归纳一下 总结一下 整合一下
    调整一下 修改一下 修订一下 完善一下 补充一下 说明一下 解释一下
    把情况说清楚 把问题说清楚 把事情说清楚 说明具体情况 说明具体原因
    明确需求 明确范围 明确时间 明确目标 明确任务 明确责任 明确分工
    明确要求 明确标准 明确流程 明确步骤 明确方向 明确内容 明确重点
    了解需求 了解情况 了解进度 了解原因 了解背景 了解细节 了解流程
    核实情况 核实信息 核实数据 核实内容 核实原因 核实材料 核实结果
    讨论方案 讨论细节 讨论需求 讨论问题 讨论安排 讨论计划 讨论进度
    沟通需求 沟通方案 沟通进度 沟通安排 沟通问题 沟通情况 沟通细节
    对齐需求 对齐目标 对齐方向 对齐信息 对齐进度 对齐方案 对齐标准
    梳理需求 梳理问题 梳理流程 梳理事项 梳理思路 梳理资料 梳理材料
    梳理进度 梳理情况 梳理信息 汇总信息 汇总资料 汇总材料 汇总数据
    汇总情况 汇总问题 汇总意见 汇总进度 汇总结果 汇总需求 汇总方案
    整理思路 整理需求 整理信息 整理情况 整理结果 整理意见 整理问题
    补齐资料 补齐材料 补齐信息 补齐内容 补全资料 补全材料 补全信息
    完善方案 完善流程 完善内容 完善细节 完善材料 完善说明 完善信息
    提供建议 提出建议 提供思路 提供方案 提供支持 提供帮助 提供协助
    给予支持 给予帮助 给予协助 给出意见 给出建议 给出方案 给出说明
    提前沟通 提前协调 提前确认 提前安排 提前告知 提前说明 提前准备
    及时沟通 及时协调 及时确认 及时反馈 及时告知 及时说明 及时处理
    后续再说 后续再看 后续再议 后续再定 后续确认 后续沟通 后续讨论
    下一步安排 下一步计划 下一步工作 下一步处理 下一步跟进 下一步讨论
    下一步确认 下一步准备 下一步检查 下一步核对 下一步分析 下一步评估
    进一步讨论 进一步沟通 进一步确认 进一步说明 进一步分析 进一步评估
    进一步了解 进一步核实 进一步研究 进一步完善 进一步调整 进一步优化
    再进一步 继续推进 继续完善 继续调整 继续优化 继续处理 继续跟进
    有待确认 有待完善 有待补充 有待讨论 有待研究 有待核实 有待处理
    有待解决 有待跟进 有待评估 有待调整 有待优化 有待改进 有待观察
    大致方向 基本方向 总体方向 主要方向 初步方向 初步方案 初步计划
    初步安排 初步想法 初步意见 初步判断 初步分析 初步评估 初步结论
    """),
]

OVERRIDES = {
    "得看情况": "deikanqingkuang", "看当时情况": "kandangshiqingkuang",
    "说得没错": "shuodemeicuo", "说得很对": "shuodehendui", "说得有道理": "shuodeyoudaoli",
    "说得很清楚": "shuodehenqingchu", "说得挺好": "shuodetinghao", "说得具体点": "shuodejutidian",
    "重新": "chongxin", "不着急回复": "buzhaojihuifu", "不用着急": "buyongzhaoji",
    "不要着急": "buyaozhaoji", "别太着急": "bietaizhaoji", "先不着急": "xianbuzhaoji",
    "先别着急": "xianbiezhaoji", "不急着回复": "bujizhehuifu", "不用急着": "buyongjizhe",
    "不急着做": "bujizhezuo", "不用急着去": "buyongjizhequ", "不用急着来": "buyongjizhelai",
    "不用急着做": "buyongjizhezuo", "别熬太晚": "bieaotaiwan", "骑车小心": "qichexiaoxin",
    "谅解": "liangjie", "方便的话": "fangbiandehua", "方便的时候": "fangbiandeshihou",
    "不太方便": "butaifangbian", "是否方便": "shifoufangbian", "如果方便": "ruguofangbian",
    "请核对数量": "qingheduishuliang", "核对数据": "heduishuju", "先核对数据": "xianheduishuju",
    "修订版": "xiudingban", "调整版": "tiaozhengban", "校对版": "jiaoduiban",
    "修改后的版本": "xiugaihoudebanben", "更新后的版本": "gengxinhoudebanben",
    "请提供资料": "qingtigongziliao", "请提供材料": "qingtigongcailiao",
    "查阅": "chayue", "请查阅附件": "qingchayuefujian", "请参阅附件": "qingcanyuefujian",
    "如有疑问": "ruyouyiwen", "已整理完": "yizhengliwan", "请及时反馈": "qingjishifankui",
    "结合实际": "jieheshiji", "从长远看": "congchangyuankan", "从长远来看": "congchangyuanlaikan",
    "很多时候": "henduoshihou", "大多数时候": "daduoshushihou", "差不多这样": "chabuduozheyang",
    "给予支持": "jiyuzhichi", "给予帮助": "jiyubangzhu", "给予协助": "jiyuxiezhu",
    "给出意见": "geichuyijian", "给出建议": "geichujianyi", "给出方案": "geichufangan",
    "给出说明": "geichushuoming", "核实一下": "heshiyixia", "进一步核实": "jinyibuheshi",
    "调整一下": "tiaozhengyixia", "再调整一下": "zaitiaozhengyixia", "进一步调整": "jinyibutiaozheng",
    "继续调整": "jixutiaozheng", "有待调整": "youdaitiaozheng", "数一数": "shuyishu",
    "量一量": "liangyiliang", "转一转": "zhuanyizhuan", "转一下": "zhuanyixia",
    "比一比再买": "biyibizaimai", "想好再买": "xianghaozaimai", "想好再说": "xianghaozaishuo",
    "晾一晾": "liangyiliang", "晾一会": "liangyihui", "一会": "yihui", "歇一会": "xieyihui",
    "坐一会": "zuoyihui", "躺一会": "tangyihui", "缓一会": "huanyihui", "晒一会": "shaiyihui",
    "放一会": "fangyihui", "休息一会": "xiuxiyihui", "先坐一会": "xianzuoyihui",
    "先歇一会": "xianxieyihui", "先放一会": "xianfangyihui", "请再等一会": "qingzaidengyihui",
    "暂时这样": "zanshizheyang", "都行": "douxing", "这样也行": "zheyangyexing",
    "这样就行": "zheyangjiuxing", "不一定能行": "buyidingnengxing", "看看再说": "kankanzaishuo",
    "处置": "chuzhi", "请帮忙处理": "qingbangmangchuli", "请尽快处理": "qingjinkuaichuli",
    "请及时处理": "qingjishichuli", "已经处理": "yijingchuli", "尚未处理": "shangweichuli",
    "后续处理": "houxuchuli", "处理情况": "chuliqingkuang", "等处理好": "dengchulihao",
    "处理妥当": "chulituodang", "处理好了": "chulihaole", "下一步处理": "xiayibuchuli",
    "继续处理": "jixuchuli", "有待处理": "youdaichuli", "很有道理": "henyoudaoli",
    "可以慢慢来": "keyimanmanlai", "有待研究": "youdaiyanjiu", "有待观察": "youdaiguancha",
    "顺便查查": "shunbianchacha", "查一查": "chayicha", "查一下看看": "chayixiakankan",
}

EXCLUDED = {
    "原因是因为", "简单可以说", "实际上说", "在这个角度", "在某种程度", "在一定程度",
    "换种角度看", "从某种意义", "如果有的话", "如果是的话", "假如有的话", "假如是的话",
    "请提供时间", "以最新为准", "顺路取下", "等确认后", "下一步跟进", "先不着急的",
}
POLYPHONES = "行长重调假还得藏薄处校系载便单转觉乐宿量数供应间差弹传解着难干圈缝会空省发划钻泊铺屈强夹纶粘兴好拗露辟落翘折角率卷鲜降参盛秘斗闷蒙漂踏纤蚕蕃质颈杆壳舍脏脯咽咳骨累识刨剥拓卡扇扁冠熨择给荷腊屏尺混查"

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main() -> None:
    base = ROOT / "resources/dictionary"
    baselines = [base / name for name in ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv")]
    baselines += [OUT / f"lexicon_batch_{n:03d}.tsv" for n in range(1, 8)]
    known_keys, known_words = set(), set()
    for path in baselines:
        for line in path.read_text(encoding="utf-8-sig").splitlines():
            if line and not line.startswith("#"):
                key, word, _ = line.split("\t")
                known_keys.add((key, word)); known_words.add(word)
    benchmarks = sorted((ROOT / "tests/data").glob("typing_*.tsv"))
    answers = {line.split("\t")[1] for path in benchmarks for line in path.read_text(encoding="utf-8-sig").splitlines()
               if line and not line.startswith("#")}
    selected, seen, alternate, excluded_benchmark = [], set(), [], []
    authored = 0
    for category, frequency, text in SECTIONS:
        for word in text.split():
            authored += 1
            if word in seen or word in EXCLUDED or not re.fullmatch(r"[\u4e00-\u9fff]{2,6}", word):
                continue
            seen.add(word)
            if word in answers:
                excluded_benchmark.append(word); continue
            key = OVERRIDES.get(word) or "".join(lazy_pinyin(word, style=Style.NORMAL, strict=True))
            if not re.fullmatch("[a-z]+", key):
                raise ValueError(f"Invalid pinyin: {key} {word}")
            if (key, word) in known_keys:
                continue
            if word in known_words:
                alternate.append({"word": word, "proposed_pinyin": key}); continue
            selected.append((category, key, word, frequency))
    selected.sort(key=lambda r: (r[0], r[1], r[2]))
    assert len(selected) == len({(k, w) for _, k, w, _ in selected})
    for _, key, word, frequency in selected:
        positions = {0}
        for char in word:
            choices = pinyin(char, style=Style.NORMAL, heteronym=True, strict=True)[0]
            positions = {i+len(c) for i in positions for c in choices if key.startswith(c, i)}
        if len(key) not in positions:
            raise ValueError(f"Impossible character reading: {key} {word}")
        assert (key, word) not in known_keys and word not in known_words and word not in answers
    lines, previous = [], None
    for category, key, word, frequency in selected:
        if previous != category:
            lines.append(f"# category: {category}"); previous = category
        lines.append(f"{key}\t{word}\t{frequency}")
    output = OUT / "lexicon_batch_008.tsv"
    output.write_text("\n".join(lines)+"\n", encoding="utf-8", newline="\n")
    metadata = {
        "batch": "lexicon_batch_008", "authoring_source": "first_party_authored", "external_lexicon_import": False,
        "pronunciation_tool": "pypinyin 0.55.0 authoring-only; reviewed compound overrides",
        "authored_candidates": authored, "new_entries": len(selected), "unique_words": len({w for _, _, w, _ in selected}),
        "sha256": sha(output), "category_counts": dict(sorted(Counter(r[0] for r in selected).items())),
        "length_distribution": dict(sorted(Counter(str(len(r[2])) for r in selected).items())),
        "frequency_range": sorted({r[3] for r in selected}), "dedup_policy": "Exclude existing key/display pair and all existing display words",
        "dedup_baseline_sha256": {p.name: sha(p) for p in baselines}, "benchmark_sha256": {p.name: sha(p) for p in benchmarks},
        "benchmark_full_answers_added": 0, "benchmark_answers_excluded": excluded_benchmark,
        "alternate_pronunciation_candidates_excluded": alternate,
        "manual_pronunciation_overrides": {w: k for w, k in OVERRIDES.items() if w in {r[2] for r in selected}},
        "validation": {"all_outputs_two_to_six_han": True, "format_valid": True,
            "per_character_possible_readings_valid": True, "canonical_or_previous_batch_collisions": 0,
            "polyphone_reviewed_display_words": sum(any(c in w for c in POLYPHONES) for _, _, w, _ in selected)},
        "canonical_dictionaries_modified": False,
    }
    output.with_suffix(".json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2)+"\n", encoding="utf-8", newline="\n")
    print(json.dumps({k: metadata[k] for k in ("new_entries", "sha256", "category_counts", "length_distribution")}, ensure_ascii=False))

if __name__ == "__main__":
    main()
