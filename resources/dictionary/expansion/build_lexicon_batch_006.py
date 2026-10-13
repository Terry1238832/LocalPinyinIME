#!/usr/bin/env python3
"""First-party selected emotions, social language, travel, clothes and sports."""
from __future__ import annotations
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
from pypinyin import Style, lazy_pinyin, pinyin

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent

# Explicitly authored reusable terms. No scraped word list, benchmark answer,
# Cartesian combination, or candidate-generation model was used.
SECTIONS = [
    ("emotions_and_personal_state", 6000, """
    高兴 开心 快乐 愉快 愉悦 欣喜 喜悦 欢喜 欢欣 喜气 洋溢 雀跃
    兴奋 激动 振奋 兴致 兴头 热情 热忱 热切 热心 期待 盼望 向往
    渴望 憧憬 愿望 希冀 希望 满足 满意 知足 安心 放心 宽心 舒心
    舒服 舒适 舒畅 畅快 爽快 爽朗 痛快 轻松 轻快 轻盈 放松 自在
    自由 从容 沉着 镇定 平静 宁静 安静 安宁 坦然 淡然 安稳 稳妥
    安定 踏实 充实 丰富 充沛 饱满 旺盛 积极 乐观 开朗 豁达 豁朗
    乐呵 乐呵呵 乐滋滋 笑呵呵 笑嘻嘻 笑盈盈 笑眯眯 笑哈哈 喜滋滋
    喜洋洋 美滋滋 甜丝丝 暖洋洋 暖乎乎 暖融融 懒洋洋 软绵绵 轻飘飘
    兴冲冲 乐陶陶 兴高采烈 欢天喜地 喜出望外 眉开眼笑 眉飞色舞
    眉目舒展 心花怒放 欣喜若狂 欢欣鼓舞 神采飞扬 兴致勃勃 满心欢喜
    愁眉 苦脸 苦恼 烦恼 烦闷 烦躁 焦躁 急躁 浮躁 焦虑 担忧 担心
    忧虑 忧愁 忧伤 惆怅 惆然 哀愁 伤感 伤心 难过 难受 悲伤 悲痛
    悲凉 悲哀 哀伤 哀痛 哀怨 失落 失望 绝望 沮丧 颓丧 低落 消沉
    萎靡 委屈 憋屈 憋闷 憋气 懊恼 懊悔 后悔 悔恨 悔意 遗憾
    内疚 愧疚 自责 自怨 埋怨 抱怨 怨念 怨气 怨言 牢骚 郁闷 郁结
    闷闷不乐 忧心忡忡 愁眉苦脸 垂头丧气 心灰意冷 灰心丧气 黯然神伤
    愁容满面 百感交集 心事重重 心不在焉 心神不定 忐忑不安 坐立不安
    生气 气愤 愤怒 愤懑 怒火 怒气 火气 脾气 暴躁 恼怒 恼火 气恼
    气急 气闷 气馁 泄气 灰心 冷淡 冷漠 冷静 冷清 冷落 冷场 紧张
    慌张 慌乱 惊慌 心慌 发慌 恐慌 害怕 畏惧 恐惧 惧怕 胆怯 怯懦
    畏缩 顾虑 犹豫 迟疑 踌躇 踌躇不前 犹豫不决 小心翼翼 提心吊胆
    惊讶 诧异 惊异 惊喜 意外 惊愕 惊呆 吃惊 愕然 错愕 震惊 震撼
    震动 动容 感动 感慨 感叹 感触 感受 感激 感恩 感谢 感念 怀念
    想念 思念 惦念 惦记 挂念 挂心 牵挂 牵念 惦着 想家 恋家 恋旧
    依恋 眷恋 眷念 留恋 不舍 舍不得 依依不舍 念念不忘 流连忘返
    喜欢 喜爱 爱好 爱慕 欣赏 赞赏 赞叹 钦佩 敬佩 佩服 仰慕 敬仰
    认同 认可 信任 信赖 依赖 安慰 慰藉 鼓励 激励 支持 陪伴 关心
    体贴 贴心 温柔 温和 温暖 温情 善意 友善 亲切 亲近 亲密 亲热
    厌烦 厌倦 厌恶 讨厌 反感 抵触 排斥 嫌弃 嫌烦 嫌闷 无聊 寂寞
    孤单 孤独 落寞 冷寂 空虚 空落落 空荡荡 孤零零 失魂落魄 茫然失措
    疲倦 疲惫 疲乏 困乏 困倦 犯困 困意 睡意 睡眼惺忪 无精打采
    精神 精力 精力充沛 神清气爽 容光焕发 神采奕奕 心满意足 心平气和
    心绪 心境 心态 心情 情绪 情感 感情 情绪低落 情绪稳定 情绪波动
    情绪失控 情绪缓和 心情舒畅 心情愉快 心情低落 心里踏实 心里难受
    有点紧张 有点难过 有点担心 有点烦躁 有点疲惫 有点失落 有点开心
    稍感安心 稍感轻松 心中有数 心里有底 心里没底 心有余悸 心有不甘
    """),
    ("social_conversation_and_consideration", 6000, """
    聊天 闲聊 闲谈 聊聊 聊一聊 聊两句 说两句 谈一谈 谈谈 谈心 交心
    交流 交谈 对话 沟通 沟通交流 互动 联系 联络 寒暄 问候 打招呼
    道谢 致谢 感谢你 谢谢你 谢啦 多谢 辛苦了 麻烦了 麻烦你 拜托你
    劳驾 劳烦 有劳 有劳了 帮帮忙 多关照 多指教 请指教 请教 请教一下
    借过 让一让 借一步 请稍候 请稍等 不客气 别客气 客气了 客气话
    不用谢 不必谢 别见外 不见外 没关系 不要紧 没事儿 没事 没问题
    没啥问题 没啥关系 不碍事 小意思 举手之劳 顺手帮忙 乐意帮忙
    方便吗 可以吗 能行吗 行不行 好不好 好吗 好的 好呀 好啊 好吧 好嘞
    行啊 行吧 可以的 可以啊 没问题的 没关系的 当然可以 当然好 可以试试
    随便聊聊 随口说说 随便问问 问问而已 随意一点 自然一点 放松一点
    认真一点 仔细一点 小心一点 留心一点 耐心一点 慢慢来 别着急 别急
    不着急 不用急 不必急 不急不躁 慢一点 小声一点 轻一点 快一点
    声音小点 声音大点 稍微等等 稍微一下 别担心 放心吧 放心好了
    别紧张 别难过 别伤心 别生气 别介意 别误会 别想太多 想开一点
    别太累了 多休息 保重 保重身体 注意身体 注意安全 注意休息 注意保暖
    一路平安 一路顺风 旅途愉快 出行平安 工作顺利 学业顺利 万事顺意
    好好休息 好好吃饭 好好睡觉 好好学习 好好工作 慢慢聊 慢慢说 慢慢讲
    听你说 你说呢 你觉得呢 你看呢 我觉得 我想想 我看看 我听听 我问问
    想清楚 看清楚 说清楚 讲清楚 弄清楚 问清楚 别弄错 别记错 别看错
    说实话 老实说 坦白说 实话实说 实不相瞒 坦诚相待 开诚布公
    换句话说 总的来说 总体来看 大体来说 具体来说 严格来说 通常来说
    比如说 举个例子 打个比方 话说回来 顺便说下 顺便说说 顺便问下
    顺便问问 说来也巧 说来话长 说起来 说得也是 说得对 有道理 挺有道理
    确实如此 正是如此 也有道理 不无道理 这个问题 这件事 那件事 这回事
    一回事 两回事 怎么回事 怎么说呢 怎么办呢 怎么弄呢 怎么走呢 怎么看呢
    怎么样呢 怎样都行 怎么都行 都可以 都行 都好 都成 不一定 未必
    未必如此 不尽如此 看情况 看实际 看需求 看安排 看时间 看心情 看天气
    因人而异 因地制宜 视情况 具体情况 随机应变 灵活处理 慎重考虑
    考虑考虑 考虑一下 想想办法 想个办法 商量商量 再商量 再考虑 再看看
    谢谢提醒 感谢提醒 感谢理解 感谢支持 感谢关心 多谢关心 多谢提醒
    谢谢关心 谢谢理解 谢谢支持 抱歉 不好意思 对不起 对不住 抱歉了
    实在抱歉 深感抱歉 请谅解 请理解 多包涵 请包涵 非常抱歉 深表歉意
    不小心 无意中 不经意 一不留神 一时疏忽 忘了 忘记了 记错了 看错了
    听错了 想错了 理解错了 弄错了 弄反了 搞错了 误会了 误解了
    难免 出乎意料 情有可原 可以理解 理所当然 理应如此 本该如此
    一言为定 说好了 说定了 说准了 确定了 别忘了 记住了 记下了
    好久不见 最近如何 最近怎样 最近忙吗 近况 最近还好 还好吗 一切顺利
    再见 回见 下回见 改日见 回头见 明天见 后天见 周末见 晚安 早安 午安
    拜拜 拜了 拜个早年 新年快乐 生日快乐 节日快乐 周末愉快 假期愉快
    祝福 祝愿 恭喜 恭贺 祝贺 恭喜你 祝贺你 好消息 喜讯 喜事 佳音
    邀请 邀约 邀约见面 邀请参加 诚挚邀请 欢迎参加 欢迎光临 欢迎回来
    """),
    ("everyday_description_and_observations", 6000, """
    干净 整洁 清洁 洁净 清爽 清新 清澈 清亮 明亮 明净 透亮 透明
    崭新 新颖 新鲜 新奇 独特 别致 精致 精细 细致 细腻 雅致 雅观
    好看 漂亮 美观 美丽 秀丽 娟秀 清秀 灵秀 优美 秀美 绚丽 绚烂
    鲜艳 明艳 艳丽 光鲜 华丽 华贵 贵气 大方 得体 体面 合体 合身
    朴素 简朴 素净 素雅 淡雅 优雅 文雅 典雅 雅静 雅洁 简洁 简约
    简练 精练 利落 干练 干脆 爽利 清晰 明晰 分明 清楚 明白 一目了然
    杂乱 凌乱 散乱 零乱 脏乱 混乱 混杂 嘈杂 吵闹 喧闹 喧哗 喧嚣
    拥挤 拥堵 拥塞 局促 狭窄 窄小 狭小 逼仄 宽敞 开阔 宽阔 广阔
    宽广 广袤 辽阔 空旷 空阔 空寂 寂静 静谧 幽静 幽深 深邃 遥远
    遥遥 悠远 辽远 偏远 偏僻 偏静 偏小 偏大 偏长 偏短 偏厚 偏薄
    厚薄 长短 宽窄 大小 高低 深浅 远近 粗细 强弱 冷暖 软硬 轻重
    软和 柔软 柔和 柔顺 顺滑 平滑 光滑 圆滑 细滑 硬朗 坚硬 僵硬
    坚实 厚实 扎实 结实 稳固 牢固 牢靠 坚固 耐用 耐久 耐磨 耐刮
    结实耐用 坚固耐用 经久耐用 轻便 便携 便捷 方便 实用 好用 顺手
    趁手 合适 适合 适用 适宜 适中 合理 合乎 合算 划算 划得来 值得
    物美价廉 货真价实 物有所值 性价比 实惠 便宜 低廉 昂贵 贵重 价高
    价格适中 价格合理 价格便宜 价格偏高 价格较低 大致相同 大体相同
    基本相同 基本一致 差不多 相差不大 相差较大 略有差异 有些区别
    变化不大 变化明显 有所变化 明显改善 稍有改善 逐渐改善 稳步改善
    稍好一些 稍差一些 略好一点 稍微好点 稍微差点 好一点 差一点
    更合适 更方便 更舒服 更清楚 更明显 更简单 更容易 更实用 更顺手
    冷冰冰 热乎乎 热腾腾 暖烘烘 热辣辣 冰凉凉 凉丝丝 凉飕飕 冰冷冷
    亮闪闪 亮晶晶 金灿灿 白花花 黑乎乎 黑黢黢 红彤彤 绿油油 灰蒙蒙
    雾蒙蒙 灰沉沉 阴沉沉 阴森森 轻悠悠 颤巍巍 慢悠悠 慢吞吞 急匆匆
    松垮垮 皱巴巴 干巴巴 干燥燥 湿漉漉 湿乎乎 黏糊糊 黏乎乎 油腻腻
    滑溜溜 滑腻腻 粗糙糙 毛茸茸 毛绒绒 软乎乎 硬邦邦 胖乎乎 瘦巴巴
    一点点 一些些 一小点 一大点 一大块 一小块 一大片 一小片 一小撮
    一大堆 一小堆 一整块 一整片 一整天 一整夜 一整周 一整月 一整年
    每一天 每一晚 每一周 每一月 每一年 日复一日 年复一年 反反复复
    三番五次 隔三差五 时不时 偶尔 一向 一贯 向来 通常 往往 常常
    常有 经常 时常 不常 频繁 密集 稀疏 稀少 稀罕 少见 常见 普通 平常
    平凡 寻常 罕见 难得 可贵 珍贵 宝贵 来之不易 久违 久盼 好久 好一会
    很久 许久 久久 暂时 临时 短暂 持久 长久 长远 长期 短期 中期
    持续 连续 连贯 接续 断续 断断续续 接连 连着 接着 随后 继而 然后
    最初 起初 初始 初步 刚刚 原先 原来 本来 后来 以后 往后 此后
    当初 当时 当前 如今 现在 眼下 此时 此刻 此间 近来 近期 最近
    前些天 这些天 后几天 这阵子 那阵子 前阵子 前几日 后几日 这几日
    """),
    ("personality_behavior_and_relationships", 6000, """
    性格 个性 品性 品格 气质 气度 风度 风范 仪态 仪表 神态 姿态
    开朗 活泼 活跃 外向 内向 腼腆 羞涩 害羞 娴静 安静 文静 沉静
    稳重 老成 成熟 稚嫩 幼稚 单纯 天真 纯真 纯粹 直率 爽直 率直
    坦率 坦诚 诚实 老实 忠厚 厚道 善良 善解人意 体谅 谅解 理解 包容
    宽容 宽厚 宽仁 宽宏 温厚 温良 温文 温文尔雅 彬彬有礼 斯文 文质彬彬
    热心肠 好心肠 心地善良 乐于助人 助人为乐 富有爱心 待人友善 待人真诚
    善于沟通 善于倾听 善于表达 善于观察 善于思考 思虑周全 考虑周到
    心思细腻 心思敏感 心思单纯 心胸开阔 心胸宽广 心胸狭窄 通情达理
    周到 周全 细心 用心 耐心 专心 贴心 关怀 关爱 关照 照顾 照料
    认真 专注 集中 聚焦 投入 敬业 尽责 尽心 尽力 尽职 负责 有责任心
    责任感 责任心 进取心 事业心 上进心 好胜心 好奇心 好奇 好学 勤学
    勤奋 勤勉 勤恳 勤快 勤劳 刻苦 努力 踏实 扎实 务实 实干 实在
    精明 聪明 聪慧 灵敏 灵巧 灵活 灵动 机灵 机敏 机智 睿智 智慧
    理智 理性 感性 感情用事 冲动 莽撞 冒失 鲁莽 草率 仓促 急性子
    慢性子 好脾气 坏脾气 急脾气 火爆脾气 暴脾气 犟脾气 直性子
    倔强 执拗 固执 顽固 stubborn 犟劲 倔脾气 持之以恒 坚持不懈 坚韧不拔
    意志坚定 意志坚强 百折不挠 锲而不舍 不屈不挠 自强不息 坚毅 坚韧
    果断 果敢 干脆 优柔 优柔寡断 拖拉 拖沓 拖延 拖延症 犹疑 迟缓
    散漫 懒散 懒惰 懈怠 怠慢 马虎 粗心 粗枝大叶 丢三落四 心粗 胆大
    大胆 勇敢 勇气 胆量 胆识 自信 自立 自主 自律 自制 自省 自觉
    自尊 自爱 自重 自强 自负 自傲 自卑 谦虚 谦逊 谦和 谦恭 谦让
    谦卑 骄傲 傲慢 狂妄 自大 狂傲 目中无人 盛气凌人 颐指气使
    大度 小气 吝啬 计较 较真 挑剔 苛刻 苛求 宽松 严格 严厉 严肃
    严谨 谨慎 慎重 小心 拘谨 拘束 随和 随性 随意 洒脱 潇洒 豪爽
    幽默 风趣 诙谐 有趣 有意思 有趣味 爱开玩笑 开玩笑 笑话 玩笑 打趣
    逗乐 逗趣 逗笑 逗人 玩笑话 调侃 打趣人 互相理解 互相尊重 互相支持
    互相关心 彼此理解 彼此尊重 彼此信任 彼此关心 换位思考 体谅他人
    和睦 相处融洽 融洽 亲近 疏远 生疏 熟悉 熟络 熟稔 亲疏 疏离 隔阂
    误会 误解 分歧 争执 争吵 吵架 闹别扭 拌嘴 冷战 和好 和解 和睦相处
    说和 劝和 调解 劝解 劝慰 安抚 安慰 释怀 看开 放下 放宽心 放平心态
    尊重 理解 信赖 坦诚交流 互相鼓励 共同进步 共同成长 共同努力
    """),
    ("clothes_fabrics_and_outfits", 5500, """
    衣服 衣物 衣装 衣着 服装 着装 穿着 穿搭 搭配 衣柜 衣橱 衣架
    上衣 外衣 外套 内搭 内衬 打底 打底衫 打底衣 打底裤 罩衫 罩衣
    短袖 长袖 无袖 七分袖 九分袖 五分袖 半袖 泡泡袖 灯笼袖 喇叭袖
    插肩袖 落肩袖 连肩袖 袖口 袖边 袖子 袖长 袖围 衣长 胸围 腰围
    肩宽 肩线 肩部 臂围 臀围 裤长 裤腰 裤围 裤脚 裤腿 裙长 裙摆
    领子 衣领 领口 领边 翻领 立领 圆领 方领 尖领 尖角领 小翻领
    娃娃领 水手领 高领 低领 半高领 连帽 兜帽 帽檐 帽围 帽带
    口袋 衣袋 裤袋 侧袋 内袋 贴袋 暗袋 胸袋 插袋 拉链袋
    衬衫 衬衣 白衬衫 格子衫 格纹衫 条纹衫 牛仔衫 牛仔衣 牛仔外套
    棉衬衫 真丝衫 雪纺衫 蕾丝衫 针织衫 毛衣 羊毛衫 羊绒衫 开衫
    针织开衫 套头衫 套头毛衣 薄毛衣 厚毛衣 毛衣外套 马甲 毛衣马甲
    针织马甲 羽绒马甲 羽绒背心 西装马甲 短马甲 长马甲 背心 吊带 吊带衫
    吊带背心 吊带裙 背带裤 背带裙 连衣裙 半身裙 短裙 长裙 中长裙
    及膝裙 过膝裙 百褶裙 褶皱裙 包臀裙 铅笔裙 鱼尾裙 伞裙 蛋糕裙
    牛仔裙 针织裙 真丝裙 雪纺裙 连身裙 衬衫裙 运动裙 印花裙 碎花裙
    小黑裙 小白裙 旗袍 汉服 唐装 中山装 西装 礼服 晚礼服 小礼服
    正装 休闲装 休闲西装 套装 两件套 三件套 裙套装 裤套装 工作服
    工装 工装服 制服 校服 围裙 护袖 袖套 防晒衣 防晒服 防晒衫
    防晒外套 皮肤衣 风衣 大衣 毛呢大衣 羊毛大衣 羊绒大衣 呢子大衣
    呢大衣 短大衣 长大衣 中长大衣 短外套 长外套 薄外套 厚外套
    夹克 夹克衫 皮夹克 皮衣 机车夹克 棒球服 棒球夹克 冲锋衣 冲锋裤
    软壳衣 硬壳衣 软壳裤 抓绒衣 抓绒裤 羽绒服 棉服 棉衣 棉袄
    短羽绒服 长羽绒服 轻薄羽绒 羽绒内胆 充绒量 含绒量 羽绒填充
    卫衣 连帽卫衣 套头卫衣 拉链卫衣 卫衣外套 运动服 运动套装 瑜伽服
    瑜伽裤 运动裤 训练服 训练裤 跑步服 跑步裤 骑行服 骑行裤 骑行背带
    短裤 长裤 休闲裤 西裤 西装裤 牛仔裤 直筒裤 小脚裤 阔腿裤 锥形裤
    喇叭裤 微喇裤 破洞裤 工装裤 束脚裤 束口裤 九分裤 七分裤 五分裤
    裙裤 百慕大裤 铅笔裤 烟管裤 哈伦裤 灯笼裤 短裙裤 棉裤 秋裤
    保暖裤 保暖衣 保暖内衣 秋衣 秋衣秋裤 内衣 内裤 文胸 胸罩 运动内衣
    无钢圈 有钢圈 胸垫 乳垫 肩带 内衣肩带 调整肩带 无痕内衣 无痕内裤
    三角裤 平角裤 四角裤 高腰内裤 中腰内裤 低腰内裤 生理裤 安全裤
    睡衣 睡裤 睡裙 睡袍 家居服 家居裤 浴袍 晨袍 真丝睡衣 棉质睡衣
    绒布睡衣 法兰绒服 珊瑚绒服 亲子装 情侣装 童装 婴儿服 儿童服
    纯棉 棉质 棉布 棉纱 精梳棉 长绒棉 有机棉 全棉 麻料 亚麻 苎麻
    棉麻 棉麻布 真丝 桑蚕丝 柞蚕丝 丝绸 丝缎 缎面 缎布 雪纺 蕾丝
    羊毛 羊绒 毛呢 呢料 呢绒 绒布 天鹅绒 灯芯绒 珊瑚绒 法兰绒 摇粒绒
    抓绒 毛绒 羽绒 鸭绒 鹅绒 白鸭绒 白鹅绒 灰鸭绒 填充棉 聚酯纤维
    涤纶 锦纶 尼龙 氨纶 粘纤 粘胶纤维 莫代尔 莱赛尔 天丝 醋酸纤维
    混纺 交织 针织 梭织 斜纹 平纹 罗纹 提花 印花 刺绣 绣花 织花
    扎染 染色 固色 水洗 石洗 做旧 磨毛 磨绒 起绒 加绒 加厚 加绒加厚
    防风 防水 防泼水 透气 吸汗 排汗 吸湿 吸湿排汗 抗静电 防静电
    不起球 不掉色 不缩水 易打理 好打理 免熨烫 免烫 抗皱 弹力 高弹
    微弹 无弹 版型 修身 合身 宽松 宽松版 修身版 直筒版 短款 长款
    中长款 短版 长版 复古款 经典款 基础款 通勤款 休闲款 运动款
    """),
    ("shoes_bags_and_accessories", 5500, """
    鞋子 鞋 鞋类 鞋款 鞋型 鞋码 鞋号 鞋长 鞋宽 鞋楦 鞋头 鞋帮
    鞋面 鞋底 鞋垫 鞋跟 鞋带 鞋舌 鞋眼 鞋扣 鞋盒 鞋撑 鞋刷
    皮鞋 运动鞋 休闲鞋 板鞋 帆布鞋 网球鞋 篮球鞋 足球鞋 跑步鞋
    慢跑鞋 训练鞋 健步鞋 徒步鞋 登山鞋 越野鞋 越野跑鞋 攀岩鞋 骑行鞋
    高帮鞋 低帮鞋 中帮鞋 平底鞋 高跟鞋 中跟鞋 低跟鞋 粗跟鞋 细跟鞋
    尖头鞋 圆头鞋 方头鞋 乐福鞋 牛津鞋 德比鞋 玛丽珍鞋 芭蕾鞋 豆豆鞋
    小白鞋 懒人鞋 一脚蹬 便鞋 布鞋 绣花鞋 老北京鞋 穆勒鞋 凉鞋
    凉拖 拖鞋 人字拖 洞洞鞋 家居拖 棉拖鞋 浴室拖 沙滩鞋 涉水鞋
    短靴 长靴 中筒靴 高筒靴 过膝靴 踝靴 马丁靴 切尔西靴 雪地靴
    雨靴 雨鞋 胶鞋 胶靴 厚底鞋 松糕鞋 绑带鞋 系带鞋 搭扣鞋 魔术贴鞋
    防滑底 橡胶底 牛筋底 真皮底 软底 硬底 薄底 厚底 内增高 外增高
    袜子 棉袜 丝袜 短袜 长袜 船袜 隐形袜 中筒袜 长筒袜 过膝袜
    连裤袜 打底袜 厚袜 薄袜 毛袜 羊毛袜 运动袜 跑步袜 五指袜 童袜
    袜口 袜筒 袜底 袜跟 袜尖 袜套 保暖袜 防滑袜 压缩袜 压力袜
    包 包包 手袋 手提包 单肩包 双肩包 双肩背包 斜挎包 斜背包 腰包
    胸包 腋下包 水桶包 托特包 邮差包 公文包 文件包 电脑包 平板包
    女包 男包 童包 妈咪包 旅行包 旅行背包 行李包 运动包 健身包
    帆布包 尼龙包 皮包 真皮包 小方包 小圆包 手拿包 晚宴包 链条包
    链条带 肩带 背带 手提带 提手 包带 包扣 包底 包内衬 包隔层
    包内袋 拉链包 搭扣包 翻盖包 抽绳包 束口袋 收纳袋 帆布袋 网袋
    防尘袋 鞋袋 鞋包 洗漱包 化妆包 首饰包 护照包 证件包 卡包 钱包
    零钱包 钥匙包 小挎包 相机包 摄影包 户外背包 登山包 登山背包
    行李箱 拉杆箱 登机箱 托运行李 旅行箱 箱包 箱轮 万向轮 拉杆 箱锁
    密码锁 行李牌 行李绑带 箱套 防尘套 行李套 硬壳箱 软箱 布箱
    围巾 围脖 丝巾 方巾 小方巾 披肩 羊毛围巾 羊绒围巾 真丝围巾
    领巾 领带 领结 领带夹 袖扣 胸针 胸花 徽章 肩章 发饰 发夹
    发卡 发箍 发圈 发带 头绳 皮筋 头箍 头巾 头饰 蝴蝶结 抓夹
    一字夹 盘发夹 发簪 发梳 边夹 顶夹 马尾夹 香蕉夹 鲨鱼夹
    帽子 棒球帽 鸭舌帽 渔夫帽 遮阳帽 太阳帽 草帽 草编帽 礼帽
    贝雷帽 针织帽 毛线帽 毛绒帽 雷锋帽 空顶帽 宽檐帽 桶帽 盆帽
    眼镜 墨镜 太阳镜 偏光镜 近视镜 老花镜 护目镜 风镜 泳镜 眼镜框
    镜框 镜片 镜腿 鼻托 眼镜布 眼镜盒 眼镜袋 镜片清洗 眼镜清洗
    手套 棉手套 羊毛手套 皮手套 防晒手套 露指手套 半指手套 全指手套
    腰带 皮带 腰封 腰链 腰绳 腰扣 皮带扣 针扣 自动扣 板扣
    首饰 饰品 手镯 手链 项链 耳环 耳坠 耳钉 耳夹 戒指 指环 吊坠
    挂坠 吊饰 珠链 珠串 串珠 银饰 金饰 玉饰 玉镯 木镯 银镯 金镯
    配饰 装饰点 点缀 搭配色 撞色 同色系 顺色 叠穿 层次感 松弛感
    """),
    ("travel_transport_lodging_and_visits", 5500, """
    旅行 旅游 出游 游玩 游览 游览区 参观 观光 观景 赏景 访古 探访
    出行 旅程 行程 旅途 旅客 游客 旅友 同游 自驾 自驾游 跟团 跟团游
    自由行 半自由行 团队游 团体游 拼团 小团 私人团 亲子游 家庭游
    周边游 城市游 短途游 长途游 一日游 两日游 三日游 深度游 休闲游
    度假 度假村 度假区 度假酒店 海滨度假 海岛游 海边游 山水游 乡村游
    古镇游 古城游 文化游 历史游 美食游 研学游 游学 城市漫步 旅拍
    旅游攻略 出行攻略 旅行攻略 游玩攻略 景区攻略 自驾攻略 景点路线
    旅游路线 旅游线路 游览路线 参观路线 游玩路线 旅游计划 出游计划
    行程单 行程表 路书 导览图 导游图 景点地图 旅游地图 景区地图
    景点 景区 名胜 名胜古迹 旅游景点 热门景点 小众景点 免费景点
    景点门票 景区门票 联票 套票 年卡票 景点预约 景区预约 门票预约
    免票 优惠票 早鸟票 预售票 门票价格 票务预订 门票预订 门票核验
    旺季 淡季 平季 旺季价格 淡季价格 错峰 错峰出行 错峰旅游 高峰出游
    景区入口 景区出口 游客中心 游客服务 导览服务 人工讲解 语音讲解
    电子导览 导览器 讲解器 导游 讲解员 领队 团长 地接 地接导游
    集合上车 跟团集合 游客集合 自行前往 自行返回 自行游览 自行参观
    景区停车 景区接驳 景区车 观光车 观光船 观光巴士 环线车 缆车 索道
    观景台 观景亭 观景平台 观光塔 观光电梯 观景栈道 玻璃栈道 木栈道
    游步道 步道 山路 山道 石阶 台阶 栈桥 石桥 吊桥 拱桥 景观桥
    湖畔 河畔 江畔 海滨 海湾 海岸 沙滩 海滩 滩涂 浅滩 湾区 半岛
    岛屿 小岛 海岛 湖心岛 瀑布 山泉 溪流 山谷 峡谷 山峰 山顶 山脚
    山腰 山脊 山麓 山坡 盘山路 山间小路 古道 林荫道 绿道 滨海步道
    酒店 旅馆 旅店 宾馆 客栈 民宿 青旅 青年旅舍 家庭旅馆 公寓酒店
    精品酒店 商务酒店 快捷酒店 经济酒店 连锁酒店 星级酒店 机场酒店
    酒店预订 客房预订 房间预订 民宿预订 订房 订酒店 订民宿 住宿 住店
    入住 入住登记 入住手续 入住时间 入住日期 入住人数 入住信息 住客信息
    退房 退房时间 退房日期 延迟退房 提前退房 续住 续房 延迟入住
    提前入住 深夜入住 夜间入住 客房 房间 房型 单人间 双人间 双床房
    大床房 家庭房 亲子房 三人间 四人间 标准间 标间 套房 套间
    行政房 景观房 海景房 山景房 湖景房 园景房 江景房 河景房 阳台房
    无窗房 有窗房 内窗房 外窗房 高楼层 低楼层 临街房 安静房 无烟房
    非吸烟房 吸烟房 连通房 无障碍房 加床 婴儿床 婴儿椅 额外加床
    房卡 门卡 房号 客房号 楼层号 前台 前台接待 酒店前台 叫醒服务
    行李寄存 行李保管 寄存行李 行李存放 寄存柜 寄存牌 行李车 行李员
    客房服务 清扫客房 房间清扫 换床单 换毛巾 加毛巾 加被子 加枕头
    酒店早餐 含早餐 不含早餐 自助早餐 早餐券 早餐时段 早餐时间
    洗漱用品 一次用品 牙具 拖鞋 浴帽 肥皂 洗发水 沐浴露 浴巾 面巾
    电热水壶 热水壶 烧水壶 吹风机 小冰箱 迷你吧 保险箱 房间保险
    客房押金 住宿押金 押金退还 发票开具 住宿发票 酒店发票
    """),
    ("outdoors_camping_and_hiking", 5500, """
    户外 户外活动 户外运动 野外 野外活动 野外露营 野营 露营 扎营 扎帐
    露营地 营地 营位 营区 露营区 帐篷 帐篷搭建 搭帐篷 收帐篷 露营帐篷
    双人帐 三人帐 家庭帐 轻量帐 充气帐 隧道帐 圆顶帐 金字塔帐 天幕
    遮阳天幕 天幕杆 撑杆 帐杆 帐篷杆 地钉 营钉 风绳 营绳 地布
    地垫 防潮垫 泡沫垫 充气垫 自充气垫 睡垫 睡袋 睡袋内胆 羽绒睡袋
    棉睡袋 隔脏袋 枕头 充气枕 旅行枕 颈枕 露营枕 营灯 露营灯 头灯
    手电 强光手电 便携灯 提灯 挂灯 帐篷灯 营地灯 灯杆 灯架
    露营桌 露营椅 折叠桌 折叠椅 月亮椅 克米特椅 行军床 折叠床
    野餐 野餐布 野餐垫 野餐篮 便当盒 餐盒 露营餐具 户外餐具 户外炉具
    卡式炉 卡式罐 燃气罐 气罐 炉头 分体炉 一体炉 酒精炉 固体燃料
    锅具 套锅 户外套锅 煎盘 烤盘 烤网 烤架 户外烤炉 炭炉 烧烤炉
    烧烤炭 木炭 无烟炭 引火炭 引火块 点火器 打火机 防风火机 点火棒
    食物保温 保温箱 保温袋 保冷袋 冷藏箱 保冷箱 冰袋 冰盒 冰板 水桶
    折叠水桶 水袋 水壶 水瓶 保温杯 户外水壶 随身水壶 饮水袋 净水器
    净水片 净水滤芯 应急净水 食品储存 防熊罐 防虫罩 防蚊网 蚊帐
    防蚊喷雾 驱蚊液 驱蚊贴 蚊香盘 驱蚊器 防虫喷雾 防晒霜 防晒帽
    防晒袖套 防晒口罩 防晒披肩 雨衣 雨披 雨罩 背包雨罩 防雨罩
    防水袋 防水包 防水收纳 急救包 医药包 救生哨 求生哨 急救毯 保温毯
    救生衣 救生圈 救生绳 防护绳 救援绳 救援包 应急灯 应急电源
    野外安全 野外求生 户外安全 应急准备 应急药品 常备药品 防暑用品
    徒步 徒步旅行 徒步路线 徒步线路 徒步路径 徒步起点 徒步终点 徒步时长
    轻装徒步 重装徒步 徒步领队 户外领队 向导 山野向导 徒步队 徒步团
    登山 登山路线 登山步道 登山计划 登山杖 登山扣 登山绳 登山头盔
    护膝 护踝 护腕 护肘 护腰 绑腿 雪套 防沙套 登山鞋带 装备清单
    户外装备 徒步装备 露营装备 装备重量 基础重量 负重 总负重 轻量化
    海拔 海拔高度 累计爬升 爬升 爬升高度 总爬升 下降高度 高差 坡度
    陡坡 缓坡 上坡 下坡 碎石路 泥土路 土路 石板路 林间路 山间路
    山间小道 林间小道 林间步道 岔路 路标 标识带 路线标识 轨迹 轨迹点
    离线路线 离线轨迹 导航轨迹 轨迹导入 路线导入 原路返回 迷路 走失
    走错路 偏离路线 探路 踏勘 踩点 试走 路线难度 徒步难度 难度等级
    涉水 过溪 渡河 涉溪 湿滑 泥泞 易滑 滑倒 绊倒 跌倒 落石 塌方
    山洪 泥石流 雷雨 雷暴 强风 大风 山雾 浓雾 能见度 天气预警
    日出 日落 看日出 看日落 观星 星空 星轨 星河 银河 满天繁星
    赏花 赏雪 赏枫 赏秋 观鸟 观蝶 观虫 观潮 赶海 拾贝 听雨 听风
    不留痕迹 无痕露营 带走垃圾 文明露营 保护环境 保护植被 勿采花草
    """),
    ("sports_fitness_and_training", 5500, """
    运动 体育 健身 锻炼 体能 体力 耐力 力量 速度 敏捷 柔韧 灵活
    身体素质 体能训练 力量训练 耐力训练 速度训练 柔韧训练 核心训练
    功能训练 康复训练 稳定训练 平衡训练 爆发力 肌耐力 灵敏性 协调性
    热身 热身运动 热身动作 动态热身 静态热身 充分热身 运动前后
    拉伸 拉伸运动 拉伸动作 动态拉伸 静态拉伸 肌肉拉伸 全身拉伸 腿部拉伸
    放松 放松运动 放松肌肉 放松训练 泡沫轴 筋膜球 筋膜枪 按摩球
    训练 训练计划 训练目标 训练强度 训练时长 训练次数 训练频率 训练安排
    训练记录 训练日志 运动记录 运动计划 锻炼计划 运动强度 运动时长
    训练组数 训练重量 训练量 每组次数 组间休息 休息间隔 训练间隔
    动作 动作要领 动作规范 动作标准 动作幅度 活动范围 动作节奏 发力
    发力方式 发力部位 呼吸节奏 呼吸方式 动作呼吸 运动呼吸 核心收紧
    收紧腹部 腹部收紧 腹式呼吸 控制呼吸 调整呼吸 匀速呼吸 节奏稳定
    跑步 慢跑 快跑 长跑 短跑 中长跑 越野跑 路跑 夜跑 晨跑 竞走
    走路 快走 健走 散步 漫步 爬楼 爬楼梯 走楼梯 原地踏步 原地跑
    跑步路线 跑步计划 跑步配速 配速 平均配速 目标配速 实际配速
    步频 步幅 步长 跑姿 跑步姿势 跑步节奏 跑步距离 跑步时长
    有氧 无氧 有氧运动 无氧运动 有氧训练 无氧训练 间歇训练 高强间歇
    高强度 中强度 低强度 低冲击 高冲击 稳态有氧 长距离 短距离
    心率 静息心率 最大心率 目标心率 平均心率 心率区间 燃脂区间
    心肺 心肺功能 心肺耐力 最大摄氧 摄氧量 跑步机 椭圆机 划船机 单车
    动感单车 健身单车 风阻单车 健身器械 综合器械 自由重量 固定器械
    哑铃 杠铃 壶铃 弹力带 阻力带 弹力绳 拉力绳 拉力器 瑜伽球 瑜伽垫
    健身垫 平衡垫 平衡板 平衡球 跳箱 跳绳 跳绳垫 战绳 药球 健身球
    深蹲 浅蹲 负重深蹲 杠铃深蹲 哑铃深蹲 高脚杯蹲 前蹲 后蹲 分腿蹲
    保加利亚 蹲起 弓步 弓步蹲 箭步蹲 行走弓步 侧弓步 徒手深蹲
    硬拉 罗马尼亚 直腿硬拉 相扑硬拉 哑铃硬拉 壶铃硬拉 臀桥 单腿臀桥
    杠铃臀桥 臀推 弹力臀桥 蚌式开合 髋外展 髋内收 髋伸展 髋屈曲
    俯卧撑 跪姿俯卧 标准俯卧 上斜俯卧 下斜俯卧 引体向上 反手引体
    高位下拉 坐姿划船 杠铃划船 哑铃划船 单臂划船 俯身划船 平板支撑
    侧板支撑 支撑旋转 肩部推举 哑铃推举 杠铃推举 过顶推举 卧推
    平板卧推 上斜卧推 下斜卧推 哑铃卧推 杠铃卧推 飞鸟 哑铃飞鸟
    夹胸 绳索夹胸 侧平举 前平举 反向飞鸟 面拉 弯举 哑铃弯举 杠铃弯举
    锤式弯举 牧师凳 二头弯举 三头伸展 三头下压 绳索下压 提踵
    站姿提踵 坐姿提踵 卷腹 反向卷腹 仰卧起坐 举腿 仰卧举腿 悬垂举腿
    登山跑 波比跳 开合跳 高抬腿 踢臀跑 跨步跳 纵跳 摸高 跳远
    立定跳远 助跑跳远 立定跳高 助跑跳高 折返跑 冲刺跑 加速跑 变速跑
    瑜伽 普拉提 太极 太极拳 八段锦 五禽戏 体操 健美操 健身操 广场舞
    街舞 爵士舞 拉丁舞 民族舞 现代舞 芭蕾舞 芭蕾形体 形体训练 体态训练
    肩颈 肩背 腰背 下背 上背 胸肌 背肌 腹肌 臀肌 腿肌 小腿肌
    肌肉 肌群 大肌群 小肌群 胸大肌 背阔肌 斜方肌 三角肌 肱二头肌
    肱三头肌 股四头肌 腘绳肌 臀大肌 臀中肌 腹直肌 腹斜肌 腹横肌
    """),
    ("ball_sports_swimming_and_recreation", 5500, """
    篮球 足球 排球 网球 羽毛球 乒乓球 棒球 垒球 手球 橄榄球 曲棍球
    水球 保龄球 台球 桌球 壁球 毽球 门球 高尔夫 网球拍 羽毛球拍 球拍
    球网 球框 篮框 球门 门框 球鞋 球衣 球裤 球袜 球场 球馆 球台
    篮球场 足球场 网球场 排球场 棒球场 羽毛球场 乒乓球台 台球桌
    发球 接发球 接球 传球 接传球 运球 带球 控球 抢球 截球 断球
    投球 投篮 投篮训练 投篮姿势 投篮手型 投篮动作 投篮命中 命中率
    上篮 扣篮 跳投 定点投篮 罚球 罚篮 三分球 两分球 中投 远投
    篮板 篮板球 前场篮板 后场篮板 防守篮板 进攻篮板 卡位 抢篮板
    助攻 抢断 盖帽 封盖 挡拆 挡人 掩护 空切 切入 突破 转身 跳步
    急停 急停跳投 假动作 变向 变向运球 交叉步 后撤步 背身 背打
    防守 进攻 快攻 反击 阵地进攻 阵地防守 人盯人 联防 协防 补防
    盯防 贴身防守 包夹 防守站位 进攻站位 内线 外线 底线 边线 中线
    罚球线 三分线 中圈 禁区 底角 弧顶 侧翼 前场 后场 半场 全场
    跑位 无球跑动 接应 策应 回防 退防 丢球 失误 走步 二运 翻腕
    犯规 违例 进攻犯规 防守犯规 打手 推人 拉人 阻挡 带球撞人
    技术犯规 违体犯规 篮球训练 球队训练 团队配合 个人训练 赛前训练
    射门 射门训练 传中 长传 短传 直塞 横传 回传 斜传 头球 铲球
    停球 颠球 脚内侧 脚外侧 脚背 抽射 推射 点球 任意球 角球 边线球
    越位 越位线 越位陷阱 反越位 乌龙球 单刀球 单刀 主罚 开球
    守门员 门将 后卫 中卫 边卫 前锋 中锋 边锋 中场 前腰 后腰
    进球 失球 控球率 射门次数 射正 射偏 传球成功 红牌 黄牌 补时
    正手 反手 正手击球 反手击球 正手攻球 反手攻球 击球 击球点 落点
    拉球 搓球 推挡 削球 抽球 扣杀 挑球 吊球 高远球 平抽球 杀球
    网前球 后场球 小球 放小球 搓小球 网前扑球 接杀球 发高远球
    发短球 发长球 发平高球 接发抢攻 正反手 握拍 握拍姿势 握拍方式
    直拍 横拍 拍柄 拍面 拍线 球线 穿线 磅数 线床 拍框 护线管
    上旋 下旋 侧旋 侧上旋 侧下旋 不转球 弧圈球 高吊弧圈 前冲弧圈
    跑动 步法 步伐 并步 交叉步 侧步 后退步 垫步 蹬地 起跳 落地
    双打 单打 混双 男单 女单 男双 女双 混合双打 双打站位 双打配合
    游泳 泳池 游泳池 泳道 泳姿 蛙泳 自由泳 仰泳 蝶泳 混合泳
    踩水 漂浮 打腿 划水 划臂 换气 呼吸 换气节奏 呼吸节奏 水中呼吸
    水中漂浮 仰漂 俯漂 浮板 打水板 浮力板 夹腿浮板 泳圈 救生浮具
    泳衣 泳装 泳裤 泳帽 泳镜 鼻夹 耳塞 游泳耳塞 防水泳包 速干毛巾
    防雾泳镜 长袖泳衣 连体泳衣 分体泳衣 连体泳装 分体泳装 儿童泳衣
    成人泳衣 训练泳衣 竞速泳衣 训练泳镜 儿童泳镜 脚蹼 长脚蹼 短脚蹼
    潜水 浮潜 深潜 自由潜 水肺潜水 潜水镜 呼吸管 潜水服 湿式潜服
    干式潜服 潜水靴 潜水袜 潜水手套 冲浪 桨板 皮划艇 划艇 赛艇 帆船
    水上运动 海上运动 水上活动 水上项目 冰上运动 滑冰 滑雪 单板 双板
    滑雪板 滑雪杖 滑雪鞋 滑雪服 滑雪镜 雪镜 冰鞋 旱冰鞋 轮滑鞋
    轮滑 滑板 滑板车 冰刀 溜冰 旱冰 爬山 骑马 射箭 射箭馆 弓箭
    """),
]

OVERRIDES = {
    "忐忑不安": "tantebuan", "心事重重": "xinshichongchong", "反反复复": "fanfanfufu",
    "重重": "chongchong", "轻重": "qingzhong", "自重": "zizhong", "重装徒步": "zhongzhuangtubu",
    "请稍候": "qingshaohou", "稍微一下": "shaoweiyixia", "稍微等等": "shaoweidengdeng",
    "相处融洽": "xiangchurongqia", "相差不大": "xiangchabuda", "相差较大": "xiangchajiaoda",
    "兴高采烈": "xinggaocailie", "兴致勃勃": "xingzhibobo", "兴冲冲": "xingchongchong",
    "兴致": "xingzhi", "兴头": "xingtou", "乐呵": "lehe", "乐呵呵": "lehehe",
    "乐陶陶": "letaotao", "乐滋滋": "lezizi", "快乐": "kuaile", "乐观": "leguan",
    "不着急": "buzhaoji", "别着急": "biezhaoji", "不急不躁": "bujibuzao",
    "衣着": "yizhuo", "着装": "zhuozhuang", "穿着": "chuanzhuo", "连着": "lianzhe",
    "惦着": "dianzhe", "睡眼惺忪": "shuiyanxingsong", "舍不得": "shebude",
    "好好休息": "haohaoxiuxi", "好好吃饭": "haohaochifan", "好好睡觉": "haohaoshuijiao",
    "好好学习": "haohaoxuexi", "好好工作": "haohaogongzuo", "爱好": "aihao", "好奇": "haoqi",
    "好学": "haoxue", "好胜心": "haoshengxin", "好奇心": "haoqixin",
    "好久不见": "haojiubujian", "好脾气": "haopiqi", "好心肠": "haoxinchang",
    "执拗": "zhiniu", "倔强": "juejiang", "强光手电": "qiangguangshoudian",
    "坚强": "jianqiang", "意志坚强": "yizhijianqiang", "自强不息": "ziqiangbuxi",
    "亲子装": "qinzizhuang", "亲子游": "qinziyou", "称心": "chenxin",
    "自省": "zixing", "反省": "fanxing", "行程": "xingcheng", "行李": "xingli",
    "划算": "huasuan", "划得来": "huadelai", "划船机": "huachuanji", "划水": "huashui",
    "划臂": "huabi", "划艇": "huating", "皮划艇": "pihuating",
    "坊间": "fangjian", "坊市": "fangshi", "薄毛衣": "baomaoyi", "薄外套": "baowaitao",
    "薄袜": "baowa", "薄底": "baodi", "薄款": "baokuan", "厚薄": "houbao", "偏薄": "pianbao",
    "衬衫": "chenshan", "衬衣": "chenyi", "白衬衫": "baichenshan", "棉衬衫": "mianchenshan",
    "风衣": "fengyi", "锦纶": "jinlun", "涤纶": "dilun", "氨纶": "anlun",
    "粘纤": "nianxian", "粘胶纤维": "nianjiaoxianwei", "纤维": "xianwei", "聚酯纤维": "juzhixianwei",
    "醋酸纤维": "cusuanxianwei", "蕾丝": "leisi", "柞蚕丝": "zuocansi", "苎麻": "zhuma",
    "长绒棉": "changrongmian", "毛绒绒": "maorongrong", "法兰绒": "falanrong",
    "行李箱": "xinglixiang", "行李包": "xinglibao", "行李牌": "xinglipai", "行李套": "xinglitao",
    "行李绑带": "xinglibangdai", "拉杆箱": "laganxiang", "系带鞋": "jidaixie",
    "夹克": "jiake", "夹克衫": "jiakeshan", "皮夹克": "pijiake", "棒球夹克": "bangqiujiake",
    "机车夹克": "jichejiake", "发卡": "faka", "发夹": "fajia", "抓夹": "zhuajia",
    "发簪": "fazan", "发箍": "fagu", "发圈": "faquan", "盘发夹": "panfajia",
    "马尾夹": "maweijia", "鲨鱼夹": "shayujia", "香蕉夹": "xiangjiaojia", "一字夹": "yizijia",
    "待人友善": "dairenyoushan", "待人真诚": "dairenzhencheng", "坦诚相待": "tanchengxiangdai",
    "几何": "jihe", "畏缩": "weisuo", "松垮垮": "songkuakua", "滑溜溜": "hualiuliu",
    "黑黢黢": "heiququ", "红彤彤": "hongtongtong", "绿油油": "lvyouyou",
    "无痕内裤": "wuhenneiku", "文胸": "wenxiong", "胸罩": "xiongzhao",
    "弹力": "tanli", "弹力带": "tanlidai", "弹力绳": "tanlisheng", "弹力臀桥": "tanlitunqiao",
    "高弹": "gaotan", "微弹": "weitan", "无弹": "wutan", "总负重": "zongfuzhong",
    "装备重量": "zhuangbeizhongliang", "基础重量": "jichuzhongliang", "负重": "fuzhong",
    "负重深蹲": "fuzhongshendun", "单臂划船": "danbihuachuan", "仰卧举腿": "yangwojutui",
    "仰泳": "yangyong", "燕尾": "yanwei", "腘绳肌": "guoshengji", "肱二头肌": "gongertouji",
    "肱三头肌": "gongsantouji", "腹直肌": "fuzhiji", "腹斜肌": "fuxieji", "腹横肌": "fuhengji",
    "脚蹼": "jiaopu", "长脚蹼": "changjiaopu", "短脚蹼": "duanjiaopu", "蛙泳": "wayong",
    "大体来说": "datilaishuo", "折返跑": "zhefanpao", "折叠床": "zhediechuang",
    "关节": "guanjie", "折叠桌": "zhediezhuo", "折叠椅": "zhedieyi", "折叠水桶": "zhedieshuitong",
    "什么": "shenme", "朝夕": "zhaoxi", "跪姿俯卧": "guizifuwo", "脊柱": "jizhu",
    "不转球": "buzhuanqiu", "转身": "zhuanshen", "上旋": "shangxuan", "下旋": "xiaxuan",
    "内旋": "neixuan", "外旋": "waixuan", "侧旋": "cexuan", "前冲弧圈": "qianchonghuquan",
    "高吊弧圈": "gaodiaohuquan", "弧圈球": "huquanqiu", "出乎意料": "chuhuyiliao",
    "发球": "faqiu", "发长球": "fachangqiu", "发短球": "faduanqiu", "发平高球": "fapinggaoqiu",
    "发高远球": "fagaoyuanqiu", "接发球": "jiefaqiu", "接发抢攻": "jiefaqianggong",
    "夹腿浮板": "jiatuifuban", "海滨度假": "haibindujia", "度假村": "dujiacun",
    "度假酒店": "dujiajiudian", "度假区": "dujiaqu", "度假": "dujia",
    "风绳": "fengsheng", "簿册": "buce", "侧翼": "ceyi", "拌嘴": "banzui",
    "血压": "xueya", "中长跑": "zhongchangpao", "长距离": "changjuli", "长款": "changkuan",
    "长筒袜": "changtongwa", "长裙": "changqun", "长袖": "changxiu", "长靴": "changxue",
    "长马甲": "changmajia", "中长款": "zhongchangkuan", "中长裙": "zhongchangqun",
    "中长大衣": "zhongchangdayi", "长外套": "changwaitao", "长大衣": "changdayi",
    "裤长": "kuchang", "裙长": "qunchang", "袖长": "xiuchang", "衣长": "yichang",
    "鞋长": "xiechang", "偏长": "pianchang", "长版": "changban", "步长": "buchang",
    "长羽绒服": "changyurongfu", "降温": "jiangwen", "累计爬升": "leijipasheng",
    "臀推": "tuntui", "扑球": "puqiu", "网前扑球": "wangqianpuqiu", "泊车": "boche",
    "铅笔裙": "qianbiqun", "铅笔裤": "qianbiku", "契尔西": "qieerxi", "切尔西靴": "qieerxixue",
    "模样": "muyang", "模板": "moban", "拂面": "fumian", "和睦": "hemu",
    "和睦相处": "hemuxiangchu", "乐于助人": "leyuzhuren", "乐意帮忙": "leyibangmang",
    "和解": "hejie", "暖和": "nuanhuo", "暖乎乎": "nuanhuhu", "热乎乎": "rehuhu",
    "简朴": "jianpu", "长传": "changchuan", "标识带": "biaozhidai", "路线标识": "luxianbiaozhi",
}

# Written zhi and common spoken shi are explicitly supported for these two new
# terms, matching the existing batch 004 policy; no automatic alias expansion.
ALIASES = {"标识带": ["biaoshidai"], "路线标识": ["luxianbiaoshi"]}
POLYPHONE_REVIEW_CHARACTERS = "行长重调假还得藏薄处校系载便单转觉乐宿量数供应间差弹传解着难干圈缝会空省发划钻泊铺屈强夹纶粘兴好拗露辟落翘折角率卷鲜降参盛秘斗闷蒙漂踏纤蚕蕃质颈杆壳舍脏脯咽咳骨累识"

# Draft labels rejected for being clipped, awkward or outside the requested form.
EXCLUDED = {
    "stubborn", "浴室拖沙", "滩鞋", "一次用品", "房间保险", "装饰点", "搭配色",
    "不舒服的", "干燥燥", "粗糙糙", "偏静", "惆然", "你觉得呢", "没问题的", "没关系的",
    "富有爱心", "体谅他人", "保加利亚", "罗马尼亚", "跪姿俯卧", "标准俯卧",
    "上斜俯卧", "下斜俯卧", "运动前后", "最大摄氧", "动作呼吸", "拍线",
    "扎帐", "防水收纳", "私人团", "高强间歇", "轻薄羽绒", "法兰绒服",
    "珊瑚绒服", "骑行背带", "轻飘飘的", "一大点", "打趣人", "景区车", "年卡票",
    "柒分裤", "箱包套", "防熊罐", "下回见", "再约吧",
}

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main() -> None:
    base = ROOT / "resources/dictionary"
    baselines = [base / name for name in ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv")]
    baselines += [OUT / f"lexicon_batch_{n:03d}.tsv" for n in range(1, 6)]
    known_keys, known_words = set(), set()
    for path in baselines:
        for line in path.read_text(encoding="utf-8-sig").splitlines():
            if line and not line.startswith("#"):
                key, word, _ = line.split("\t")
                known_keys.add((key, word)); known_words.add(word)
    benchmarks = sorted((ROOT / "tests/data").glob("typing_*.tsv"))
    benchmark_answers = set()
    for path in benchmarks:
        for line in path.read_text(encoding="utf-8-sig").splitlines():
            if line and not line.startswith("#"):
                benchmark_answers.add(line.split("\t")[1])
    selected, seen, excluded_alternate = [], set(), []
    draft_count = 0
    for category, frequency, text in SECTIONS:
        for word in text.split():
            draft_count += 1
            if word in seen or word in EXCLUDED or not re.fullmatch(r"[\u4e00-\u9fff]{2,4}", word):
                continue
            seen.add(word)
            if word in benchmark_answers:
                raise ValueError(f"Benchmark answer in draft: {word}")
            key = OVERRIDES.get(word) or "".join(lazy_pinyin(word, style=Style.NORMAL, strict=True))
            if not re.fullmatch(r"[a-z]+", key):
                raise ValueError(f"Invalid pronunciation: {word} {key}")
            if (key, word) in known_keys:
                continue
            if word in known_words:
                excluded_alternate.append({"word": word, "proposed_pinyin": key})
                continue
            selected.append((category, key, word, frequency))
            selected.extend((category, alias, word, frequency)
                            for alias in ALIASES.get(word, []) if (alias, word) not in known_keys)
    selected.sort(key=lambda r: (r[0], r[1], r[2]))
    assert len(selected) == len({(key, word) for category, key, word, frequency in selected})
    for category, key, word, frequency in selected:
        positions = {0}
        for char in word:
            options = pinyin(char, style=Style.NORMAL, heteronym=True, strict=True)[0]
            positions = {i + len(reading) for i in positions for reading in options if key.startswith(reading, i)}
        if len(key) not in positions:
            raise ValueError(f"Impossible character reading: {key} {word}")
        assert (key, word) not in known_keys and word not in known_words
    lines, previous = [], None
    for category, key, word, frequency in selected:
        if previous != category:
            lines.append(f"# category: {category}"); previous = category
        lines.append(f"{key}\t{word}\t{frequency}")
    output = OUT / "lexicon_batch_006.tsv"
    output.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    selected_words = {r[2] for r in selected}
    metadata = {
        "batch": "lexicon_batch_006", "authoring_source": "first_party_authored",
        "external_lexicon_import": False, "pronunciation_tool": "pypinyin authoring-only; explicit reviewed compound overrides",
        "authored_candidates": draft_count, "new_entries": len(selected), "unique_words": len(selected_words),
        "sha256": sha(output), "category_counts": dict(sorted(Counter(r[0] for r in selected).items())),
        "length_distribution": dict(sorted(Counter(str(len(r[2])) for r in selected).items())),
        "frequency_range": [min(r[3] for r in selected), max(r[3] for r in selected)],
        "dedup_policy": "Exclude existing key/display pair and existing display words with alternate generated readings",
        "dedup_baseline_sha256": {p.name: sha(p) for p in baselines},
        "benchmark_sha256": {p.name: sha(p) for p in benchmarks},
        "benchmark_full_answers_added": 0,
        "alternate_pronunciation_candidates_excluded": excluded_alternate,
        "manual_pronunciation_overrides": {w: k for w, k in OVERRIDES.items() if w in selected_words},
        "explicit_input_aliases": {w: keys for w, keys in ALIASES.items() if w in selected_words},
        "alias_notes": "Written zhi/common spoken shi for the two explicitly listed new identifier terms; same policy as batch 004.",
        "pronunciation_review_references": ["https://dict.revised.moe.edu.tw/dictView.jsp?ID=18492&q=1&word=14"],
        "validation": {"unique_key_display_pairs": len(selected), "format_valid": True,
            "all_outputs_two_to_four_han": True, "per_character_possible_readings_valid": True,
            "canonical_or_previous_batch_collisions": 0, "benchmark_full_answers_added": 0,
            "polyphone_reviewed_display_words": sum(any(c in w for c in POLYPHONE_REVIEW_CHARACTERS) for w in selected_words)},
        "canonical_dictionaries_modified": False,
    }
    output.with_suffix(".json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({k: metadata[k] for k in ("new_entries", "authored_candidates", "sha256", "category_counts")}, ensure_ascii=False))

if __name__ == "__main__":
    main()
